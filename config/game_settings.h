/****************************************
 * @file game_settings.h
 * @brief ゲーム設定の管理
 * @author Natsume Shidara
 * @date 2026/01/13
 *
 * 各種設定値を保持し、ゲーム全体から参照可能にする
 ****************************************/

#ifndef GAME_SETTINGS_H
#define GAME_SETTINGS_H

 // 設定値の範囲
namespace SettingsRange
{
    // 視点感度
    constexpr float SENSITIVITY_MIN = 0.1f;
    constexpr float SENSITIVITY_MAX = 3.0f;
    constexpr float SENSITIVITY_DEFAULT = 1.0f;

    // 音量
    constexpr float VOLUME_MIN = 0.0f;
    constexpr float VOLUME_MAX = 1.0f;
    constexpr float BGM_VOLUME_DEFAULT = 0.3f;
    constexpr float SE_VOLUME_DEFAULT = 0.3f;

    // 表示設定
    constexpr bool BORDERLESS_DEFAULT = false;
    constexpr bool VSYNC_DEFAULT = true;
}

// 初期化・終了
void GameSettings_Initialize();
void GameSettings_Finalize();

// 永続化 (save/settings.dat へ保存/読込)
//  Load: 既定値で初期化済みの状態から、保存値があれば上書き適用する。
//        全サブシステム(サウンド/Direct3D/ウィンドウ)初期化後に呼ぶこと。
//  Save: 現在値をファイルへ書き出す (設定変更時・終了時に呼ぶ)。
void GameSettings_Load();
void GameSettings_Save();

// 視点感度
float GameSettings_GetSensitivity();
void GameSettings_SetSensitivity(float value);

// BGM音量
float GameSettings_GetBGMVolume();
void GameSettings_SetBGMVolume(float value);

// SE音量
float GameSettings_GetSEVolume();
void GameSettings_SetSEVolume(float value);

// ボーダーレスウィンドウ
bool GameSettings_GetBorderless();
void GameSettings_SetBorderless(bool value);

// 垂直同期（VSync）
bool GameSettings_GetVSync();
void GameSettings_SetVSync(bool value);

// 設定のリセット
void GameSettings_ResetToDefault();

#endif // GAME_SETTINGS_H