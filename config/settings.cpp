/****************************************
 * @file settings.cpp
 * @brief 設定画面の実装
 * @author Natsume Shidara
 * @date 2026/01/13
 * @update 2026/02/25
 *
 * 設定項目（マウス感度、音量、画面設定等）の
 * ユーザーインターフェースおよび制御ロジックを管理します。
 ****************************************/

 //--------------------------------------
 // インクルードガード／依存ヘッダ
 //--------------------------------------
#include "settings.h"
#include "scene.h"
#include "texture.h"
#include "sprite.h"
#include "direct3d.h"
#include "key_logger.h"
#include "pad_logger.h"
#include "mouse.h"
#include "game_window.h"
#include "fade.h"
#include "game_settings.h"
#include "sound_manager.h"
#include "debug_text.h"

#include <DirectXMath.h>
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <sstream>
#include <string>

#ifdef USE_ASSET_DLL
#include "asset_dll.h"
#include "resource.h"
#endif

using namespace DirectX;

// 定数定義（無名名前空間）
namespace
{
    //--------------------------------------
    // リソースパス
    //--------------------------------------
    constexpr const wchar_t* TEX_BG = L"assets/ui/settings_bg.png";
    constexpr const wchar_t* TEX_WHITE = L"assets/white.png";
    constexpr const wchar_t* TEX_TITLE = L"assets/ui/settings_title.png";
    constexpr const wchar_t* FONT_PATH = L"assets/consolab_ascii_512.png";

    //--------------------------------------
    // メニュー項目定義
    //--------------------------------------
    enum MenuItem
    {
        MENU_SENSITIVITY,
        MENU_BGM_VOLUME,
        MENU_SE_VOLUME,
        MENU_BORDERLESS,
        MENU_VSYNC,
        MENU_RESET,
        MENU_BACK,
        MENU_COUNT
    };

    const char* MENU_LABELS[MENU_COUNT] = {
        "Mouse Sensitivity",
        "BGM Volume",
        "SE Volume",
        "Borderless Window",
        "VSync",
        "Reset to Default",
        "Back to Title"
    };

    //--------------------------------------
    // レイアウト・アニメーション設定
    //--------------------------------------
    constexpr float TITLE_Y = 50.0f;
    constexpr float MENU_START_Y = 180.0f;
    constexpr float MENU_SPACING = 64.0f;
    constexpr float LABEL_X = 100.0f;
    constexpr float SLIDER_X = 500.0f;
    constexpr float SLIDER_WIDTH = 300.0f;
    constexpr float SLIDER_HEIGHT = 20.0f;
    constexpr float KNOB_WIDTH = 16.0f;
    constexpr float KNOB_HEIGHT = 30.0f;
    constexpr float BUTTON_WIDTH = 180.0f;
    constexpr float BUTTON_HEIGHT = 40.0f;
    constexpr float TOGGLE_WIDTH = 80.0f;
    constexpr float TOGGLE_HEIGHT = 32.0f;
    constexpr float CURSOR_HEIGHT = 50.0f;

    constexpr float ADJUST_SPEED = 0.015f;
    constexpr float BLINK_SPEED = 5.0f;
    constexpr float CURSOR_SPEED = 3.0f;

    //--------------------------------------
    // 入力リピート設定
    //--------------------------------------
    constexpr float REPEAT_DELAY = 0.25f;
    constexpr float REPEAT_INTERVAL = 0.03f;
}

// 内部変数（静的変数群）
static int g_TexBg = -1;
static int g_TexWhite = -1;
static int g_TexTitle = -1;

static float g_ScreenWidth = 0.0f;
static float g_ScreenHeight = 0.0f;

static int   g_SelectedItem = 0;
static float g_AnimTimer = 0.0f;
static bool  g_IsTransitioning = false;

static float g_InputRepeatTimer = 0.0f;
static bool  g_FirstInput = true;
static bool  g_IsDraggingSlider = false;

static hal::DebugText* g_pDebugText = nullptr;

