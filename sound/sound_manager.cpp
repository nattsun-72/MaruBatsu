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

#ifdef USE_ASSET_DLL
#include "asset_dll.h"
#include "resource.h"
#endif

namespace
{
    struct SoundFileInfo
    {
        const char* path;
        bool isBGM;
    };

    // Day 1段階では実ファイル無し。β版で素材を入れる時にパスを設定する。
    const SoundFileInfo SOUND_FILES[SOUND_MAX] =
    {
        { nullptr, true  },  // SOUND_BGM_TITLE
        { nullptr, true  },  // SOUND_BGM_GAME
        { nullptr, true  },  // SOUND_BGM_RESULT
        { nullptr, false },  // SOUND_SE_SELECT
        { nullptr, false },  // SOUND_SE_DECIDE
        { nullptr, false },  // SOUND_SE_CANCEL
        { nullptr, false },  // SOUND_SE_PAUSE
    };
}

// 内部変数
static int g_AudioHandles[SOUND_MAX];
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
        if (SOUND_FILES[i].path != nullptr)
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
}

void SoundManager_PlayBGM(SoundID id)
{
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
