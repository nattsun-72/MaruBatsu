/****************************************
 * @file title.cpp
 * @brief タイトル画面「斬空 -ZANKUU-」の実装
 * @author Natsume Shidara
 * @date 2026/01/12
 * @update 2026/01/13 - サウンド対応
 *
 * 演出フロー:
 * 1. 背景フェードイン (0.0s〜1.0s)
 * 2. 斬撃エフェクト (1.0s〜1.8s)
 * 3. ロゴ登場 (1.3s〜2.3s)
 * 4. Press Enter点滅 (2.5s〜)
 ****************************************/

#include "title.h"
#include "scene.h"
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

using namespace DirectX;

// 定数
namespace
{
    constexpr const wchar_t* BG_TEXTURE_PATH = L"assets/ui/title_bg_only.png";
    constexpr const wchar_t* LOGO_TEXTURE_PATH = L"assets/ui/title_logo.png";
    constexpr const wchar_t* PRESS_START_TEXTURE_PATH = L"assets/ui/press_start.png";
    constexpr const wchar_t* WHITE_TEXTURE_PATH = L"assets/white.png";
    constexpr const wchar_t* FONT_TEXTURE_PATH = L"assets/consolab_ascii_512.png";

    // ASCII印字可能範囲
    constexpr int ASCII_PRINTABLE_MIN = 32;    // 印字可能文字の最小値（スペース）
    constexpr int ASCII_PRINTABLE_MAX = 126;   // 印字可能文字の最大値（チルダ）

    // フォントアトラス設定 (16x16グリッド, ASCII 32〜)
    constexpr int FONT_GRID = 16;
    constexpr float FONT_CELL = 32.0f;   // 512 / 16
    constexpr float HINT_CHAR_W = 18.0f;  // 描画サイズ
    constexpr float HINT_CHAR_H = 24.0f;

    constexpr float TIME_BG_FADE_START = 0.0f;
    constexpr float TIME_BG_FADE_END = 1.0f;
    constexpr float TIME_SLASH_START = 1.0f;
    constexpr float TIME_SLASH_END = 1.8f;
    constexpr float TIME_LOGO_START = 1.3f;
    constexpr float TIME_LOGO_END = 2.3f;
    constexpr float TIME_PRESS_ENTER_START = 2.5f;

    constexpr float LOGO_SCALE_START = 1.5f;
    constexpr float LOGO_SCALE_END = 1.0f;

    constexpr float BLINK_SPEED = 3.0f;

    constexpr int SLASH_COUNT = 3;
    constexpr float SLASH_DURATION = 0.15f;

    // 斬撃エフェクト パラメータ
    constexpr float SLASH_OFFSCREEN_OFFSET = 200.0f;     // 画面外オフセット（ピクセル）
    constexpr float SLASH_0_ANGLE_DEG = -10.0f;           // 斬撃0の角度（度）
    constexpr float SLASH_0_WIDTH = 8.0f;                  // 斬撃0の幅
    constexpr float SLASH_0_LENGTH = 400.0f;               // 斬撃0の長さ
    constexpr float SLASH_0_START_Y_RATIO = 0.2f;         // 斬撃0の開始Y位置（画面比率）
    constexpr float SLASH_0_END_Y_RATIO = 0.4f;           // 斬撃0の終了Y位置（画面比率）

    constexpr float SLASH_1_TIME_OFFSET = 0.1f;           // 斬撃1の時間オフセット
    constexpr float SLASH_1_ANGLE_DEG = 15.0f;            // 斬撃1の角度（度）
    constexpr float SLASH_1_WIDTH = 6.0f;                  // 斬撃1の幅
    constexpr float SLASH_1_LENGTH = 350.0f;               // 斬撃1の長さ
    constexpr float SLASH_1_START_Y_RATIO = 0.3f;         // 斬撃1の開始Y位置（画面比率）
    constexpr float SLASH_1_END_Y_RATIO = 0.6f;           // 斬撃1の終了Y位置（画面比率）

