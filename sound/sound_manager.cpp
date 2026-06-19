/****************************************
 * @file sound_manager.cpp
 * @brief ゲーム全体のサウンド管理 (2D版・最小構成)
 * @author Natsume Shidara
 * @date 2026/01/13
 * @update 2026/05/15 - 〇×ローグライト用にスリム化
 *
 * β版でサウンド素材を入れる際は SOUND_FILES のパスと
 * SOUND_RESOURCE_IDS (USE_ASSET_DLL時) を編集する。
 ****************************************/

#include "sound_manager.h"
#include "audio.h"
#include <cmath>
#include <algorithm>
#include <string>
#include <filesystem>
#include <system_error>

#ifdef USE_ASSET_DLL
#include "asset_dll.h"
#include "resource.h"
#endif

namespace
{
    struct SoundFileInfo
    {
        const char* path;   // 実行時カレント基準の相対パス (assets/sound/...)
        bool isBGM;
    };

    // ----------------------------------------------------------------------
    // サウンド素材テーブル
    //
    // 各 ID に「素材を置くべき相対パス」を割り当てている。素材は未確保でよく、
    // 起動時にファイルが存在する ID のみロードする (存在チェックは
    // SoundManager_Initialize)。よって:
    //   ・ファイルが無くてもロードを試行しない → 参照エラー/警告が出ない
    //   ・あとから assets/sound/ に下記の名前で mp3 (または wav) を置くだけで、
    //     次回起動時に自動的に鳴るようになる (コード変更不要)
    // 拡張子を wav にしたい場合は下のパスの ".mp3" を ".wav" へ変えるだけでよい。
    // ----------------------------------------------------------------------
    const SoundFileInfo SOUND_FILES[SOUND_MAX] =
    {
        { "assets/sound/bgm_title.mp3",   true  },  // SOUND_BGM_TITLE
        { "assets/sound/bgm_game.mp3",    true  },  // SOUND_BGM_GAME
        { "assets/sound/bgm_result.mp3",  true  },  // SOUND_BGM_RESULT
        { "assets/sound/se_select.mp3",   false },  // SOUND_SE_SELECT
        { "assets/sound/se_decide.mp3",   false },  // SOUND_SE_DECIDE
        { "assets/sound/se_cancel.mp3",   false },  // SOUND_SE_CANCEL
        { "assets/sound/se_pause.mp3",    false },  // SOUND_SE_PAUSE
        { "assets/sound/se_place.mp3",    false },  // SOUND_SE_PLACE
        { "assets/sound/se_win.mp3",      false },  // SOUND_SE_WIN
        { "assets/sound/se_lose.mp3",     false },  // SOUND_SE_LOSE
        { "assets/sound/se_ability.mp3",  false },  // SOUND_SE_ABILITY
        { "assets/sound/se_timer_low.mp3",false },  // SOUND_SE_TIMER_LOW
    };

    /**
     * @brief  指定パスの素材が読み込み可能な状態で存在するか
     * @param  path 相対/絶対パス (nullptr 可)
     * @return 通常ファイルとして存在すれば true。例外は投げない (安全側で false)。
     */
    bool AudioFileExists(const char* path)
    {
        if (path == nullptr || path[0] == '\0') return false;
        std::error_code ec;   // throw しない exists/is_regular_file を使う
        return std::filesystem::is_regular_file(std::filesystem::path(path), ec) && !ec;
    }
}

// 内部変数
static int  g_AudioHandles[SOUND_MAX];
static bool g_Initialized = false;   // Initialize 完了前の再生要求を弾くためのガード
static SoundID g_CurrentBGM = SOUND_MAX;
static float g_BGMVolume = 1.0f;
static float g_SEVolume = 1.0f;

#ifdef USE_ASSET_DLL
// SoundID → DLLリソースID マッピング (β版で実素材を入れる時に有効化)
// Phase 5 で resource.h を整理する際に対応IDを定義する。
// 現状はリソースIDが未定義なので、USE_ASSET_DLL経路はファイルパス経由にフォールバック。
#endif

void SoundManager_Initialize()
{
    InitAudio();

    for (int i = 0; i < SOUND_MAX; i++)
    {
        // 素材が実在する ID のみロード。未配置の ID は -1 (無音) のままにし、
        // ロード自体を試みない (ファイル未存在による警告・エラーを避ける)。
        if (AudioFileExists(SOUND_FILES[i].path))
        {
            g_AudioHandles[i] = LoadAudio(SOUND_FILES[i].path);
        }
        else
        {
            g_AudioHandles[i] = -1;
        }
    }

    g_CurrentBGM = SOUND_MAX;
    g_BGMVolume = 1.0f;
    g_SEVolume = 1.0f;
    g_Initialized = true;
}

void SoundManager_Finalize()
{
    for (int i = 0; i < SOUND_MAX; i++)
    {
        if (g_AudioHandles[i] >= 0)
        {
            UnloadAudio(g_AudioHandles[i]);
            g_AudioHandles[i] = -1;
        }
    }

    UninitAudio();
    g_Initialized = false;
}

void SoundManager_PlayBGM(SoundID id)
{
    if (!g_Initialized) return;
    if (id < 0 || id >= SOUND_MAX) return;
    if (g_AudioHandles[id] < 0) return;
    if (g_CurrentBGM == id) return;

    SoundManager_StopBGM();

    SetAudioVolume(g_AudioHandles[id], g_BGMVolume);
    PlayAudio(g_AudioHandles[id], true);
    g_CurrentBGM = id;
}

void SoundManager_StopBGM()
{
    if (g_CurrentBGM != SOUND_MAX && g_AudioHandles[g_CurrentBGM] >= 0)
    {
        StopAudio(g_AudioHandles[g_CurrentBGM]);
        g_CurrentBGM = SOUND_MAX;
    }
}

void SoundManager_SetBGMVolume(float volume)
{
    g_BGMVolume = fmaxf(0.0f, fminf(1.0f, volume));

    if (g_CurrentBGM != SOUND_MAX && g_AudioHandles[g_CurrentBGM] >= 0)
    {
        SetAudioVolume(g_AudioHandles[g_CurrentBGM], g_BGMVolume);
    }
}

void SoundManager_PlaySE(SoundID id)
{
    if (!g_Initialized) return;
    if (id < 0 || id >= SOUND_MAX) return;
    if (g_AudioHandles[id] < 0) return;

    SetAudioVolume(g_AudioHandles[id], g_SEVolume);
    PlayAudio(g_AudioHandles[id], false);
}

void SoundManager_SetSEVolume(float volume)
{
    g_SEVolume = fmaxf(0.0f, fminf(1.0f, volume));
}

void SoundManager_Update(float /*dt*/)
{
}

void SoundManager_StopAll()
{
    StopAllAudio();
    g_CurrentBGM = SOUND_MAX;
}
