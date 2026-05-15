/****************************************
 * @file result.cpp
 * @brief リザルト画面の実装
 * @author Natsume Shidara
 * @date 2025/08/19
 * @update 2026/01/12 - 斬空アクションゲーム用に改造
 * @update 2026/01/13 - サウンド対応
 * @update 2026/02/18 - 統計情報表示追加
 ****************************************/

#include "result.h"
#include "scene.h"
#include "score.h"
#include "combo.h"
#include "texture.h"
#include "sprite.h"
#include "direct3d.h"
#include "key_logger.h"
#include "pad_logger.h"
#include "input_manager.h"
#include "fade.h"
#include "sound_manager.h"
#ifdef USE_ASSET_DLL
#include "asset_dll.h"
#include "resource.h"
#endif

#include <DirectXMath.h>
#include <cmath>
#include <algorithm>
#include <cstdio>

using namespace DirectX;

// 定数
namespace
{
    constexpr const wchar_t* RESULT_BG_TEXTURE_PATH = L"assets/ui/result_bg.png";
    constexpr const wchar_t* RESULT_TITLE_TEXTURE_PATH = L"assets/ui/result_title.png";
    constexpr const wchar_t* PRESS_START_TEXTURE_PATH = L"assets/ui/press_start.png";
    constexpr const wchar_t* WHITE_TEXTURE_PATH = L"assets/white.png";
    constexpr const wchar_t* FONT_TEXTURE_PATH = L"assets/consolab_ascii_512.png";

    // フォントアトラス設定（16x16グリッド, ASCII 32〜）
    constexpr int FONT_GRID = 16;
    constexpr float FONT_CELL = 32.0f;   // 512 / 16

    // スコア数字フォント
    constexpr int FONT_SRC_WIDTH = 89;
    constexpr int FONT_SRC_HEIGHT = 123;
    constexpr float DRAW_SCALE = 0.25f;
    constexpr int SCORE_DIGITS = 7;

    constexpr float SCORE_SIZE_MAX = 2.5f;
    constexpr float SCORE_COUNT_DURATION = 3.0f;  // スコア加算演出の所要時間（秒）

    constexpr float BLINK_SPEED = 3.0f;

    // レイアウト定数
    constexpr float TITLE_Y_RATIO = 0.08f;
    constexpr float PANEL_Y_RATIO = 0.25f;
    constexpr float PANEL_HEIGHT_RATIO = 0.52f;
    constexpr float PRESS_ENTER_Y_RATIO = 0.83f;
    constexpr float HINT_Y_RATIO = 0.90f;

    // 統計項目のフォントサイズ
    constexpr float LABEL_CHAR_W = 20.0f;
    constexpr float LABEL_CHAR_H = 28.0f;
    constexpr float VALUE_CHAR_W = 24.0f;
    constexpr float VALUE_CHAR_H = 32.0f;
    constexpr float HINT_CHAR_W = 16.0f;
    constexpr float HINT_CHAR_H = 22.0f;

    // 統計項目の表示タイミング
    constexpr float STAT_SCORE_DELAY = 0.3f;
    constexpr float STAT_DEFEATS_DELAY = 0.8f;
    constexpr float STAT_COMBO_DELAY = 1.3f;
    constexpr float STAT_ALL_DONE_DELAY = 1.8f;

    // スコアSE再生間隔
    constexpr float SCORE_SOUND_INTERVAL = 0.05f;

    // ASCII文字範囲
    constexpr int ASCII_PRINTABLE_MIN = 32;            // 印字可能ASCII最小値（スペース）
    constexpr int ASCII_PRINTABLE_MAX = 126;           // 印字可能ASCII最大値（チルダ）

    // レイアウト: マージン・インセット
    constexpr float STAT_ROW_MARGIN = 40.0f;           // 統計行の左右マージン
    constexpr float PANEL_INSET = 30.0f;               // パネル内側の余白
    constexpr float SEPARATOR_HEIGHT = 2.0f;           // 区切り線の高さ

    // パネル幅比率
    constexpr float PANEL_WIDTH_RATIO = 0.6f;          // パネル幅の画面比率

    // パネル背景色
    constexpr float PANEL_BG_R = 0.05f;                // パネル背景のRed成分
    constexpr float PANEL_BG_G = 0.05f;                // パネル背景のGreen成分
    constexpr float PANEL_BG_B = 0.12f;                // パネル背景のBlue成分
    constexpr float PANEL_BG_A = 0.7f;                 // パネル背景のAlpha成分