    constexpr float SLASH_2_TIME_OFFSET = 0.2f;           // 斬撃2の時間オフセット
    constexpr float SLASH_2_ANGLE_DEG = -2.0f;            // 斬撃2の角度（度）
    constexpr float SLASH_2_WIDTH = 10.0f;                 // 斬撃2の幅
    constexpr float SLASH_2_LENGTH = 500.0f;               // 斬撃2の長さ
    constexpr float SLASH_2_START_Y_RATIO = 0.5f;         // 斬撃2の開始Y位置（画面比率）
    constexpr float SLASH_2_END_Y_RATIO = 0.48f;          // 斬撃2の終了Y位置（画面比率）

    constexpr float DEG_TO_RAD = 3.14159f / 180.0f;       // 度からラジアン変換係数

    // 斬撃グロー（発光）パラメータ
    constexpr int SLASH_GLOW_LAYERS = 3;                   // グローレイヤー最大インデックス
    constexpr float SLASH_GLOW_BASE_ALPHA = 0.3f;         // グロー基本アルファ値
    constexpr float SLASH_GLOW_ALPHA_STEP = 0.08f;        // グローレイヤーごとのアルファ減少量
    constexpr float SLASH_GLOW_WIDTH_STEP = 8.0f;         // グローレイヤーごとの幅増加量
    constexpr float SLASH_GLOW_COLOR_R = 0.7f;            // グロー色R
    constexpr float SLASH_GLOW_COLOR_G = 0.85f;           // グロー色G
    constexpr float SLASH_GLOW_COLOR_B = 1.0f;            // グロー色B
    constexpr float SLASH_CORE_WIDTH_RATIO = 0.5f;        // コア幅比率（slash.widthに対する）
    constexpr float SLASH_CORE_OFFSET_RATIO = 0.25f;      // コアY中心オフセット比率

    // フェード演出時間
    constexpr double FADE_DURATION_GAME_START = 1.0;       // ゲーム開始時フェード（秒）
    constexpr double FADE_DURATION_SETTINGS = 0.5;         // 設定画面遷移フェード（秒）
    constexpr double FADE_DURATION_TITLE_IN = 0.5;         // タイトル画面フェードイン（秒）

    // Press Enter表示フェード時間
    constexpr float PRESS_ENTER_FADE_DURATION = 0.5f;     // Press Enter表示のフェードイン時間

    // ロゴ描画パラメータ
    constexpr float LOGO_Y_OFFSET = 50.0f;                // ロゴY座標オフセット（上方向）

    // Press Enter描画パラメータ
    constexpr float PRESS_ENTER_Y_RATIO = 0.8f;           // Press Enter Y位置（画面比率）
    constexpr float BLINK_ALPHA_MIN = 0.3f;               // 点滅最小アルファ値
    constexpr float BLINK_ALPHA_RANGE = 0.7f;             // 点滅アルファ変動幅

    // ヒントテキストパラメータ
    constexpr float HINT_ALPHA_MAX = 0.6f;                // ヒントテキスト最大アルファ値
    constexpr float HINT_Y_RATIO = 0.88f;                 // ヒントテキストY位置（画面比率）
    constexpr int HINT_TEXT_LENGTH = 10;                   // ヒントテキスト文字数
    constexpr float HINT_COLOR_R = 0.7f;                  // ヒントテキスト色R
    constexpr float HINT_COLOR_G = 0.7f;                  // ヒントテキスト色G
    constexpr float HINT_COLOR_B = 0.8f;                  // ヒントテキスト色B

    // 斬撃フェード閾値
    constexpr float SLASH_FADE_THRESHOLD = 0.5f;          // 斬撃アルファフェード開始進行度
    constexpr float SLASH_FADE_MULTIPLIER = 2.0f;         // 斬撃アルファフェード倍率
}

// 斬撃エフェクト構造体
struct SlashEffect
{
    float startTime;
    float startX, startY;
    float endX, endY;
    float angle;
    float width;
    float length;
    bool active;
    bool soundPlayed;  // SE再生済みフラグ
};