// 内部関数プロトタイプ宣言
static void  DrawSlider(float x, float y, float value, float minVal, float maxVal, bool selected);
static void  DrawButton(float x, float y, const char* text, bool selected);
static void  DrawToggle(float x, float y, bool value, bool selected);
static float GetCurrentValue(int item);
static void  SetCurrentValue(int item, float value);
static void  AdjustValue(int item, float delta);
static bool  GetCurrentToggle(int item);
static void  ToggleCurrentValue(int item);
static bool  IsSliderItem(int item) { return item <= MENU_SE_VOLUME; }
static bool  IsToggleItem(int item) { return item == MENU_BORDERLESS || item == MENU_VSYNC; }

/****************************************
 * 基本動作関数群の実装
 ****************************************/

 /**
  * @brief 設定画面の初期化
  * @detail リソースの読み込み、表示設定、フェード開始を行います。
  */
void Settings_Initialize()
{
    /*--------------------------------------
     * 画面解像度の取得
     *------------------------------------*/
    g_ScreenWidth = static_cast<float>(Direct3D_GetBackBufferWidth());
    g_ScreenHeight = static_cast<float>(Direct3D_GetBackBufferHeight());

    /*--------------------------------------
     * テクスチャ・フォントのロード
     *------------------------------------*/
#ifdef USE_ASSET_DLL
    {
        const void* pData = nullptr;
        size_t dataSize = 0;
        if (AssetDLL_GetData(IDR_TEX_UI_SETTINGS_BG, &pData, &dataSize))
            g_TexBg = Texture_LoadFromMemory(pData, dataSize, L"settings_bg");
        if (AssetDLL_GetData(IDR_TEX_WHITE, &pData, &dataSize))
            g_TexWhite = Texture_LoadFromMemory(pData, dataSize, L"settings_white");
        if (AssetDLL_GetData(IDR_TEX_UI_SETTINGS_TITLE, &pData, &dataSize))
            g_TexTitle = Texture_LoadFromMemory(pData, dataSize, L"settings_title");
    }
#else
    g_TexBg = Texture_Load(TEX_BG);
    g_TexWhite = Texture_Load(TEX_WHITE);
    g_TexTitle = Texture_Load(TEX_TITLE);
#endif

    /*--------------------------------------
     * デバッグテキスト（UI文字列用）の生成
     *------------------------------------*/
    g_pDebugText = new hal::DebugText(
        Direct3D_GetDevice(),
        Direct3D_GetContext(),
        FONT_PATH,
        static_cast<int>(g_ScreenWidth),
        static_cast<int>(g_ScreenHeight),
        0.0f, 0.0f, 0, 0, 0.0f, 24.0f
    );

    /*--------------------------------------
     * 内部状態の初期化
     *------------------------------------*/
    g_SelectedItem = 0;
    g_AnimTimer = 0.0f;
    g_IsTransitioning = false;
    g_InputRepeatTimer = 0.0f;
    g_FirstInput = true;
    g_IsDraggingSlider = false;
    

    /*--------------------------------------
     * マウス設定およびフェード開始
     *------------------------------------*/
    Mouse_SetMode(MOUSE_POSITION_MODE_ABSOLUTE);
    Mouse_SetVisible(true);

    if (Fade_GetState() == FADE_STATE::FINISHED_OUT)
    {
        Fade_Start(0.5, false);
    }
}

/**
 * @brief 設定画面の終了処理
 * @detail リソースの解放とマウス状態の復帰を行います。
 */
void Settings_Finalize()
{
    Mouse_SetVisible(false);
    Mouse_SetMode(MOUSE_POSITION_MODE_RELATIVE);

    if (g_pDebugText)
    {
        delete g_pDebugText;
        g_pDebugText = nullptr;
    }

    g_TexBg = -1;
    g_TexWhite = -1;
    g_TexTitle = -1;
}

/**
 * @brief 設定画面の更新
 * @param elapsed_time 前フレームからの経過時間
 */
