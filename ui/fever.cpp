/****************************************
 * @file fever.cpp
 * @brief フィーバーモード管理の実装
 * @detail
 * - 毎フレームCombo_GetCount()を参照しティアを判定
 * - ティア変化時にFOV遷移・各種パラメータ切替
 * - Tier2+で金色ビネット描画・HP自動回復
 *
 * @author Natsume Shidara
 * @date 2026/02/22
 ****************************************/

#include "fever.h"
#include "combo.h"
#include "player.h"
#include "player_camera.h"
#include "direct3d.h"
#include "sprite.h"

#include <cmath>
#include <algorithm>
#include <vector>
#include <d3d11.h>
#include <DirectXMath.h>

using namespace DirectX;

// 定数
namespace
{
    // ティア閾値
    constexpr int TIER1_THRESHOLD = 50;
    constexpr int TIER2_THRESHOLD = 100;
    constexpr int TIER3_THRESHOLD = 200;

    // Tier 1: 移動速度倍率
    constexpr float SPEED_MULTIPLIER = 1.3f;

    // Tier 1: FOV
    constexpr float FEVER_FOV       = 1.2f;
    constexpr float FEVER_FOV_SPEED = 5.0f;

    // Tier 1: ジャンプ・エアダッシュ上限
    constexpr int FEVER_MAX_JUMP      = 3;
    constexpr int FEVER_MAX_AIR_DASH  = 2;
    constexpr int NORMAL_MAX_JUMP     = 2;
    constexpr int NORMAL_MAX_AIR_DASH = 1;

    // Tier 2: HP回復
    constexpr float HP_RECOVER_INTERVAL = 2.5f;
    constexpr int   HP_RECOVER_AMOUNT   = 1;

    // Tier 2: 金色ビネット
    constexpr float GOLD_PULSE_SPEED = 2.5f;
    constexpr float GOLD_MIN_ALPHA   = 0.3f;
    constexpr float GOLD_MAX_ALPHA   = 0.75f;
    constexpr int   VIGNETTE_TEX_SIZE = 256;

    // Tier 3: 攻撃範囲倍率
    constexpr float RANGE_MULTIPLIER = 1.4f;

    // ビネット距離正規化
    constexpr float INV_SQRT2 = 0.707f;          // 1/√2（対角距離の正規化係数）

    // ビネットしきい値
    constexpr float VIGNETTE_INNER_THRESHOLD = 0.05f;   // ビネット内側しきい値
    constexpr float VIGNETTE_OUTER_RANGE = 0.95f;       // ビネット外側範囲

    // 金色RGB値
    constexpr uint8_t GOLD_COLOR_R = 255;         // 金色のRed成分
    constexpr uint8_t GOLD_COLOR_G = 200;         // 金色のGreen成分
    constexpr uint8_t GOLD_COLOR_B = 50;          // 金色のBlue成分

    // RGBA パッキング用ビットシフト
    constexpr int SHIFT_ALPHA = 24;               // アルファチャネルのビットシフト量
    constexpr int SHIFT_BLUE = 16;                // 青チャネルのビットシフト量
    constexpr int SHIFT_GREEN = 8;                // 緑チャネルのビットシフト量

    // 金色パルスの周波数倍率
    constexpr float GOLD_PULSE_FREQ_MULTIPLIER = 2.0f;  // パルス周波数の乗数
}

// 内部変数
static FeverTier g_CurrentTier  = FeverTier::None;
static FeverTier g_PreviousTier = FeverTier::None;

static float g_HpRecoverTimer  = 0.0f;
static float g_GoldPulseTimer  = 0.0f;

static ID3D11ShaderResourceView* g_pGoldVignetteSRV = nullptr;
static float g_ScreenWidth  = 0.0f;
static float g_ScreenHeight = 0.0f;

// 金色ビネットテクスチャ生成（ランタイム）
static ID3D11ShaderResourceView* CreateGoldVignetteTexture(ID3D11Device* pDevice)
{
    const int W = VIGNETTE_TEX_SIZE;
    const int H = VIGNETTE_TEX_SIZE;

    std::vector<uint32_t> pixels(W * H);

    for (int y = 0; y < H; y++)
    {
        for (int x = 0; x < W; x++)
        {
            float u = (static_cast<float>(x) + 0.5f) / static_cast<float>(W);
            float v = (static_cast<float>(y) + 0.5f) / static_cast<float>(H);

            float dx = u - 0.5f;
            float dy = v - 0.5f;
            float dist = sqrtf(dx * dx + dy * dy) / INV_SQRT2;

            float vignette = dist * dist;
            vignette = std::fminf(1.0f, vignette);
            vignette = std::fmaxf(0.0f, (vignette - VIGNETTE_INNER_THRESHOLD) / VIGNETTE_OUTER_RANGE);
            vignette = sqrtf(vignette);

            // 金色
            uint8_t r = GOLD_COLOR_R;
            uint8_t g = GOLD_COLOR_G;
            uint8_t b = GOLD_COLOR_B;
            uint8_t a = static_cast<uint8_t>(vignette * 255.0f);

            pixels[y * W + x] = (static_cast<uint32_t>(a) << SHIFT_ALPHA) |
                                 (static_cast<uint32_t>(b) << SHIFT_BLUE) |
                                 (static_cast<uint32_t>(g) << SHIFT_GREEN) |
                                 (static_cast<uint32_t>(r));
        }
    }

    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width            = W;
    texDesc.Height           = H;
    texDesc.MipLevels        = 1;
    texDesc.ArraySize        = 1;
    texDesc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage            = D3D11_USAGE_IMMUTABLE;
    texDesc.BindFlags        = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem      = pixels.data();
    initData.SysMemPitch   = W * sizeof(uint32_t);

    ID3D11Texture2D* pTex = nullptr;
    HRESULT hr = pDevice->CreateTexture2D(&texDesc, &initData, &pTex);
    if (FAILED(hr)) return nullptr;

    ID3D11ShaderResourceView* pSRV = nullptr;
    hr = pDevice->CreateShaderResourceView(pTex, nullptr, &pSRV);
    pTex->Release();

    if (FAILED(hr)) return nullptr;
    return pSRV;
}