// 内部変数
static int g_BgTexId = -1;
static int g_LogoTexId = -1;
static int g_PressEnterTexId = -1;
static int g_WhiteTexId = -1;

static float g_ScreenWidth = 0.0f;
static float g_ScreenHeight = 0.0f;

static float g_Timer = 0.0f;
static float g_BlinkTimer = 0.0f;
static bool g_CanInput = false;
static bool g_IsTransitioning = false;
static Scene g_NextScene = Scene::GAME;  // 遷移先シーン

static SlashEffect g_Slashes[SLASH_COUNT];

// オプションヒント用フォントテクスチャ
static int g_FontTexId = -1;

// 内部関数プロトタイプ
static void InitSlashEffects();
static void UpdateSlashEffects(float dt);
static void DrawSlashEffects();
static void DrawText(const char* text, float x, float y, const XMFLOAT4& color);
static float EaseOutCubic(float t);
static float EaseOutBack(float t);

// イージング関数
static float EaseOutCubic(float t)
{
    return 1.0f - powf(1.0f - t, 3.0f);
}

static float EaseOutBack(float t)
{
    constexpr float c1 = 1.70158f;
    constexpr float c3 = c1 + 1.0f;
    return 1.0f + c3 * powf(t - 1.0f, 3.0f) + c1 * powf(t - 1.0f, 2.0f);
}

// フォントアトラスによるテキスト描画
static void DrawText(const char* text, float x, float y, const XMFLOAT4& color)
{
    if (g_FontTexId < 0 || !text) return;

    float curX = x;
    for (int i = 0; text[i] != '\0'; i++)
    {
        int ch = static_cast<unsigned char>(text[i]);
        if (ch < ASCII_PRINTABLE_MIN || ch > ASCII_PRINTABLE_MAX)
        {
            curX += HINT_CHAR_W;
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
            HINT_CHAR_W, HINT_CHAR_H,
            0.0f,
            color
        );

        curX += HINT_CHAR_W;
    }
}

// 斬撃エフェクト初期化
static void InitSlashEffects()
{
    g_Slashes[0].startTime = TIME_SLASH_START;
    g_Slashes[0].startX = -SLASH_OFFSCREEN_OFFSET;
    g_Slashes[0].startY = g_ScreenHeight * SLASH_0_START_Y_RATIO;
    g_Slashes[0].endX = g_ScreenWidth + SLASH_OFFSCREEN_OFFSET;
    g_Slashes[0].endY = g_ScreenHeight * SLASH_0_END_Y_RATIO;
    g_Slashes[0].angle = SLASH_0_ANGLE_DEG * DEG_TO_RAD;
    g_Slashes[0].width = SLASH_0_WIDTH;
    g_Slashes[0].length = SLASH_0_LENGTH;
    g_Slashes[0].active = true;
    g_Slashes[0].soundPlayed = false;

    g_Slashes[1].startTime = TIME_SLASH_START + SLASH_1_TIME_OFFSET;
    g_Slashes[1].startX = g_ScreenWidth + SLASH_OFFSCREEN_OFFSET;
    g_Slashes[1].startY = g_ScreenHeight * SLASH_1_START_Y_RATIO;
    g_Slashes[1].endX = -SLASH_OFFSCREEN_OFFSET;
    g_Slashes[1].endY = g_ScreenHeight * SLASH_1_END_Y_RATIO;
    g_Slashes[1].angle = SLASH_1_ANGLE_DEG * DEG_TO_RAD;
    g_Slashes[1].width = SLASH_1_WIDTH;
    g_Slashes[1].length = SLASH_1_LENGTH;
    g_Slashes[1].active = true;
    g_Slashes[1].soundPlayed = false;

    g_Slashes[2].startTime = TIME_SLASH_START + SLASH_2_TIME_OFFSET;
    g_Slashes[2].startX = -SLASH_OFFSCREEN_OFFSET;
    g_Slashes[2].startY = g_ScreenHeight * SLASH_2_START_Y_RATIO;
    g_Slashes[2].endX = g_ScreenWidth + SLASH_OFFSCREEN_OFFSET;
    g_Slashes[2].endY = g_ScreenHeight * SLASH_2_END_Y_RATIO;
    g_Slashes[2].angle = SLASH_2_ANGLE_DEG * DEG_TO_RAD;
    g_Slashes[2].width = SLASH_2_WIDTH;
    g_Slashes[2].length = SLASH_2_LENGTH;
    g_Slashes[2].active = true;
    g_Slashes[2].soundPlayed = false;
}