void Settings_Update(double elapsed_time)
{
    // マウスカーソルが非表示なら強制的に表示する
    CURSORINFO ci = { sizeof(CURSORINFO) };
    if (GetCursorInfo(&ci) && !(ci.flags & CURSOR_SHOWING))
    {
        Mouse_SetVisible(true);
    }


    float dt = static_cast<float>(elapsed_time);
    g_AnimTimer += dt;

    /*--------------------------------------
     * シーン遷移中の処理
     *------------------------------------*/
    if (g_IsTransitioning)
    {
        if (Fade_GetState() == FADE_STATE::FINISHED_OUT)
        {
            Scene_Change(Scene::TITLE);
        }
        return;
    }

    /*--------------------------------------
     * 上下移動入力（キーボード・パッド）
     *------------------------------------*/
    bool upInput = KeyLogger_IsTrigger(KK_W) || KeyLogger_IsTrigger(KK_UP) ||
        PadLogger_IsTrigger(0, XINPUT_GAMEPAD_DPAD_UP);
    bool downInput = KeyLogger_IsTrigger(KK_S) || KeyLogger_IsTrigger(KK_DOWN) ||
        PadLogger_IsTrigger(0, XINPUT_GAMEPAD_DPAD_DOWN);

    XMFLOAT2 stick = PadLogger_GetLeftThumbStick(0);
    static bool stickUpPrev = false, stickDownPrev = false;
    bool stickUp = stick.y > 0.5f, stickDown = stick.y < -0.5f;

    if (stickUp && !stickUpPrev)     upInput = true;
    if (stickDown && !stickDownPrev) downInput = true;
    stickUpPrev = stickUp; stickDownPrev = stickDown;

    if (upInput)
    {
        g_SelectedItem = (g_SelectedItem - 1 + MENU_COUNT) % MENU_COUNT;
        SoundManager_PlaySE(SOUND_SE_SELECT);
    }
    if (downInput)
    {
        g_SelectedItem = (g_SelectedItem + 1) % MENU_COUNT;
        SoundManager_PlaySE(SOUND_SE_SELECT);
    }

    /*--------------------------------------
     * マウス入力および項目選択
     *------------------------------------*/
    Mouse_State mouseState = {};
    Mouse_GetState(&mouseState);

    POINT cursorPos = {};
    GetCursorPos(&cursorPos);
    ScreenToClient(GameWindow_GetHWND(), &cursorPos);

    // クライアント領域座標 → バックバッファ座標に変換
    // （ボーダレス時はクライアント領域が画面解像度になるため補正が必要）
    RECT clientRect = {};
    GetClientRect(GameWindow_GetHWND(), &clientRect);
    float clientW = static_cast<float>(clientRect.right);
    float clientH = static_cast<float>(clientRect.bottom);
    float mouseX = (clientW > 0.0f) ? static_cast<float>(cursorPos.x) * (g_ScreenWidth / clientW) : 0.0f;
    float mouseY = (clientH > 0.0f) ? static_cast<float>(cursorPos.y) * (g_ScreenHeight / clientH) : 0.0f;

    // 左クリックのトリガー判定（自前で前フレーム比較）
    static bool prevLeftButton = false;
    bool leftTrigger = (mouseState.leftButton && !prevLeftButton);
    prevLeftButton = mouseState.leftButton;

    if (leftTrigger)
    {
        for (int i = 0; i < MENU_COUNT; i++)
        {
            float itemY = MENU_START_Y + i * MENU_SPACING;
            if (mouseY >= itemY - 10.0f && mouseY < itemY + CURSOR_HEIGHT - 10.0f &&
                mouseX >= LABEL_X - 20.0f && mouseX <= g_ScreenWidth - 50.0f)
            {
                if (g_SelectedItem != i)
                {
                    g_SelectedItem = i;
                    SoundManager_PlaySE(SOUND_SE_SELECT);
                }

                if (IsSliderItem(i)) g_IsDraggingSlider = true;
                else if (IsToggleItem(i))
                {
                    ToggleCurrentValue(i);
                    SoundManager_PlaySE(SOUND_SE_DECIDE);
                }
                else if (i == MENU_RESET)
                {
                    GameSettings_ResetToDefault();
                    SoundManager_PlaySE(SOUND_SE_DECIDE);
                }
                else if (i == MENU_BACK)
                {
                    SoundManager_PlaySE(SOUND_SE_DECIDE);
                    g_IsTransitioning = true;
                    Fade_Start(0.5, true);
                }
                break;
            }
        }
    }

    /*--------------------------------------
     * スライダードラッグ更新
     *------------------------------------*/
    if (g_IsDraggingSlider)
    {
        if (mouseState.leftButton)
        {
            float normalized = std::clamp((mouseX - SLIDER_X) / SLIDER_WIDTH, 0.0f, 1.0f);
            float minVal = 0.0f, maxVal = 1.0f;
            if (g_SelectedItem == MENU_SENSITIVITY) { minVal = SettingsRange::SENSITIVITY_MIN; maxVal = SettingsRange::SENSITIVITY_MAX; }
            else { minVal = SettingsRange::VOLUME_MIN; maxVal = SettingsRange::VOLUME_MAX; }

            SetCurrentValue(g_SelectedItem, minVal + normalized * (maxVal - minVal));
        }
        else g_IsDraggingSlider = false;
    }

    /*--------------------------------------
     * 左右操作（リピート処理）
     *------------------------------------*/
    bool leftHeld = KeyLogger_IsPressed(KK_A) || KeyLogger_IsPressed(KK_LEFT) || PadLogger_IsPressed(0, XINPUT_GAMEPAD_DPAD_LEFT) || (stick.x < -0.3f);
    bool rightHeld = KeyLogger_IsPressed(KK_D) || KeyLogger_IsPressed(KK_RIGHT) || PadLogger_IsPressed(0, XINPUT_GAMEPAD_DPAD_RIGHT) || (stick.x > 0.3f);

    if (leftHeld || rightHeld)
    {
        if (IsSliderItem(g_SelectedItem))
        {
            g_InputRepeatTimer += dt;
            bool shouldAdjust = false;
            if (g_FirstInput) { shouldAdjust = true; g_FirstInput = false; }
            else if (g_InputRepeatTimer >= REPEAT_DELAY) { shouldAdjust = true; g_InputRepeatTimer = REPEAT_DELAY - REPEAT_INTERVAL; }

            if (shouldAdjust)
            {
                float delta = (rightHeld ? ADJUST_SPEED : -ADJUST_SPEED);
                if (g_SelectedItem == MENU_SENSITIVITY) delta *= (SettingsRange::SENSITIVITY_MAX - SettingsRange::SENSITIVITY_MIN);
                AdjustValue(g_SelectedItem, delta);
            }
        }
        else if (IsToggleItem(g_SelectedItem))
        {
            if (KeyLogger_IsTrigger(KK_A) || KeyLogger_IsTrigger(KK_LEFT) || KeyLogger_IsTrigger(KK_D) || KeyLogger_IsTrigger(KK_RIGHT) ||
                PadLogger_IsTrigger(0, XINPUT_GAMEPAD_DPAD_LEFT) || PadLogger_IsTrigger(0, XINPUT_GAMEPAD_DPAD_RIGHT))
            {
                ToggleCurrentValue(g_SelectedItem);
                SoundManager_PlaySE(SOUND_SE_DECIDE);
            }
        }
    }
    else { g_InputRepeatTimer = 0.0f; g_FirstInput = true; }

    /*--------------------------------------
     * 決定・キャンセル入力
     *------------------------------------*/
    if (KeyLogger_IsTrigger(KK_ENTER) || KeyLogger_IsTrigger(KK_SPACE) || PadLogger_IsTrigger(0, XINPUT_GAMEPAD_A))
    {
        if (g_SelectedItem == MENU_BACK)
        {
            SoundManager_PlaySE(SOUND_SE_DECIDE);
            g_IsTransitioning = true;
            Fade_Start(0.5, true);
        }
        else if (g_SelectedItem == MENU_RESET)
        {
            GameSettings_ResetToDefault();
            SoundManager_PlaySE(SOUND_SE_DECIDE);
        }
        else if (IsToggleItem(g_SelectedItem))
        {
            ToggleCurrentValue(g_SelectedItem);
            SoundManager_PlaySE(SOUND_SE_DECIDE);
        }
    }

    if (KeyLogger_IsTrigger(KK_BACK) || PadLogger_IsTrigger(0, XINPUT_GAMEPAD_B))
    {
        SoundManager_PlaySE(SOUND_SE_CANCEL);
        g_IsTransitioning = true;
        Fade_Start(0.5, true);
    }
}