// ティア判定
static FeverTier DetermineTier(int comboCount)
{
    if (comboCount >= TIER3_THRESHOLD) return FeverTier::Tier3;
    if (comboCount >= TIER2_THRESHOLD) return FeverTier::Tier2;
    if (comboCount >= TIER1_THRESHOLD) return FeverTier::Tier1;
    return FeverTier::None;
}

// 初期化
void Fever_Initialize()
{
    g_CurrentTier    = FeverTier::None;
    g_PreviousTier   = FeverTier::None;
    g_HpRecoverTimer = 0.0f;
    g_GoldPulseTimer = 0.0f;

    ID3D11Device* pDevice = Direct3D_GetDevice();
    if (pDevice)
    {
        g_pGoldVignetteSRV = CreateGoldVignetteTexture(pDevice);
    }

    g_ScreenWidth  = static_cast<float>(Direct3D_GetBackBufferWidth());
    g_ScreenHeight = static_cast<float>(Direct3D_GetBackBufferHeight());
}

// 終了
void Fever_Finalize()
{
    if (g_pGoldVignetteSRV)
    {
        g_pGoldVignetteSRV->Release();
        g_pGoldVignetteSRV = nullptr;
    }
    g_CurrentTier  = FeverTier::None;
    g_PreviousTier = FeverTier::None;
}

// 更新
void Fever_Update(float dt)
{
    g_PreviousTier = g_CurrentTier;
    g_CurrentTier  = DetermineTier(Combo_GetCount());

    // ティア変化時のFOV処理
    if (g_CurrentTier != g_PreviousTier)
    {
        if (g_CurrentTier >= FeverTier::Tier1)
        {
            PLCamera_SetTargetFOV(FEVER_FOV, FEVER_FOV_SPEED);
        }
        else
        {
            PLCamera_ResetFOV(FEVER_FOV_SPEED);
        }

        // Tier2未満に落ちたらHP回復タイマーリセット
        if (g_CurrentTier < FeverTier::Tier2)
        {
            g_HpRecoverTimer = 0.0f;
        }
    }

    // Tier 2+: HP回復
    if (g_CurrentTier >= FeverTier::Tier2)
    {
        g_HpRecoverTimer += dt;
        if (g_HpRecoverTimer >= HP_RECOVER_INTERVAL)
        {
            g_HpRecoverTimer -= HP_RECOVER_INTERVAL;
            Player_RecoverHP(HP_RECOVER_AMOUNT);
        }

        g_GoldPulseTimer += dt;
    }
    else
    {
        g_GoldPulseTimer = 0.0f;
    }
}

// 描画（金色ビネット）
void Fever_Draw()
{
    if (g_CurrentTier < FeverTier::Tier2) return;
    if (!g_pGoldVignetteSRV) return;

    float wave = (sinf(g_GoldPulseTimer * GOLD_PULSE_SPEED * GOLD_PULSE_FREQ_MULTIPLIER) + 1.0f) * 0.5f;
    float alpha = GOLD_MIN_ALPHA + wave * (GOLD_MAX_ALPHA - GOLD_MIN_ALPHA);

    Sprite_Begin();
    XMFLOAT4 vignetteColor = { 1.0f, 1.0f, 1.0f, alpha };
    Sprite_Draw(g_pGoldVignetteSRV,
        0.0f, 0.0f,
        g_ScreenWidth, g_ScreenHeight,
        0.0f, vignetteColor);
}

// ゲッター
FeverTier Fever_GetTier()
{
    return g_CurrentTier;
}

bool Fever_IsActive()
{
    return g_CurrentTier >= FeverTier::Tier1;
}

float Fever_GetSpeedMultiplier()
{
    return (g_CurrentTier >= FeverTier::Tier1) ? SPEED_MULTIPLIER : 1.0f;
}

float Fever_GetFOVTarget()
{
    return (g_CurrentTier >= FeverTier::Tier1) ? FEVER_FOV : PLCamera_GetDefaultFOV();
}

int Fever_GetMaxJumpCount()
{
    return (g_CurrentTier >= FeverTier::Tier1) ? FEVER_MAX_JUMP : NORMAL_MAX_JUMP;
}

int Fever_GetMaxAirDashCount()
{
    return (g_CurrentTier >= FeverTier::Tier1) ? FEVER_MAX_AIR_DASH : NORMAL_MAX_AIR_DASH;
}

float Fever_GetRangeMultiplier()
{
    return (g_CurrentTier >= FeverTier::Tier3) ? RANGE_MULTIPLIER : 1.0f;
}
