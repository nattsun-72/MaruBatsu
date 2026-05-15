/****************************************
 * @file damage_effect.cpp
 * @brief 被弾エフェクト（画面フラッシュ・瀕死時赤演出）
 * @author Natsume Shidara
 * @date 2026/02/19
 *
 * - 被弾時: 画面全体に赤いフラッシュ（短時間）
 * - 瀕死時（HP <= LOW_HP_THRESHOLD）:
 *   画面縁から赤いビネットが演出するポストエフェクト表現
 *
 * ビネットテクスチャはランタイムで生成（外部アセット不要）
 ****************************************/

#include "damage_effect.h"
#include "texture.h"
#include "sprite.h"
#include "direct3d.h"
#include "player.h"
#ifdef USE_ASSET_DLL
#include "asset_dll.h"
#include "resource.h"
#endif

#include <cmath>
#include <algorithm>
#include <vector>
#include <DirectXMath.h>

using namespace DirectX;

// 定数
namespace
{
    // 瀕死閾値（HP <= この値で赤演出開始）
    constexpr int LOW_HP_THRESHOLD = 3;

    // --- 被弾フラッシュ設定 ---
    constexpr float HIT_FLASH_DURATION = 0.15f;    // フラッシュ持続時間
    constexpr float HIT_FLASH_MAX_ALPHA = 0.35f;   // フラッシュ最大アルファ

    // --- 瀕死演出設定 ---
    constexpr float CRITICAL_PULSE_SPEED = 3.0f;   // 演出速度
    constexpr float CRITICAL_MIN_ALPHA = 0.10f;    // 演出最小アルファ（常に薄く赤い）
    constexpr float CRITICAL_MAX_ALPHA = 0.45f;    // 演出最大アルファ（画面全体に強い）

    // HP1の時はさらに強い表現
    constexpr float CRITICAL_HP1_PULSE_SPEED = 5.0f;
    constexpr float CRITICAL_HP1_MIN_ALPHA = 0.20f;
    constexpr float CRITICAL_HP1_MAX_ALPHA = 0.65f;

    // ビネットテクスチャサイズ
    constexpr int VIGNETTE_TEX_SIZE = 256;

    // --- ビネット距離正規化 ---
    constexpr float VIGNETTE_NORMALIZE_FACTOR = 0.707f; // √2の逆数（対角線距離を0〜1に正規化）

    // --- ビネットカーブ設定 ---
    constexpr float VIGNETTE_INNER_CUTOFF = 0.05f;      // 中心の透明領域（この割合以下は完全透明）
    constexpr float VIGNETTE_OUTER_RANGE = 0.95f;        // 外側のグラデーション範囲

    // --- ビネットRGBAカラー ---
    constexpr uint8_t VIGNETTE_COLOR_R = 220;            // 赤チャンネル（鮮やかな赤）
    constexpr uint8_t VIGNETTE_COLOR_G = 8;              // 緑チャンネル
    constexpr uint8_t VIGNETTE_COLOR_B = 5;              // 青チャンネル
    constexpr float VIGNETTE_ALPHA_SCALE = 255.0f;       // アルファ値スケール（0〜255）

    // --- RGBAパッキング用ビットシフト ---
    constexpr int SHIFT_ALPHA = 24;                      // アルファチャンネルのシフト量
    constexpr int SHIFT_BLUE = 16;                       // 青チャンネルのシフト量
    constexpr int SHIFT_GREEN = 8;                       // 緑チャンネルのシフト量

    // --- 被弾フラッシュ色設定 ---
    constexpr float FLASH_COLOR_G = 0.1f;                // フラッシュの緑成分
    constexpr float FLASH_COLOR_B = 0.05f;               // フラッシュの青成分

    // --- 瀕死パルス波形 ---
    constexpr float PULSE_FREQUENCY = 2.0f;              // パルス波の周波数倍率

    // --- フォールバック赤色 ---
    constexpr float FALLBACK_RED = 0.8f;                 // ビネット未生成時の赤色成分
}