/**
 * @brief 設定画面の描画
 */
void Settings_Draw()
{
    Direct3D_DepthStencilStateDepthIsEnable(false);
    Direct3D_SetBlendState(BlendMode::Alpha);
    Sprite_Begin();

    /*--------------------------------------
     * 背景・タイトルの描画
     *------------------------------------*/
    if (g_TexBg >= 0) Sprite_Draw(g_TexBg, 0.0f, 0.0f, g_ScreenWidth, g_ScreenHeight);
    else if (g_TexWhite >= 0) Sprite_Draw(g_TexWhite, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, g_ScreenWidth, g_ScreenHeight, 0.0f, { 0.08f, 0.08f, 0.12f, 1.0f });

    if (g_TexTitle >= 0) Sprite_Draw(g_TexTitle, (g_ScreenWidth - 400.0f) * 0.5f, TITLE_Y, 400.0f, 80.0f);

    /*--------------------------------------
     * 各メニュー項目の描画ループ
     *------------------------------------*/
    for (int i = 0; i < MENU_COUNT; i++)
    {
        float y = MENU_START_Y + i * MENU_SPACING;
        bool selected = (i == g_SelectedItem);

        if (selected && g_TexWhite >= 0)
        {
            float alpha = 0.3f + 0.15f * sinf(g_AnimTimer * CURSOR_SPEED);
            Sprite_Draw(g_TexWhite, LABEL_X - 20.0f, y - 10.0f, 0.0f, 0.0f, 1.0f, 1.0f, g_ScreenWidth - LABEL_X - 50.0f, CURSOR_HEIGHT, 0.0f, { 0.3f, 0.5f, 0.9f, alpha });
        }

        if (IsSliderItem(i)) DrawSlider(SLIDER_X, y + 3.0f, GetCurrentValue(i), (i == MENU_SENSITIVITY ? SettingsRange::SENSITIVITY_MIN : SettingsRange::VOLUME_MIN), (i == MENU_SENSITIVITY ? SettingsRange::SENSITIVITY_MAX : SettingsRange::VOLUME_MAX), selected);
        else if (IsToggleItem(i)) DrawToggle(SLIDER_X, y + 2.0f, GetCurrentToggle(i), selected);
        else DrawButton(SLIDER_X, y - 5.0f, MENU_LABELS[i], selected);
    }

    /*--------------------------------------
     * テキスト情報の描画（DebugText）
     *------------------------------------*/
    if (g_pDebugText)
    {
        std::stringstream ss;
        if (g_TexTitle < 0) ss << "\n        SETTINGS\n\n"; else ss << "\n\n\n\n\n";

        for (int i = 0; i < MENU_COUNT; i++)
        {
            ss << (i == g_SelectedItem ? "> " : "  ") << MENU_LABELS[i];
            for (int p = static_cast<int>(strlen(MENU_LABELS[i])); p < 25; p++) ss << " ";

            if (IsSliderItem(i))
            {
                float v = GetCurrentValue(i), minV = (i == MENU_SENSITIVITY ? SettingsRange::SENSITIVITY_MIN : SettingsRange::VOLUME_MIN), maxV = (i == MENU_SENSITIVITY ? SettingsRange::SENSITIVITY_MAX : SettingsRange::VOLUME_MAX);
                ss << static_cast<int>((v - minV) / (maxV - minV) * 100.0f + 0.5f) << "%";
            }
            else if (IsToggleItem(i)) ss << (GetCurrentToggle(i) ? "ON" : "OFF");
            ss << "\n\n";
        }
        ss << "\n\n  [W/S] Select  [A/D] Adjust  [Enter] Confirm  [BS] Back";
        g_pDebugText->SetText(ss.str().c_str());
        g_pDebugText->Draw();
        g_pDebugText->Clear();
    }
    Direct3D_DepthStencilStateDepthIsEnable(true);
}

