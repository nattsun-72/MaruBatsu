/****************************************
 * @file game_settings.cpp
 * @brief ゲーム設定の管理
 * @author Natsume Shidara
 * @date 2026/01/13
 ****************************************/

#include "game_settings.h"
#include "sound_manager.h"
#include "game_window.h"
#include "direct3d.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>

 // 内部変数
static float g_Sensitivity = SettingsRange::SENSITIVITY_DEFAULT;
static float g_BGMVolume = SettingsRange::BGM_VOLUME_DEFAULT;
static float g_SEVolume = SettingsRange::SE_VOLUME_DEFAULT;
static bool g_Borderless = SettingsRange::BORDERLESS_DEFAULT;
static bool g_VSync = SettingsRange::VSYNC_DEFAULT;

// ヘルパー関数
static float Clamp(float value, float min, float max)
{
    return std::max(min, std::min(max, value));
}

// 初期化・終了

void GameSettings_Initialize()
{
    // デフォルト値で初期化
    g_Sensitivity = SettingsRange::SENSITIVITY_DEFAULT;
    g_BGMVolume = SettingsRange::BGM_VOLUME_DEFAULT;
    g_SEVolume = SettingsRange::SE_VOLUME_DEFAULT;
    g_Borderless = SettingsRange::BORDERLESS_DEFAULT;
    g_VSync = SettingsRange::VSYNC_DEFAULT;

    // サウンドマネージャーに初期値を適用
    SoundManager_SetBGMVolume(g_BGMVolume);
    SoundManager_SetSEVolume(g_SEVolume);
}

void GameSettings_Finalize()
{
    // 終了時に最新の設定を書き出す (オーバーレイ閉時にも保存するため通常は冗長)
    GameSettings_Save();
}

// 永続化 (save/settings.dat, BOMなしUTF-8テキスト, "キー 値" 1行1項目)

namespace
{
    const char* SETTINGS_DIR  = "save";
    const char* SETTINGS_PATH = "save/settings.dat";
}

void GameSettings_Load()
{
    std::ifstream ifs(SETTINGS_PATH, std::ios::binary);
    if (!ifs) return;   // 保存なし → 既定値のまま

    std::string key;
    // "キー 値" を順に読む。未知キー/欠落は無視し、既知キーのみ適用する。
    // 適用は setter 経由なので、クランプとサブシステムへの反映も同時に行われる。
    while (ifs >> key)
    {
        if (key == "bgm")         { float v; if (ifs >> v) GameSettings_SetBGMVolume(v); }
        else if (key == "se")     { float v; if (ifs >> v) GameSettings_SetSEVolume(v); }
        else if (key == "vsync")  { int  v; if (ifs >> v) GameSettings_SetVSync(v != 0); }
        else                      { std::string skip; std::getline(ifs, skip); }  // 未知キー行を読み飛ばす
    }
}

void GameSettings_Save()
{
    std::error_code ec;
    std::filesystem::create_directories(SETTINGS_DIR, ec);

    std::ofstream ofs(SETTINGS_PATH, std::ios::binary | std::ios::trunc);
    if (!ofs) return;

    ofs << "bgm "   << g_BGMVolume << "\n";
    ofs << "se "    << g_SEVolume  << "\n";
    ofs << "vsync " << (g_VSync ? 1 : 0) << "\n";
}

// 視点感度

float GameSettings_GetSensitivity()
{
    return g_Sensitivity;
}

void GameSettings_SetSensitivity(float value)
{
    g_Sensitivity = Clamp(value, SettingsRange::SENSITIVITY_MIN, SettingsRange::SENSITIVITY_MAX);
}

// BGM音量

float GameSettings_GetBGMVolume()
{
    return g_BGMVolume;
}

void GameSettings_SetBGMVolume(float value)
{
    g_BGMVolume = Clamp(value, SettingsRange::VOLUME_MIN, SettingsRange::VOLUME_MAX);
    SoundManager_SetBGMVolume(g_BGMVolume);
}

// SE音量

float GameSettings_GetSEVolume()
{
    return g_SEVolume;
}

void GameSettings_SetSEVolume(float value)
{
    g_SEVolume = Clamp(value, SettingsRange::VOLUME_MIN, SettingsRange::VOLUME_MAX);
    SoundManager_SetSEVolume(g_SEVolume);
}

// ボーダーレスウィンドウ

bool GameSettings_GetBorderless()
{
    return g_Borderless;
}

void GameSettings_SetBorderless(bool value)
{
    g_Borderless = value;
    GameWindow_SetBorderless(value);
}

// 垂直同期（VSync）

bool GameSettings_GetVSync()
{
    return g_VSync;
}

void GameSettings_SetVSync(bool value)
{
    g_VSync = value;
    Direct3D_SetVSync(value);
}

// 設定のリセット

void GameSettings_ResetToDefault()
{
    GameSettings_SetSensitivity(SettingsRange::SENSITIVITY_DEFAULT);
    GameSettings_SetBGMVolume(SettingsRange::BGM_VOLUME_DEFAULT);
    GameSettings_SetSEVolume(SettingsRange::SE_VOLUME_DEFAULT);
    GameSettings_SetBorderless(SettingsRange::BORDERLESS_DEFAULT);
    GameSettings_SetVSync(SettingsRange::VSYNC_DEFAULT);
}