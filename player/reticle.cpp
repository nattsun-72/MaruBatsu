/****************************************
 * @file reticle.cpp
 * @brief 画面中央のレティクル（照準）表示
 * @author Natsume Shidara
 * @date 2026/02/22
 * @update 2026/02/22 - フィーバーティアに応じた色変化
 ****************************************/

#include "reticle.h"
#include "texture.h"
#include "sprite.h"
#include "direct3d.h"
#include "fever.h"
#ifdef USE_ASSET_DLL
#include "asset_dll.h"
#include "resource.h"
#endif

#include <DirectXMath.h>
#include <algorithm>

using namespace DirectX;

// 定数
namespace
{
    constexpr const wchar_t* WHITE_TEXTURE_PATH = L"assets/white.png";

    // レティクルサイズ
    constexpr float LINE_LENGTH = 16.0f;    // 線の長さ（片側）
    constexpr float LINE_THICKNESS = 2.0f;  // 線の太さ
    constexpr float GAP = 4.0f;             // 中心からの隙間

    // フィーバーティア別レティクルカラー
    // ※ CUD（カラーユニバーサルデザイン）準拠
    //   明度・彩度差を大きく取り、色覚タイプを問わず識別可能な配色
    //
    // None : 白（ニュートラル）
    // Tier1: スカイブルー  - 明るい青（全色覚タイプで「青」として認識される安全色）
    // Tier2: オレンジ      - 暖色系（青との明度・色相差が最大で、P/D型でも黄系として識別可能）
    // Tier3: イエロー      - 最高輝度（全色覚タイプで最も目立つ色。最強段階にふさわしい）
    const XMFLOAT4 COLOR_NONE  = { 1.00f, 1.00f, 1.00f, 0.70f }; // 白
    const XMFLOAT4 COLOR_TIER1 = { 0.40f, 0.75f, 1.00f, 0.85f }; // スカイブルー
    const XMFLOAT4 COLOR_TIER2 = { 1.00f, 0.60f, 0.15f, 0.90f }; // オレンジ
    const XMFLOAT4 COLOR_TIER3 = { 1.00f, 0.95f, 0.20f, 0.95f }; // イエロー

    // 色遷移の補間速度（秒あたりの補間率）
    constexpr float COLOR_LERP_SPEED = 5.0f;
}

// 内部変数
static int g_WhiteTexId = -1;
static XMFLOAT4 g_CurrentColor = COLOR_NONE;

// ヘルパー関数

// フィーバーティアに対応するターゲット色を取得
static const XMFLOAT4& GetTargetColor(FeverTier tier)
{
    switch (tier)
    {
    case FeverTier::Tier1: return COLOR_TIER1;
    case FeverTier::Tier2: return COLOR_TIER2;
    case FeverTier::Tier3: return COLOR_TIER3;
    default:               return COLOR_NONE;
    }
}

// XMFLOAT4 をチャネルごとに線形補間
static XMFLOAT4 LerpColor(const XMFLOAT4& a, const XMFLOAT4& b, float t)
{
    t = (std::max)(0.0f, (std::min)(1.0f, t));
    return {
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t,
        a.w + (b.w - a.w) * t,
    };
}

// 初期化
void Reticle_Initialize()
{
#ifdef USE_ASSET_DLL
    {
        const void* pData = nullptr;
        size_t dataSize = 0;
        if (AssetDLL_GetData(IDR_TEX_WHITE, &pData, &dataSize))
            g_WhiteTexId = Texture_LoadFromMemory(pData, dataSize, L"reticle_white");
    }
#else
    g_WhiteTexId = Texture_Load(WHITE_TEXTURE_PATH);
#endif
    g_CurrentColor = COLOR_NONE;
}

// 終了処理
void Reticle_Finalize()
{
    g_WhiteTexId = -1;
}

// 更新（色の補間）
void Reticle_Update(float dt)
{
    const XMFLOAT4& target = GetTargetColor(Fever_GetTier());
    float t = (std::min)(1.0f, COLOR_LERP_SPEED * dt);
    g_CurrentColor = LerpColor(g_CurrentColor, target, t);
}

// 描画
void Reticle_Draw()
{
    if (g_WhiteTexId < 0) return;

    float screenW = static_cast<float>(Direct3D_GetBackBufferWidth());
    float screenH = static_cast<float>(Direct3D_GetBackBufferHeight());
    float cx = screenW * 0.5f;
    float cy = screenH * 0.5f;

    Sprite_Begin();

    // 上の線
    Sprite_Draw(g_WhiteTexId,
        cx - LINE_THICKNESS * 0.5f,
        cy - GAP - LINE_LENGTH,
        LINE_THICKNESS, LINE_LENGTH,
        0.0f, g_CurrentColor);

    // 下の線
    Sprite_Draw(g_WhiteTexId,
        cx - LINE_THICKNESS * 0.5f,
        cy + GAP,
        LINE_THICKNESS, LINE_LENGTH,
        0.0f, g_CurrentColor);

    // 左の線
    Sprite_Draw(g_WhiteTexId,
        cx - GAP - LINE_LENGTH,
        cy - LINE_THICKNESS * 0.5f,
        LINE_LENGTH, LINE_THICKNESS,
        0.0f, g_CurrentColor);

    // 右の線
    Sprite_Draw(g_WhiteTexId,
        cx + GAP,
        cy - LINE_THICKNESS * 0.5f,
        LINE_LENGTH, LINE_THICKNESS,
        0.0f, g_CurrentColor);

    // 中心ドット（小さい点）
    constexpr float DOT_SIZE = 2.0f;
    Sprite_Draw(g_WhiteTexId,
        cx - DOT_SIZE * 0.5f,
        cy - DOT_SIZE * 0.5f,
        DOT_SIZE, DOT_SIZE,
        0.0f, g_CurrentColor);
}