    // アクセントライン色
    constexpr float ACCENT_COLOR_R = 0.3f;             // アクセントのRed成分
    constexpr float ACCENT_COLOR_G = 0.5f;             // アクセントのGreen成分
    constexpr float ACCENT_COLOR_B = 1.0f;             // アクセントのBlue成分（修正: 0.8f→1.0fは原コード通り）

    // アクセントライン高さ
    constexpr float ACCENT_LINE_HEIGHT = 3.0f;         // パネル上部装飾ラインの高さ

    // パネル内レイアウト
    constexpr float PANEL_TOP_PADDING = 30.0f;         // パネル上部のパディング
    constexpr float ROW_SPACING_RATIO = 0.22f;         // 行間隔のパネル高さ比率

    // 統計フェード
    constexpr float STAT_FADE_DURATION = 0.3f;         // 統計項目のフェードイン時間

    // スコアラベル色
    constexpr float SCORE_LABEL_R = 0.6f;              // スコアラベルのRed成分
    constexpr float SCORE_LABEL_G = 0.7f;              // スコアラベルのGreen成分
    constexpr float SCORE_LABEL_B = 0.9f;              // スコアラベルのBlue成分

    // スコア数字Y方向オフセット
    constexpr float SCORE_NUM_Y_OFFSET = 10.0f;        // スコア数字のY方向オフセット

    // 統計行Y方向パディング
    constexpr float STATS_Y_PADDING = 25.0f;           // 統計行のY方向パディング

    // 点滅・ヒント表示
    constexpr float BLINK_MIN_ALPHA = 0.3f;            // 点滅の最小アルファ値
    constexpr float BLINK_ALPHA_RANGE = 0.7f;          // 点滅のアルファ変動幅
    constexpr float HINT_ALPHA = 0.5f;                 // ヒントテキストのアルファ値

    // 区切り線の色
    constexpr float SEPARATOR_R = 0.4f;                // 区切り線のRed成分
    constexpr float SEPARATOR_G = 0.45f;               // 区切り線のGreen成分
    constexpr float SEPARATOR_B = 0.6f;                // 区切り線のBlue成分
    constexpr float SEPARATOR_BASE_A = 0.4f;           // 区切り線の基本アルファ値

    // ヒント色
    constexpr float HINT_COLOR_RG = 0.6f;              // ヒントテキストのRed/Green成分
    constexpr float HINT_COLOR_B = 0.7f;               // ヒントテキストのBlue成分

    // 区切り線のY方向オフセット
    constexpr float SEPARATOR_Y_OFFSET = 10.0f;        // 区切り線のY方向オフセット
}

// 内部変数
static int g_ResultBGTexId = -1;
static int g_ResultTitleTexId = -1;
static int g_PressEnterTexId = -1;
static int g_WhiteTexId = -1;
static int g_FontTexId = -1;

static float g_ScreenWidth = 0.0f;
static float g_ScreenHeight = 0.0f;

static float g_ScoreScale = 0.0f;
static float g_BlinkTimer = 0.0f;
static float g_Timer = 0.0f;
static bool g_CanPressEnter = false;

static float g_ScoreSoundTimer = 0.0f;
static bool g_ScoreFinishSoundPlayed = false;

// リザルト用保存データ
static int g_ResultMaxCombo = 0;
static int g_ResultDefeats = 0;

// フォントアトラスによるテキスト描画
static void DrawFontText(const char* text, float x, float y, float charW, float charH, const XMFLOAT4& color)
{
    if (g_FontTexId < 0 || !text) return;

    float curX = x;
    for (int i = 0; text[i] != '\0'; i++)
    {
        int ch = static_cast<unsigned char>(text[i]);
        if (ch < ASCII_PRINTABLE_MIN || ch > ASCII_PRINTABLE_MAX)
        {
            curX += charW;
            continue;
        }

        int index = ch - ASCII_PRINTABLE_MIN;
        int col = index % FONT_GRID;
        int row = index / FONT_GRID;

        float uvX = col * FONT_CELL;
        float uvY = row * FONT_CELL;

        Sprite_Draw(
            g_FontTexId,
            curX, y,
            uvX, uvY, FONT_CELL, FONT_CELL,
            charW, charH,
            0.0f,
            color
        );

        curX += charW;
    }
}

// テキスト幅を計算
static float GetTextWidth(const char* text, float charW)
{
    int len = 0;
    for (int i = 0; text[i] != '\0'; i++) len++;
    return len * charW;
}

