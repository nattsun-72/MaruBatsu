/****************************************
 * @file main.cpp
 * @brief アプリケーションエントリーポイント (2D版)
 * @author Natsume Shidara
 * @date 2025/06/06
 * @update 2026/05/15 - 〇×ローグライト用2D版へスリム化
 *
 * WinMainを含むメインループの管理。
 * 各サブシステムの初期化・終了を制御し、
 * フレーム更新と描画のループを実行する。
 *
 * フレームレート制御:
 *   VSync（垂直同期）により、モニターのリフレッシュレートに同期。
 *   Direct3D_Present() 内の Present(1, 0) で制御。
 ****************************************/

#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <sstream>

// ウィンドウ・入力
#include "game_window.h"
#include "key_logger.h"
#include "pad_logger.h"
#include "mouse.h"

// タイマー
#include "system_timer.h"

// サウンド・設定
#include "sound_manager.h"
#include "game_settings.h"

// Direct3D・シェーダー
#include "direct3d.h"
#include "sampler.h"
#include "shader.h"

// 描画リソース
#include "sprite.h"
#include "texture.h"
#include "sprite_anim.h"

// ゲームロジック・UI
#include "scene.h"
#include "fade.h"
#include "debug_text.h"

int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPSTR,
    _In_ int nCmdShow)
{
    // COM・DPI初期化
    (void)CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    // ウィンドウ作成
    HWND hWnd = GameWindow_Create(hInstance);

    // 入力・タイマー初期化
    SystemTimer_Initialize();
    KeyLogger_Initialize();
    Mouse_Initialize(hWnd);
    PadLogger_Initialize();

    // サウンド・ゲーム設定初期化
    SoundManager_Initialize();
    GameSettings_Initialize();

    // Direct3D・2Dシェーダー初期化
    Direct3D_Initialize(hWnd);
    Sampler_Initialize(Direct3D_GetDevice(), Direct3D_GetContext());
    Shader_Initialize(Direct3D_GetDevice(), Direct3D_GetContext());

    // 描画リソース初期化
    Sprite_Initialize(Direct3D_GetDevice(), Direct3D_GetContext());
    Texture_Initialize(Direct3D_GetDevice(), Direct3D_GetContext());
    SpriteAnim_Initialize();
    Fade_Initialize();

    // デバッグテキスト初期化
    hal::DebugText debugText(Direct3D_GetDevice(), Direct3D_GetContext(),
        L"assets/consolab_ascii_512.png",
        Direct3D_GetBackBufferWidth(), Direct3D_GetBackBufferHeight(),
        0.0f, 0.0f, 0, 0, 0.0f, 16.0f);

    // マウス・シーン初期化
    Mouse_SetVisible(true);
    Mouse_SetMode(MOUSE_POSITION_MODE_ABSOLUTE);
    Scene_Initialize();

    // ウィンドウ表示
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    // フレーム計測用変数
    SystemTimer_Reset();
    double fps_last_time = SystemTimer_GetTime();
    double current_time = 0.0;
    ULONG frame_count = 0;
    double fps = 0.0;
    double elapsed_time = 0.0;

    // メインループ
    MSG msg = {};

    do
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            // FPS計測（1秒ごとに更新）
            current_time = SystemTimer_GetTime();
            double fps_elapsed_time = current_time - fps_last_time;

            if (fps_elapsed_time >= 1.0)
            {
                fps = frame_count / fps_elapsed_time;
                fps_last_time = current_time;
                frame_count = 0;
            }

            elapsed_time += SystemTimer_GetElapsedTime();

            //----------------------------------------------------------
            // 更新処理
            //----------------------------------------------------------
            KeyLogger_Update();
            PadLogger_Update();
            Scene_Update(elapsed_time);
            SpriteAnim_Update(elapsed_time);
            Fade_Update(elapsed_time);

            //----------------------------------------------------------
            // 描画処理
            //----------------------------------------------------------
            Scene_Draw();

            // フェード・デバッグ表示（最前面）
            Direct3D_SetBlendState(BlendMode::Alpha);
            Fade_Draw();

#if defined(DEBUG) || defined(_DEBUG)
            std::stringstream ss;
            ss << "fps:" << fps << std::endl;
            debugText.SetText(ss.str().c_str());
            debugText.Draw();
            debugText.Clear();
#endif

            // バックバッファ表示（VSync待ち）
            Direct3D_Present();
            Scene_Refresh();

            frame_count++;
            elapsed_time = 0.0;
        }
    } while (msg.message != WM_QUIT);

    // 終了処理（初期化の逆順）

    // シーン
    Scene_Finalize();

    // ゲーム設定・サウンド
    GameSettings_Finalize();
    SoundManager_Finalize();

    // 描画リソース
    Fade_Finalize();
    SpriteAnim_Finalize();
    Texture_Finalize();
    Sprite_Finalize();

    // シェーダー
    Shader_Finalize();

    // Direct3D・サンプラー
    Sampler_Finalize();
    Direct3D_Finalize();

    // 入力
    Mouse_Finalize();

    return static_cast<int>(msg.wParam);
}