// 斬撃エフェクト更新（SE再生）
static void UpdateSlashEffects(float dt)
{
    (void)dt;

    for (int i = 0; i < SLASH_COUNT; i++)
    {
        SlashEffect& slash = g_Slashes[i];
        if (!slash.active) continue;

        // 斬撃開始タイミングでSE再生
        if (!slash.soundPlayed && g_Timer >= slash.startTime)
        {
            SoundManager_PlaySE(SOUND_SE_TITLE_SLASH);
            slash.soundPlayed = true;
        }
    }
}

// 斬撃エフェクト描画
static void DrawSlashEffects()
{
    if (g_WhiteTexId < 0) return;

    for (int i = 0; i < SLASH_COUNT; i++)
    {
        SlashEffect& slash = g_Slashes[i];
        if (!slash.active) continue;

        float elapsed = g_Timer - slash.startTime;
        if (elapsed < 0.0f || elapsed > SLASH_DURATION) continue;

        float progress = elapsed / SLASH_DURATION;
        float easedProgress = EaseOutCubic(progress);

        float currentX = slash.startX + (slash.endX - slash.startX) * easedProgress;
        float currentY = slash.startY + (slash.endY - slash.startY) * easedProgress;

        float alpha = 1.0f;
        if (progress > SLASH_FADE_THRESHOLD)
        {
            alpha = 1.0f - (progress - SLASH_FADE_THRESHOLD) * SLASH_FADE_MULTIPLIER;
        }

        for (int g = SLASH_GLOW_LAYERS; g >= 0; g--)
        {
            float glowAlpha = alpha * (SLASH_GLOW_BASE_ALPHA - g * SLASH_GLOW_ALPHA_STEP);
            float glowWidth = slash.width + g * SLASH_GLOW_WIDTH_STEP;

            XMFLOAT4 color = { SLASH_GLOW_COLOR_R, SLASH_GLOW_COLOR_G, SLASH_GLOW_COLOR_B, glowAlpha };

            Sprite_Draw(
                g_WhiteTexId,
                currentX - slash.length * 0.5f,
                currentY - glowWidth * 0.5f,
                0.0f, 0.0f, 1.0f, 1.0f,
                slash.length, glowWidth,
                slash.angle,
                color
            );
        }

        XMFLOAT4 coreColor = { 1.0f, 1.0f, 1.0f, alpha };
        Sprite_Draw(
            g_WhiteTexId,
            currentX - slash.length * 0.5f,
            currentY - slash.width * SLASH_CORE_OFFSET_RATIO,
            0.0f, 0.0f, 1.0f, 1.0f,
            slash.length, slash.width * SLASH_CORE_WIDTH_RATIO,
            slash.angle,
            coreColor
        );
    }
}