/**
 * @brief シャドウ描画（未使用）
 */
void Settings_DrawShadow() {}

/****************************************
 * 内部補助関数群の実装
 ****************************************/

 /**
  * @brief スライダーの描画
  */
static void DrawSlider(float x, float y, float value, float minVal, float maxVal, bool selected)
{
    if (g_TexWhite < 0) return;
    float norm = std::clamp((value - minVal) / (maxVal - minVal), 0.0f, 1.0f);

    Sprite_Draw(g_TexWhite, x, y, 0.0f, 0.0f, 1.0f, 1.0f, SLIDER_WIDTH, SLIDER_HEIGHT, 0.0f, { 0.2f, 0.2f, 0.25f, 0.9f });
    float barWidth = SLIDER_WIDTH * norm;
    if (barWidth > 0)
    {
        float pulse = selected ? (0.8f + 0.2f * sinf(g_AnimTimer * BLINK_SPEED)) : 0.7f;
        Sprite_Draw(g_TexWhite, x, y, 0.0f, 0.0f, 1.0f, 1.0f, barWidth, SLIDER_HEIGHT, 0.0f, { 0.3f * pulse, 0.6f * pulse, 1.0f * pulse, 0.9f });
    }
    Sprite_Draw(g_TexWhite, x + barWidth - KNOB_WIDTH * 0.5f, y - (KNOB_HEIGHT - SLIDER_HEIGHT) * 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, KNOB_WIDTH, KNOB_HEIGHT, 0.0f, selected ? XMFLOAT4{ 1,1,1,1 } : XMFLOAT4{ 0.85f, 0.85f, 0.9f, 1.0f });
}