// 内部変数

// テクスチャ
static int g_WhiteTexId = -1;
static ID3D11ShaderResourceView* g_pVignetteSRV = nullptr;

// 被弾フラッシュ
static float g_HitFlashTimer = 0.0f;

// 瀕死演出
static float g_CriticalPulseTimer = 0.0f;

// 画面サイズ
static float g_ScreenWidth = 0.0f;
static float g_ScreenHeight = 0.0f;

// ビネットテクスチャ生成（ランタイム）
// 中心が透明、縁が赤いラジアルグラデーション
static ID3D11ShaderResourceView* CreateVignetteTexture(ID3D11Device* pDevice)
{
    const int W = VIGNETTE_TEX_SIZE;
    const int H = VIGNETTE_TEX_SIZE;

    std::vector<uint32_t> pixels(W * H);

    for (int y = 0; y < H; y++)
    {
        for (int x = 0; x < W; x++)
        {
            // UV座標（0〜1）
            float u = (static_cast<float>(x) + 0.5f) / static_cast<float>(W);
            float v = (static_cast<float>(y) + 0.5f) / static_cast<float>(H);

            // 中心からの距離（0〜√2）
            float dx = u - 0.5f;
            float dy = v - 0.5f;
            float dist = sqrtf(dx * dx + dy * dy) / VIGNETTE_NORMALIZE_FACTOR;  // 0〜1に正規化

            // ビネットカーブ: 中心はやや透明、画面全体に赤が出やすい
            float vignette = dist * dist;  // 二乗カーブ
            vignette = std::fminf(1.0f, vignette);

            // 中心の20%のみ透明、それ以降は急速に赤くなる
            vignette = std::fmaxf(0.0f, (vignette - VIGNETTE_INNER_CUTOFF) / VIGNETTE_OUTER_RANGE);
            vignette = sqrtf(vignette);  // sqrtカーブで中間領域も赤くする

            uint8_t r = VIGNETTE_COLOR_R;  // より鮮やかな赤
            uint8_t g = VIGNETTE_COLOR_G;
            uint8_t b = VIGNETTE_COLOR_B;
            uint8_t a = static_cast<uint8_t>(vignette * VIGNETTE_ALPHA_SCALE);

            // RGBA（リトルエンディアン: ABGR順でパック）
            pixels[y * W + x] = (static_cast<uint32_t>(a) << SHIFT_ALPHA) |
                                 (static_cast<uint32_t>(b) << SHIFT_BLUE) |
                                 (static_cast<uint32_t>(g) << SHIFT_GREEN) |
                                 (static_cast<uint32_t>(r));
        }
    }

    // D3D11テクスチャ作成
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = W;
    texDesc.Height = H;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_IMMUTABLE;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = pixels.data();
    initData.SysMemPitch = W * sizeof(uint32_t);

    ID3D11Texture2D* pTex = nullptr;
    HRESULT hr = pDevice->CreateTexture2D(&texDesc, &initData, &pTex);
    if (FAILED(hr))
    {
        OutputDebugStringA("DamageEffect: CreateTexture2D for vignette failed\n");
        return nullptr;
    }

    ID3D11ShaderResourceView* pSRV = nullptr;
    hr = pDevice->CreateShaderResourceView(pTex, nullptr, &pSRV);
    pTex->Release();

    if (FAILED(hr))
    {
        OutputDebugStringA("DamageEffect: CreateSRV for vignette failed\n");
        return nullptr;
    }

    return pSRV;
}

// 初期化
void DamageEffect_Initialize()
{
#ifdef USE_ASSET_DLL
    {
        const void* pData = nullptr;
        size_t dataSize = 0;
        if (AssetDLL_GetData(IDR_TEX_WHITE, &pData, &dataSize))
            g_WhiteTexId = Texture_LoadFromMemory(pData, dataSize, L"damage_white");
    }
#else
    g_WhiteTexId = Texture_Load(L"assets/white.png");
#endif

    ID3D11Device* pDevice = Direct3D_GetDevice();
    if (pDevice)
    {
        g_pVignetteSRV = CreateVignetteTexture(pDevice);
    }

    g_HitFlashTimer = 0.0f;
    g_CriticalPulseTimer = 0.0f;

    g_ScreenWidth = static_cast<float>(Direct3D_GetBackBufferWidth());
    g_ScreenHeight = static_cast<float>(Direct3D_GetBackBufferHeight());
}