// 中央揃えでテキスト描画
static void DrawFontTextCentered(const char* text, float y, float charW, float charH, const XMFLOAT4& color)
{
    float textW = GetTextWidth(text, charW);
    float x = (g_ScreenWidth - textW) * 0.5f;
    DrawFontText(text, x, y, charW, charH, color);
}

// 統計行描画（ラベル左寄せ、値右寄せ）
static void DrawStatRow(const char* label, const char* value, float panelX, float panelW, float y, float alpha)
{
    float labelX = panelX + STAT_ROW_MARGIN;
    XMFLOAT4 labelColor = { 0.8f, 0.8f, 0.85f, alpha };
    DrawFontText(label, labelX, y, LABEL_CHAR_W, LABEL_CHAR_H, labelColor);

    float valueW = GetTextWidth(value, VALUE_CHAR_W);
    float valueX = panelX + panelW - STAT_ROW_MARGIN - valueW;
    XMFLOAT4 valueColor = { 1.0f, 1.0f, 1.0f, alpha };
    DrawFontText(value, valueX, y, VALUE_CHAR_W, VALUE_CHAR_H, valueColor);
}

// 区切り線描画
static void DrawSeparator(float panelX, float panelW, float y, float alpha)
{
    if (g_WhiteTexId < 0) return;
    XMFLOAT4 lineColor = { SEPARATOR_R, SEPARATOR_G, SEPARATOR_B, SEPARATOR_BASE_A * alpha };
    Sprite_Draw(g_WhiteTexId, panelX + PANEL_INSET, y,
        0.0f, 0.0f, 1.0f, 1.0f,
        panelW - PANEL_INSET * 2.0f, SEPARATOR_HEIGHT,
        0.0f, lineColor);
}

// 初期化
void Result_Initialize()
{
    Fade_Start(1.0, false);

    g_ScreenWidth = static_cast<float>(Direct3D_GetBackBufferWidth());
    g_ScreenHeight = static_cast<float>(Direct3D_GetBackBufferHeight());

#ifdef USE_ASSET_DLL
    {
        const void* pData = nullptr;
        size_t dataSize = 0;
        if (AssetDLL_GetData(IDR_TEX_UI_RESULT_BG, &pData, &dataSize))
            g_ResultBGTexId = Texture_LoadFromMemory(pData, dataSize, L"result_bg");
        if (AssetDLL_GetData(IDR_TEX_UI_RESULT_TITLE, &pData, &dataSize))
            g_ResultTitleTexId = Texture_LoadFromMemory(pData, dataSize, L"result_title");
        if (AssetDLL_GetData(IDR_TEX_UI_PRESS_START, &pData, &dataSize))
            g_PressEnterTexId = Texture_LoadFromMemory(pData, dataSize, L"result_press_start");
        if (AssetDLL_GetData(IDR_TEX_WHITE, &pData, &dataSize))
            g_WhiteTexId = Texture_LoadFromMemory(pData, dataSize, L"result_white");
        if (AssetDLL_GetData(IDR_TEX_CONSOLAB, &pData, &dataSize))
            g_FontTexId = Texture_LoadFromMemory(pData, dataSize, L"result_font");
    }
#else
    g_ResultBGTexId = Texture_Load(RESULT_BG_TEXTURE_PATH);
    g_ResultTitleTexId = Texture_Load(RESULT_TITLE_TEXTURE_PATH);
    g_PressEnterTexId = Texture_Load(PRESS_START_TEXTURE_PATH);
    g_WhiteTexId = Texture_Load(WHITE_TEXTURE_PATH);
    g_FontTexId = Texture_Load(FONT_TEXTURE_PATH);
#endif

    Score_InitializeForResult(0.0f, 0.0f, SCORE_DIGITS);

    // 統計データを保存
    g_ResultMaxCombo = Combo_GetMaxCombo();
    g_ResultDefeats = Combo_GetTotalDefeats();

    g_ScoreScale = 0.0f;
    g_BlinkTimer = 0.0f;
    g_Timer = 0.0f;
    g_CanPressEnter = false;
    g_ScoreSoundTimer = 0.0f;
    g_ScoreFinishSoundPlayed = false;

    // リザルトBGM再生
    SoundManager_PlayBGM(SOUND_BGM_RESULT);
}

// 終了処理
void Result_Finalize()
{
    Score_Finalize();
    g_ResultBGTexId = -1;
    g_ResultTitleTexId = -1;
    g_PressEnterTexId = -1;
    g_WhiteTexId = -1;
    g_FontTexId = -1;
}