/**
 * @brief ボタン状UIの背景描画
 */
static void DrawButton(float x, float y, const char* /*text*/, bool selected)
{
    if (g_TexWhite < 0) return;
    float alpha = selected ? (0.85f + 0.15f * sinf(g_AnimTimer * BLINK_SPEED)) : 0.8f;
    Sprite_Draw(g_TexWhite, x, y, 0.0f, 0.0f, 1.0f, 1.0f, BUTTON_WIDTH, BUTTON_HEIGHT, 0.0f, selected ? XMFLOAT4{ 0.3f, 0.5f, 0.8f, alpha } : XMFLOAT4{ 0.2f, 0.25f, 0.35f, 0.8f });
}

/**
 * @brief ON/OFFトグルの描画
 */
static void DrawToggle(float x, float y, bool value, bool selected)
{
    if (g_TexWhite < 0) return;
    float alpha = selected ? (0.85f + 0.15f * sinf(g_AnimTimer * BLINK_SPEED)) : 0.8f;
    Sprite_Draw(g_TexWhite, x, y, 0.0f, 0.0f, 1.0f, 1.0f, TOGGLE_WIDTH, TOGGLE_HEIGHT, 0.0f, value ? XMFLOAT4{ 0.15f, 0.5f, 0.3f, alpha } : XMFLOAT4{ 0.35f, 0.15f, 0.15f, alpha });
    float kSize = TOGGLE_HEIGHT - 4.0f;
    Sprite_Draw(g_TexWhite, value ? (x + TOGGLE_WIDTH - kSize - 2.0f) : (x + 2.0f), y + 2.0f, 0.0f, 0.0f, 1.0f, 1.0f, kSize, kSize, 0.0f, { 1,1,1,1 });
}

//--------------------------------------
// 各種ゲッター・セッター・調整関数
//--------------------------------------
static float GetCurrentValue(int item) {
    if (item == MENU_SENSITIVITY) return GameSettings_GetSensitivity();
    if (item == MENU_BGM_VOLUME)  return GameSettings_GetBGMVolume();
    if (item == MENU_SE_VOLUME)   return GameSettings_GetSEVolume();
    return 0.0f;
}

static void SetCurrentValue(int item, float value) {
    if (item == MENU_SENSITIVITY) GameSettings_SetSensitivity(value);
    else if (item == MENU_BGM_VOLUME) GameSettings_SetBGMVolume(value);
    else if (item == MENU_SE_VOLUME)  GameSettings_SetSEVolume(value);
}

static bool GetCurrentToggle(int item) {
    if (item == MENU_BORDERLESS) return GameSettings_GetBorderless();
    if (item == MENU_VSYNC)      return GameSettings_GetVSync();
    return false;
}

static void ToggleCurrentValue(int item) {
    if (item == MENU_BORDERLESS) GameSettings_SetBorderless(!GameSettings_GetBorderless());
    else if (item == MENU_VSYNC) GameSettings_SetVSync(!GameSettings_GetVSync());
}

static void AdjustValue(int item, float delta) {
    float val = GetCurrentValue(item) + delta;
    if (item == MENU_SENSITIVITY) val = std::clamp(val, SettingsRange::SENSITIVITY_MIN, SettingsRange::SENSITIVITY_MAX);
    else val = std::clamp(val, SettingsRange::VOLUME_MIN, SettingsRange::VOLUME_MAX);
    SetCurrentValue(item, val);
}