// 初期化
void Title_Initialize()
{
    g_ScreenWidth = static_cast<float>(Direct3D_GetBackBufferWidth());
    g_ScreenHeight = static_cast<float>(Direct3D_GetBackBufferHeight());

#ifdef USE_ASSET_DLL
    {
        const void* pData = nullptr;
        size_t dataSize = 0;
        if (AssetDLL_GetData(IDR_TEX_UI_TITLE_BG, &pData, &dataSize))
            g_BgTexId = Texture_LoadFromMemory(pData, dataSize, L"title_bg");
        if (AssetDLL_GetData(IDR_TEX_UI_TITLE_LOGO, &pData, &dataSize))
            g_LogoTexId = Texture_LoadFromMemory(pData, dataSize, L"title_logo");
        if (AssetDLL_GetData(IDR_TEX_UI_PRESS_START, &pData, &dataSize))
            g_PressEnterTexId = Texture_LoadFromMemory(pData, dataSize, L"title_press_start");
        if (AssetDLL_GetData(IDR_TEX_WHITE, &pData, &dataSize))
            g_WhiteTexId = Texture_LoadFromMemory(pData, dataSize, L"title_white");
        if (AssetDLL_GetData(IDR_TEX_CONSOLAB, &pData, &dataSize))
            g_FontTexId = Texture_LoadFromMemory(pData, dataSize, L"title_font");
    }
#else
    g_BgTexId = Texture_Load(BG_TEXTURE_PATH);
    g_LogoTexId = Texture_Load(LOGO_TEXTURE_PATH);
    g_PressEnterTexId = Texture_Load(PRESS_START_TEXTURE_PATH);
    g_WhiteTexId = Texture_Load(WHITE_TEXTURE_PATH);
    g_FontTexId = Texture_Load(FONT_TEXTURE_PATH);
#endif

    g_Timer = 0.0f;
    g_BlinkTimer = 0.0f;
    g_CanInput = false;
    g_IsTransitioning = false;
    g_NextScene = Scene::GAME;

    InitSlashEffects();

    // タイトルBGM再生
    SoundManager_PlayBGM(SOUND_BGM_TITLE);

    if (Fade_GetState() == FADE_STATE::FINISHED_OUT)
    {
        Fade_Start(FADE_DURATION_TITLE_IN, false);
    }
}

// 終了処理
void Title_Finalize()
{
    g_BgTexId = -1;
    g_LogoTexId = -1;
    g_PressEnterTexId = -1;
    g_WhiteTexId = -1;
    g_FontTexId = -1;
}

// 更新
void Title_Update(double elapsed_time)
{
    float dt = static_cast<float>(elapsed_time);

    g_Timer += dt;
    g_BlinkTimer += dt * BLINK_SPEED;

    // 斬撃エフェクト更新（SE再生）
    UpdateSlashEffects(dt);

    if (g_Timer >= TIME_PRESS_ENTER_START)
    {
        g_CanInput = true;
    }

    if (g_IsTransitioning)
    {
        if (Fade_GetState() == FADE_STATE::FINISHED_OUT)
        {
            Scene_Change(g_NextScene);
        }
        return;
    }

    // ゲーム開始入力
    bool startInput = KeyLogger_IsTrigger(KK_ENTER) ||
        PadLogger_IsTrigger(0, XINPUT_GAMEPAD_START) ||
        PadLogger_IsTrigger(0, XINPUT_GAMEPAD_A) ||
        InputManager_IsMouseLeftTrigger();

    // 設定画面入力
    bool settingsInput = KeyLogger_IsTrigger(KK_O) ||
        PadLogger_IsTrigger(0, XINPUT_GAMEPAD_BACK) ||
        PadLogger_IsTrigger(0, XINPUT_GAMEPAD_Y);

    if (g_CanInput && startInput)
    {
        // 決定SE
        SoundManager_PlaySE(SOUND_SE_DECIDE);
        g_NextScene = Scene::GAME;
        g_IsTransitioning = true;
        Fade_Start(FADE_DURATION_GAME_START, true);
    }
    else if (g_CanInput && settingsInput)
    {
        // 設定画面へ
        SoundManager_PlaySE(SOUND_SE_SELECT);
        g_NextScene = Scene::SETTINGS;
        g_IsTransitioning = true;
        Fade_Start(FADE_DURATION_SETTINGS, true);
    }
}