// 更新
void Result_Update(double elapsed_time)
{
    float dt = static_cast<float>(elapsed_time);

    g_Timer += dt;

    // スコア表示のスケールアップ（N秒で完了）
    if (g_ScoreScale < SCORE_SIZE_MAX)
    {
        g_ScoreScale += (SCORE_SIZE_MAX / SCORE_COUNT_DURATION) * dt;

        // スコアカウントアップ中のSE
        g_ScoreSoundTimer += dt;
        if (g_ScoreSoundTimer >= SCORE_SOUND_INTERVAL)
        {
            SoundManager_PlaySE(SOUND_SE_SCORE_COUNT);
            g_ScoreSoundTimer = 0.0f;
        }

        if (g_ScoreScale >= SCORE_SIZE_MAX)
        {
            g_ScoreScale = SCORE_SIZE_MAX;

            // スコア表示完了SE
            if (!g_ScoreFinishSoundPlayed)
            {
                SoundManager_PlaySE(SOUND_SE_SCORE_FINISH);
                g_ScoreFinishSoundPlayed = true;
            }
        }
    }

    // 全統計表示後に入力受付
    if (g_Timer >= STAT_ALL_DONE_DELAY && g_ScoreScale >= SCORE_SIZE_MAX)
    {
        g_CanPressEnter = true;
    }

    g_BlinkTimer += dt * BLINK_SPEED;

    Score_Update(elapsed_time);

    bool confirmInput = KeyLogger_IsTrigger(KK_ENTER) ||
        PadLogger_IsTrigger(0, XINPUT_GAMEPAD_START) ||
        PadLogger_IsTrigger(0, XINPUT_GAMEPAD_A) ||
        InputManager_IsMouseLeftTrigger();

    if (g_CanPressEnter && confirmInput)
    {
        // 決定SE
        SoundManager_PlaySE(SOUND_SE_DECIDE);
        Fade_Start(1.0, true);
    }

    if (Fade_GetState() == FADE_STATE::FINISHED_OUT)
    {
        Scene_Change(Scene::TITLE);
    }
}

