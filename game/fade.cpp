/****************************************
 * @file   fade.cpp
 * @brief  画面のフェードイン・アウト制御の実装
 * @author Natsume Shidara
 * @date   2025/07/10
 ****************************************/

#include "fade.h"

#include <algorithm>

#include "debug_ostream.h"
#include "direct3d.h"
#include "sprite.h"
#include "texture.h"
#ifdef USE_ASSET_DLL
#include "asset_dll.h"
#include "resource.h"
#endif

//--------------------------------------
// 内部状態
//--------------------------------------
static double g_FadeTime        = 0.0;              // フェードにかける総秒数
static double g_FadeStartTime   = 0.0;              // フェード開始時の累積時間
static double g_AccumulatedTime = 0.0;              // 起動からの累積時間
static auto   g_State           = FADE_STATE::NONE; // 現在のフェード状態
static int    g_FadeTexId       = -1;               // フェード用1pxテクスチャID

static float        g_Alpha = 0.0f;                 // 現在の不透明度(0:透明〜1:不透明)
static Color::COLOR g_color = Color::BLACK;         // フェード色

//======================================
// 初期化・終了
//======================================
void Fade_Initialize()
{
    g_FadeTime        = 1.0;
    g_FadeStartTime   = 0.0;
    g_AccumulatedTime = 0.0;
    g_color           = Color::BLACK;
    g_Alpha           = 0.0f;
    g_State           = FADE_STATE::NONE;

    /*--- フェード用テクスチャ(白1px)の読み込み ---*/
#ifdef USE_ASSET_DLL
    {
        // x64構成: AssetDLLに埋め込まれたリソースから読み込む
        const void* pData = nullptr;
        size_t dataSize = 0;
        if (AssetDLL_GetData(IDR_TEX_WHITE, &pData, &dataSize))
            g_FadeTexId = Texture_LoadFromMemory(pData, dataSize, L"fade_white");
    }
#else
    // 非DLL構成: ファイルから直接読み込む
    g_FadeTexId = Texture_Load(L"assets/white.png");
#endif
}

void Fade_Finalize()
{}

//======================================
// 更新・描画
//======================================
void Fade_Update(double elapsed_time)
{
    // 完了状態・無状態のときは更新不要
    if (g_State == FADE_STATE::FINISHED_IN || g_State == FADE_STATE::FINISHED_OUT)
        return;
    if (g_State == FADE_STATE::NONE)
        return;

    g_AccumulatedTime += elapsed_time;

    // 進捗を 0.0〜1.0 に正規化
    double progress = min((g_AccumulatedTime - g_FadeStartTime) / g_FadeTime, 1.0);

    // 進捗が1に達したら完了状態へ遷移
    if (progress >= 1)
    {
        g_State = g_State == FADE_STATE::FADE_IN ? FADE_STATE::FINISHED_IN : FADE_STATE::FINISHED_OUT;
    }

    // フェードイン時は不透明→透明、アウト時は透明→不透明
    g_Alpha = static_cast<float>(g_State == FADE_STATE::FADE_IN ? 1.0 - progress : progress);
}

void Fade_Draw()
{
    // 何も覆わない状態では描画不要
    if (g_State == FADE_STATE::NONE)
        return;
    if (g_State == FADE_STATE::FINISHED_IN)
        return;

    // 現在のアルファをフェード色へ適用
    Color::COLOR color = g_color;
    color.w = g_Alpha;

    // 画面全体を覆う矩形を描画
    float width  = static_cast<float>(Direct3D_GetBackBufferWidth());
    float height = static_cast<float>(Direct3D_GetBackBufferHeight());
    Sprite_Draw(g_FadeTexId, 0.0f, 0.0f, width, height, 0.0f, color);
}

//======================================
// フェード開始・状態取得
//======================================
void Fade_Start(double duration, bool isFadeOut, Color::COLOR color)
{
    g_FadeStartTime = g_AccumulatedTime;
    g_FadeTime      = duration;
    g_State         = isFadeOut ? FADE_STATE::FADE_OUT : FADE_STATE::FADE_IN;
    g_color         = color;
    g_Alpha         = isFadeOut ? 1.0f : 0.0f;
}

FADE_STATE Fade_GetState()
{
    return g_State;
}