// 描画
void Title_Draw()
{
    Direct3D_DepthStencilStateDepthIsEnable(false);
    Direct3D_SetBlendState(BlendMode::Alpha);

    Sprite_Begin();

    // 背景
    if (g_BgTexId >= 0)
    {
        float bgAlpha = 1.0f;
        if (g_Timer < TIME_BG_FADE_END)
        {
            float progress = (g_Timer - TIME_BG_FADE_START) / (TIME_BG_FADE_END - TIME_BG_FADE_START);
            bgAlpha = std::clamp(progress, 0.0f, 1.0f);
        }

        XMFLOAT4 bgColor = { 1.0f, 1.0f, 1.0f, bgAlpha };
        Sprite_Draw(g_BgTexId, 0.0f, 0.0f, g_ScreenWidth, g_ScreenHeight, 0.0f, bgColor);
    }

    // 斬撃エフェクト
    DrawSlashEffects();

    // ロゴ
    if (g_LogoTexId >= 0 && g_Timer >= TIME_LOGO_START)
    {
        float logoW = static_cast<float>(Texture_Width(g_LogoTexId));
        float logoH = static_cast<float>(Texture_Height(g_LogoTexId));

        float logoProgress = (g_Timer - TIME_LOGO_START) / (TIME_LOGO_END - TIME_LOGO_START);
        logoProgress = std::clamp(logoProgress, 0.0f, 1.0f);

        float easedProgress = EaseOutBack(logoProgress);
        float scale = LOGO_SCALE_START + (LOGO_SCALE_END - LOGO_SCALE_START) * easedProgress;
        float alpha = EaseOutCubic(logoProgress);

        float drawW = logoW * scale;
        float drawH = logoH * scale;
        float logoX = (g_ScreenWidth - drawW) * 0.5f;
        float logoY = (g_ScreenHeight - drawH) * 0.5f - LOGO_Y_OFFSET;

        XMFLOAT4 logoColor = { 1.0f, 1.0f, 1.0f, alpha };
        Sprite_Draw(g_LogoTexId, logoX, logoY, drawW, drawH, 0.0f, logoColor);
    }

    // Press Enter
    if (g_PressEnterTexId >= 0 && g_CanInput)
    {
        float pressW = static_cast<float>(Texture_Width(g_PressEnterTexId));
        float pressX = (g_ScreenWidth - pressW) * 0.5f;
        float pressY = g_ScreenHeight * PRESS_ENTER_Y_RATIO;

        float blinkAlpha = (sinf(g_BlinkTimer) + 1.0f) * 0.5f;
        blinkAlpha = BLINK_ALPHA_MIN + blinkAlpha * BLINK_ALPHA_RANGE;

        float fadeProgress = (g_Timer - TIME_PRESS_ENTER_START) / PRESS_ENTER_FADE_DURATION;
        fadeProgress = std::clamp(fadeProgress, 0.0f, 1.0f);
        float alpha = blinkAlpha * fadeProgress;

        XMFLOAT4 pressColor = { 1.0f, 1.0f, 1.0f, alpha };
        Sprite_Draw(g_PressEnterTexId, pressX, pressY, 0.0f, pressColor);
    }

    // [O] Option ヒントテキスト
    if (g_FontTexId >= 0 && g_CanInput)
    {
        float fadeProgress = (g_Timer - TIME_PRESS_ENTER_START) / PRESS_ENTER_FADE_DURATION;
        fadeProgress = std::clamp(fadeProgress, 0.0f, 1.0f);
        float hintAlpha = HINT_ALPHA_MAX * fadeProgress;

        const char* hintStr = "[O] Option";
        int hintLen = HINT_TEXT_LENGTH;
        float textW = hintLen * HINT_CHAR_W;
        float hintX = (g_ScreenWidth - textW) * 0.5f;
        float hintY = g_ScreenHeight * HINT_Y_RATIO;

        XMFLOAT4 hintColor = { HINT_COLOR_R, HINT_COLOR_G, HINT_COLOR_B, hintAlpha };
        DrawText(hintStr, hintX, hintY, hintColor);
    }

    Direct3D_DepthStencilStateDepthIsEnable(true);
}

// シャドウ描画（空実装）
void Title_DrawShadow()
{
}