// 描画
void Result_Draw()
{
    Direct3D_DepthStencilStateDepthIsEnable(false);
    Direct3D_SetBlendState(BlendMode::Alpha);

    Sprite_Begin();

    // 背景
    if (g_ResultBGTexId >= 0)
    {
        Sprite_Draw(g_ResultBGTexId, 0.0f, 0.0f, g_ScreenWidth, g_ScreenHeight);
    }

    // タイトル ("RESULT")
    if (g_ResultTitleTexId >= 0)
    {
        float titleW = static_cast<float>(Texture_Width(g_ResultTitleTexId));
        float titleX = (g_ScreenWidth - titleW) * 0.5f;
        float titleY = g_ScreenHeight * TITLE_Y_RATIO;

        Sprite_Draw(g_ResultTitleTexId, titleX, titleY);
    }

    // --- 統計パネル背景 ---
    float panelW = g_ScreenWidth * PANEL_WIDTH_RATIO;
    float panelX = (g_ScreenWidth - panelW) * 0.5f;
    float panelY = g_ScreenHeight * PANEL_Y_RATIO;
    float panelH = g_ScreenHeight * PANEL_HEIGHT_RATIO;

    if (g_WhiteTexId >= 0)
    {
        XMFLOAT4 panelColor = { PANEL_BG_R, PANEL_BG_G, PANEL_BG_B, PANEL_BG_A };
        Sprite_Draw(g_WhiteTexId, panelX, panelY,
            0.0f, 0.0f, 1.0f, 1.0f,
            panelW, panelH,
            0.0f, panelColor);

        // パネル上部の装飾ライン
        XMFLOAT4 accentColor = { ACCENT_COLOR_R, ACCENT_COLOR_G, ACCENT_COLOR_B, 0.8f };
        Sprite_Draw(g_WhiteTexId, panelX, panelY,
            0.0f, 0.0f, 1.0f, 1.0f,
            panelW, ACCENT_LINE_HEIGHT,
            0.0f, accentColor);
    }

    // --- 統計項目 ---
    float rowStartY = panelY + PANEL_TOP_PADDING;
    float rowSpacing = panelH * ROW_SPACING_RATIO;

    // 1. SCORE（数字はScore_Drawを使用）
    {
        float statAlpha = std::clamp((g_Timer - STAT_SCORE_DELAY) / STAT_FADE_DURATION, 0.0f, 1.0f);
        if (statAlpha > 0.0f)
        {
            float labelX = panelX + STAT_ROW_MARGIN;
            XMFLOAT4 labelColor = { SCORE_LABEL_R, SCORE_LABEL_G, SCORE_LABEL_B, statAlpha };
            DrawFontText("SCORE", labelX, rowStartY, LABEL_CHAR_W, LABEL_CHAR_H, labelColor);

            // スコア数字、Score_Draw使用、パネル内中央
            if (g_ScoreScale > 0.0f)
            {
                float scoreCharWidth = FONT_SRC_WIDTH * DRAW_SCALE * g_ScoreScale;
                float scoreTotalWidth = scoreCharWidth * SCORE_DIGITS;

                float scoreX = panelX + panelW - STAT_ROW_MARGIN - scoreTotalWidth;
                float scoreY = rowStartY + LABEL_CHAR_H + SCORE_NUM_Y_OFFSET;

                Score_Draw(scoreX, scoreY, g_ScoreScale, g_ScoreScale);
            }
        }
    }

    float statsRowY = rowStartY + LABEL_CHAR_H + FONT_SRC_HEIGHT * DRAW_SCALE * SCORE_SIZE_MAX + STATS_Y_PADDING;

    // 区切り線
    {
        float sepAlpha = std::clamp((g_Timer - STAT_DEFEATS_DELAY) / STAT_FADE_DURATION, 0.0f, 1.0f);
        if (sepAlpha > 0.0f)
        {
            DrawSeparator(panelX, panelW, statsRowY - SEPARATOR_Y_OFFSET, sepAlpha);
        }
    }

    // 2. DEFEATS
    {
        float statAlpha = std::clamp((g_Timer - STAT_DEFEATS_DELAY) / STAT_FADE_DURATION, 0.0f, 1.0f);
        if (statAlpha > 0.0f)
        {
            char defeatStr[16];
            snprintf(defeatStr, sizeof(defeatStr), "%d", g_ResultDefeats);
            DrawStatRow("DEFEATS", defeatStr, panelX, panelW, statsRowY, statAlpha);
        }
    }

    // 区切り線
    {
        float sepAlpha = std::clamp((g_Timer - STAT_COMBO_DELAY) / STAT_FADE_DURATION, 0.0f, 1.0f);
        if (sepAlpha > 0.0f)
        {
            DrawSeparator(panelX, panelW, statsRowY + rowSpacing - SEPARATOR_Y_OFFSET, sepAlpha);
        }
    }

    // 3. MAX COMBO
    {
        float statAlpha = std::clamp((g_Timer - STAT_COMBO_DELAY) / STAT_FADE_DURATION, 0.0f, 1.0f);
        if (statAlpha > 0.0f)
        {
            char comboStr[16];
            snprintf(comboStr, sizeof(comboStr), "%d", g_ResultMaxCombo);
            DrawStatRow("MAX COMBO", comboStr, panelX, panelW, statsRowY + rowSpacing, statAlpha);
        }
    }

    // Press Enter
    if (g_CanPressEnter && g_PressEnterTexId >= 0)
    {
        float pressW = static_cast<float>(Texture_Width(g_PressEnterTexId));
        float pressX = (g_ScreenWidth - pressW) * 0.5f;
        float pressY = g_ScreenHeight * PRESS_ENTER_Y_RATIO;

        float alpha = (sinf(g_BlinkTimer) + 1.0f) * 0.5f;
        alpha = BLINK_MIN_ALPHA + alpha * BLINK_ALPHA_RANGE;
        XMFLOAT4 color = { 1.0f, 1.0f, 1.0f, alpha };
        Sprite_Draw(g_PressEnterTexId, pressX, pressY, 0.0f, color);
    }

    // ナビゲーションヒント
    if (g_CanPressEnter && g_FontTexId >= 0)
    {
        float hintAlpha = HINT_ALPHA;
        XMFLOAT4 hintColor = { HINT_COLOR_RG, HINT_COLOR_RG, HINT_COLOR_B, hintAlpha };
        DrawFontTextCentered("[Enter] Back to Title", g_ScreenHeight * HINT_Y_RATIO, HINT_CHAR_W, HINT_CHAR_H, hintColor);
    }

    Direct3D_DepthStencilStateDepthIsEnable(true);
}

// シャドウ描画（空実装）
void Result_DrawShadow()
{
}