// 終了
void DamageEffect_Finalize()
{
    g_WhiteTexId = -1;

    if (g_pVignetteSRV)
    {
        g_pVignetteSRV->Release();
        g_pVignetteSRV = nullptr;
    }
}

// 被弾フラッシュトリガー
void DamageEffect_TriggerHitFlash()
{
    g_HitFlashTimer = HIT_FLASH_DURATION;
}

// 更新
void DamageEffect_Update(float dt)
{
    // 被弾フラッシュタイマー
    if (g_HitFlashTimer > 0.0f)
    {
        g_HitFlashTimer -= dt;
        if (g_HitFlashTimer < 0.0f)
            g_HitFlashTimer = 0.0f;
    }

    // 瀕死パルスタイマー（常に進行）
    g_CriticalPulseTimer += dt;
}

// 描画
void DamageEffect_Draw()
{
    int currentHP = Player_GetHP();
    bool drawHitFlash = (g_HitFlashTimer > 0.0f);
    bool drawCritical = (currentHP > 0 && currentHP <= LOW_HP_THRESHOLD);

    if (!drawHitFlash && !drawCritical) return;

    Sprite_Begin();

    //--------------------------------------
    // 1) 被弾フラッシュ（赤い画面オーバーレイ）
    //--------------------------------------
    if (drawHitFlash)
    {
        float t = g_HitFlashTimer / HIT_FLASH_DURATION;  // 1.0→0.0
        // イーズアウト: 最初に強く、すぐ消える
        float alpha = HIT_FLASH_MAX_ALPHA * t * t;

        XMFLOAT4 flashColor = { 1.0f, FLASH_COLOR_G, FLASH_COLOR_B, alpha };

        Sprite_Draw(g_WhiteTexId,
            0.0f, 0.0f,
            g_ScreenWidth, g_ScreenHeight,
            0.0f, flashColor);
    }

    //--------------------------------------
    // 2) 瀕死時の赤演出ビネット
    //--------------------------------------
    if (drawCritical)
    {
        // HP1の時はさらに強く速い演出
        float pulseSpeed = (currentHP <= 1) ? CRITICAL_HP1_PULSE_SPEED : CRITICAL_PULSE_SPEED;
        float maxAlpha   = (currentHP <= 1) ? CRITICAL_HP1_MAX_ALPHA   : CRITICAL_MAX_ALPHA;
        float minAlpha   = (currentHP <= 1) ? CRITICAL_HP1_MIN_ALPHA   : CRITICAL_MIN_ALPHA;

        // sinカーブで0〜1に正規化
        float wave = (sinf(g_CriticalPulseTimer * pulseSpeed * PULSE_FREQUENCY) + 1.0f) * 0.5f;
        float alpha = minAlpha + wave * (maxAlpha - minAlpha);

        if (g_pVignetteSRV)
        {
            // ビネットSRV直接描画（赤いビネットテクスチャ）
            XMFLOAT4 vignetteColor = { 1.0f, 1.0f, 1.0f, alpha };
            Sprite_Draw(g_pVignetteSRV,
                0.0f, 0.0f,
                g_ScreenWidth, g_ScreenHeight,
                0.0f, vignetteColor);
        }
        else
        {
            // フォールバック: 白テクスチャを赤く染めて全画面オーバーレイ
            XMFLOAT4 fallbackColor = { FALLBACK_RED, 0.0f, 0.0f, alpha };
            Sprite_Draw(g_WhiteTexId,
                0.0f, 0.0f,
                g_ScreenWidth, g_ScreenHeight,
                0.0f, fallbackColor);
        }
    }
}
