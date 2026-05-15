/****************************************
 * @file score.cpp
 * @brief スコア管理の実装
 * @author Natsume Shidara
 * @date 2025/07/09
 * @update 2026/01/12 - 各種機能をアクションゲーム用に改修
 ****************************************/

#include "score.h"
#include "texture.h"
#include "sprite.h"

#include <algorithm>
#include <DirectXMath.h>
#include <filesystem>

#ifdef USE_ASSET_DLL
#include "asset_dll.h"
#include "resource.h"
#endif

using namespace DirectX;

// 定数
namespace
{
    // テクスチャパス
    constexpr const wchar_t* SCORE_TEXTURE_PATH = L"assets/ui/numbers.png";

    // フォントサイズ（テクスチャ上）
    constexpr int FONT_SRC_WIDTH = 89;
    constexpr int FONT_SRC_HEIGHT = 123;

    // 描画スケール（1/4サイズで表示）
    constexpr float DRAW_SCALE = 0.25f;

    // カウントアップ所要時間（秒）
    constexpr float COUNT_UP_DURATION = 3.0f;

    // デフォルト設定
    constexpr int DEFAULT_DIGIT_COUNT = 7;                // デフォルト表示桁数
    constexpr unsigned int DEFAULT_COUNTER_STOP = 9999999; // デフォルトカウンターストップ値
}

// 内部変数
static unsigned int g_Score = 0;           // 実際のスコア
static unsigned int g_ViewScore = 0;       // 表示用スコア（カウントアップ演出）
static unsigned int g_FinalScore = 0;      // 最終スコア（リザルト用）
static int g_Digit = DEFAULT_DIGIT_COUNT;                    // 表示桁数
static unsigned int g_CounterStop = DEFAULT_COUNTER_STOP;  // 最大値
static float g_PosX = 0.0f;
static float g_PosY = 0.0f;
static int g_ScoreTexId = -1;

// 内部関数
static void DrawNumber(float x, float y, int num, float sizeX, float sizeY)
{
    if (g_ScoreTexId < 0) return;

    // 描画サイズ（1/4スケール適用）
    float drawW = FONT_SRC_WIDTH * DRAW_SCALE * sizeX;
    float drawH = FONT_SRC_HEIGHT * DRAW_SCALE * sizeY;

    // 描画（フルスペックで）
    // 引数順序: texid, x, y, uvX, uvY, uvW, uvH, drawW, drawH, angle, color
    Sprite_Draw(
        g_ScoreTexId,
        x, y,
        static_cast<float>(FONT_SRC_WIDTH * num), 0.0f,  // UV切り抜き位置
        static_cast<float>(FONT_SRC_WIDTH),               // UV幅
        static_cast<float>(FONT_SRC_HEIGHT),              // UV高さ
        drawW,                                            // 描画幅
        drawH,                                            // 描画高さ
        0.0f,                                             // 角度
        XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f)                 // 色
    );
}

// 初期化
void Score_Initialize(float x, float y, int digit)
{
    g_Score = 0;
    g_ViewScore = 0;
    g_Digit = digit;
    g_PosX = x;
    g_PosY = y;

#ifdef USE_ASSET_DLL
    {
        const void* pData = nullptr;
        size_t dataSize = 0;
        if (AssetDLL_GetData(IDR_TEX_UI_NUMBERS, &pData, &dataSize))
            g_ScoreTexId = Texture_LoadFromMemory(pData, dataSize, L"score_numbers");
    }
#else
    g_ScoreTexId = Texture_Load(SCORE_TEXTURE_PATH);
#endif

    // カウンターストップ値を計算
    g_CounterStop = 1;
    for (int i = 0; i < digit; i++)
    {
        g_CounterStop *= 10;
    }
    g_CounterStop--;
}

// 終了処理
void Score_Finalize()
{
    g_ScoreTexId = -1;
}

// 更新
void Score_Update(double elapsed_time)
{
    float dt = static_cast<float>(elapsed_time);

    // カウントアップ演出（COUNT_UP_DURATION秒で完了）
    if (g_ViewScore < g_Score)
    {
        // 1フレームあたりの加算量 = 目標値 / (所要時間 / dt)
        float addPerFrame = static_cast<float>(g_Score) / COUNT_UP_DURATION * dt;
        unsigned int add = static_cast<unsigned int>(std::fmaxf(1.0f, addPerFrame));
        g_ViewScore = (std::min)(g_ViewScore + add, g_Score);
    }
}

// 描画（位置・サイズ指定）
void Score_Draw(float x, float y, float sizeX, float sizeY)
{
    if (g_ScoreTexId < 0) return;

    Sprite_Begin();

    unsigned int temp = (std::min)(g_ViewScore, g_CounterStop);

    // 1文字の描画幅（DRAW_SCALE適用）
    float charWidth = FONT_SRC_WIDTH * DRAW_SCALE * sizeX;

    for (int i = 0; i < g_Digit; i++)
    {
        int n = temp % 10;

        // 右詰め描画（右寄せ）
        float drawX = x + (g_Digit - 1 - i) * charWidth;

        DrawNumber(drawX, y, n, sizeX, sizeY);
        temp /= 10;
    }
}

// 描画（デフォルト位置）
void Score_Draw()
{
    Score_Draw(g_PosX, g_PosY, 1.0f, 1.0f);
}

// スコア取得
unsigned int Score_GetScore()
{
    return g_Score;
}

// スコア加算
void Score_AddScore(int value)
{
    g_Score = (std::min)(g_Score + static_cast<unsigned int>(value), g_CounterStop);
}

// スコアリセット
void Score_ResetScore()
{
    g_Score = 0;
    g_ViewScore = 0;
}

// 最終スコア保存（ゲーム終了時）
void Score_SaveFinal()
{
    g_FinalScore = g_Score;
}

// 最終スコア取得
unsigned int Score_GetFinalScore()
{
    return g_FinalScore;
}

// リザルト画面用初期化（スコアはリセットしない）
void Score_InitializeForResult(float x, float y, int digit)
{
    g_Digit = digit;
    g_PosX = x;
    g_PosY = y;

    // 最終スコアを表示用にセット
    g_Score = g_FinalScore;
    g_ViewScore = 0;  // 0からカウントアップ演出

    // テクスチャ読み込み
#ifdef USE_ASSET_DLL
    {
        const void* pData = nullptr;
        size_t dataSize = 0;
        if (AssetDLL_GetData(IDR_TEX_UI_NUMBERS, &pData, &dataSize))
            g_ScoreTexId = Texture_LoadFromMemory(pData, dataSize, L"score_numbers_result");
    }
#else
    g_ScoreTexId = Texture_Load(SCORE_TEXTURE_PATH);
#endif

    // カウンターストップ値を計算
    g_CounterStop = 1;
    for (int i = 0; i < digit; i++)
    {
        g_CounterStop *= 10;
    }
    g_CounterStop--;
}