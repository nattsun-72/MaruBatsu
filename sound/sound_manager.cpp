/****************************************
 * @file sound_manager.cpp
 * @brief ゲーム全体のサウンド管理
 * @author Natsume Shidara
 * @date 2026/01/13
 * @update 2026/02/22 - フィーバーティアに応じたBGMピッチ変化
 *
 * @note 音源ファイルを追加する場合は SOUND_FILES 配列の
 *       パスを変更してください
 ****************************************/

#include "sound_manager.h"
#include "audio.h"
#include "fever.h"
#include <cmath>
#include <algorithm>
#include <string>

#ifdef USE_ASSET_DLL
#include "asset_dll.h"
#include "resource.h"
#endif

 // 音源ファイルパス設定
 // ※ここを変更すれば音源を差し替え可能
namespace
{
    struct SoundFileInfo
    {
        const char* path;
        bool isBGM;
    };

    const SoundFileInfo SOUND_FILES[SOUND_MAX] =
    {
        // BGM
        { "assets/sound/bgm/title.mp3",         true  },  // SOUND_BGM_TITLE
        { "assets/sound/bgm/game.mp3",          true  },  // SOUND_BGM_GAME
        { "assets/sound/bgm/result.mp3",        true  },  // SOUND_BGM_RESULT

        // SE - プレイヤー
        { "assets/sound/se/slash.mp3",          false },  // SOUND_SE_SLASH
        { "assets/sound/se/slash_heavy.mp3",    false },  // SOUND_SE_SLASH_HEAVY
        { "assets/sound/se/slash_hit.mp3",      false },  // SOUND_SE_SLASH_HIT
        { "assets/sound/se/player_damage.mp3",  false },  // SOUND_SE_PLAYER_DAMAGE
        { "assets/sound/se/jump.mp3",           false },  // SOUND_SE_JUMP
        { "assets/sound/se/double_jump.mp3",    false },  // SOUND_SE_DOUBLE_JUMP
        { "assets/sound/se/dash_charge.mp3",    false },  // SOUND_SE_DASH_CHARGE
        { "assets/sound/se/dash.mp3",           false },  // SOUND_SE_DASH
        { "assets/sound/se/step.mp3",           false },  // SOUND_SE_STEP
        { "assets/sound/se/land.mp3",           false },  // SOUND_SE_LAND

        // SE - コンボ
        { "assets/sound/se/combo_1.mp3",        false },  // SOUND_SE_COMBO_1
        { "assets/sound/se/combo_2.mp3",        false },  // SOUND_SE_COMBO_2
        { "assets/sound/se/combo_3.mp3",        false },  // SOUND_SE_COMBO_3
        { "assets/sound/se/combo_4.mp3",        false },  // SOUND_SE_COMBO_4

        // SE - 敵（地上型）
        { "assets/sound/se/enemy_fire.mp3",     false },  // SOUND_SE_ENEMY_FIRE
        { "assets/sound/se/enemy_death.mp3",    false },  // SOUND_SE_ENEMY_DEATH
        { "assets/sound/se/enemy_slice.mp3",    false },  // SOUND_SE_ENEMY_SLICE

        // SE - 敵（飛行型）
        { "assets/sound/se/flying_charge.mp3", false },  // SOUND_SE_FLYING_CHARGE_READY
        { "assets/sound/se/flying_charge.mp3",       false },  // SOUND_SE_FLYING_CHARGE
        { "assets/sound/se/flying_whoosh.mp3",       false },  // SOUND_SE_FLYING_WHOOSH

        // SE - UI
        { "assets/sound/se/select.mp3",         false },  // SOUND_SE_SELECT
        { "assets/sound/se/decide.mp3",         false },  // SOUND_SE_DECIDE
        { "assets/sound/se/cancel.mp3",         false },  // SOUND_SE_CANCEL
        { "assets/sound/se/pause.mp3",          false },  // SOUND_SE_PAUSE

        // SE - タイトル
        { "assets/sound/se/title_slash.mp3",    false },  // SOUND_SE_TITLE_SLASH

        // SE - リザルト
        { "assets/sound/se/score_count.mp3",    false },  // SOUND_SE_SCORE_COUNT
        { "assets/sound/se/score_finish.mp3",   false },  // SOUND_SE_SCORE_FINISH
    };
}

// BGMピッチ設定（フィーバーティア別）
// 半音単位: Tier1 +1半音、Tier2 +2半音、Tier3 +3半音
// 周波数比 = 2^(semitones/12)
namespace
{
    constexpr float PITCH_NONE  = 1.0f;                    // 通常
    constexpr float PITCH_TIER1 = 1.059463f;               // 2^(1/12) ≈ +1半音
    constexpr float PITCH_TIER2 = 1.122462f;               // 2^(2/12) ≈ +2半音
    constexpr float PITCH_TIER3 = 1.189207f;               // 2^(3/12) ≈ +3半音

    // ピッチ補間速度（秒あたり）
    constexpr float PITCH_LERP_SPEED = 4.0f;

    // コンボSE段階閾値
    constexpr int COMBO_THRESHOLD_TIER4 = 50;  // コンボSE最高段階の閾値
    constexpr int COMBO_THRESHOLD_TIER3 = 20;  // コンボSE第3段階の閾値
    constexpr int COMBO_THRESHOLD_TIER2 = 10;  // コンボSE第2段階の閾値

    float GetTargetPitch(FeverTier tier)
    {
        switch (tier)
        {
        case FeverTier::Tier1: return PITCH_TIER1;
        case FeverTier::Tier2: return PITCH_TIER2;
        case FeverTier::Tier3: return PITCH_TIER3;
        default:               return PITCH_NONE;
        }
    }
}

// 内部変数
static int g_AudioHandles[SOUND_MAX];
static SoundID g_CurrentBGM = SOUND_MAX;
static float g_BGMVolume = 1.0f;
static float g_SEVolume = 1.0f;
static float g_CurrentBGMPitch = PITCH_NONE;

#ifdef USE_ASSET_DLL
// SoundID → DLLリソースID マッピングテーブル
static const int SOUND_RESOURCE_IDS[SOUND_MAX] =
{
    // BGM
    IDR_SND_BGM_TITLE,          // SOUND_BGM_TITLE
    IDR_SND_BGM_GAME,           // SOUND_BGM_GAME
    IDR_SND_BGM_RESULT,         // SOUND_BGM_RESULT

    // SE - プレイヤー
    IDR_SND_SE_SLASH,           // SOUND_SE_SLASH
    IDR_SND_SE_SLASH_HEAVY,     // SOUND_SE_SLASH_HEAVY
    IDR_SND_SE_SLASH_HIT,       // SOUND_SE_SLASH_HIT
    IDR_SND_SE_PLAYER_DAMAGE,   // SOUND_SE_PLAYER_DAMAGE
    IDR_SND_SE_JUMP,            // SOUND_SE_JUMP
    IDR_SND_SE_DOUBLE_JUMP,     // SOUND_SE_DOUBLE_JUMP
    IDR_SND_SE_DASH_CHARGE,     // SOUND_SE_DASH_CHARGE
    IDR_SND_SE_DASH,            // SOUND_SE_DASH
    IDR_SND_SE_STEP,            // SOUND_SE_STEP
    IDR_SND_SE_LAND,            // SOUND_SE_LAND

    // SE - コンボ
    IDR_SND_SE_COMBO_1,         // SOUND_SE_COMBO_1
    IDR_SND_SE_COMBO_2,         // SOUND_SE_COMBO_2
    IDR_SND_SE_COMBO_3,         // SOUND_SE_COMBO_3
    IDR_SND_SE_COMBO_4,         // SOUND_SE_COMBO_4

    // SE - 敵（地上型）
    IDR_SND_SE_ENEMY_FIRE,      // SOUND_SE_ENEMY_FIRE
    IDR_SND_SE_ENEMY_DEATH,     // SOUND_SE_ENEMY_DEATH
    IDR_SND_SE_ENEMY_SLICE,     // SOUND_SE_ENEMY_SLICE

    // SE - 敵（飛行型）
    IDR_SND_SE_FLYING_CHARGE,   // SOUND_SE_FLYING_CHARGE_READY
    IDR_SND_SE_FLYING_CHARGE,   // SOUND_SE_FLYING_CHARGE
    IDR_SND_SE_FLYING_WHOOSH,   // SOUND_SE_FLYING_WHOOSH

    // SE - UI
    IDR_SND_SE_SELECT,          // SOUND_SE_SELECT
    IDR_SND_SE_DECIDE,          // SOUND_SE_DECIDE
    IDR_SND_SE_CANCEL,          // SOUND_SE_CANCEL
    IDR_SND_SE_PAUSE,           // SOUND_SE_PAUSE

    // SE - タイトル
    IDR_SND_SE_TITLE_SLASH,     // SOUND_SE_TITLE_SLASH

    // SE - リザルト
    IDR_SND_SE_SCORE_COUNT,     // SOUND_SE_SCORE_COUNT
    IDR_SND_SE_SCORE_FINISH,    // SOUND_SE_SCORE_FINISH
};
#endif

// 初期化
void SoundManager_Initialize()
{
    InitAudio();

#ifdef USE_ASSET_DLL
    for (int i = 0; i < SOUND_MAX; i++)
    {
        const void* pData = nullptr;
        size_t dataSize = 0;
        if (AssetDLL_GetData(SOUND_RESOURCE_IDS[i], &pData, &dataSize))
        {
            // サウンドファイルのフォーマット判定（パスの拡張子から）
            const char* fmt = "mp3"; // デフォルトはMP3
            if (SOUND_FILES[i].path != nullptr)
            {
                std::string path(SOUND_FILES[i].path);
                if (path.rfind(".wav") != std::string::npos)
                    fmt = "wav";
            }
            g_AudioHandles[i] = LoadAudioFromMemory(pData, dataSize, fmt);
        }
        else
        {
            g_AudioHandles[i] = -1;
        }
    }
#else
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
#endif

    g_CurrentBGM = SOUND_MAX;
    g_BGMVolume = 1.0f;
    g_SEVolume = 1.0f;
    g_CurrentBGMPitch = PITCH_NONE;
}

// 終了処理
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

// BGM再生
void SoundManager_PlayBGM(SoundID id)
{
    if (id < 0 || id >= SOUND_MAX) return;
    if (g_AudioHandles[id] < 0) return;

    if (g_CurrentBGM == id) return;

    SoundManager_StopBGM();

    // ボリュームとピッチを設定してから再生
    SetAudioVolume(g_AudioHandles[id], g_BGMVolume);
    g_CurrentBGMPitch = GetTargetPitch(Fever_GetTier());
    SetAudioPitch(g_AudioHandles[id], g_CurrentBGMPitch);
    PlayAudio(g_AudioHandles[id], true);
    g_CurrentBGM = id;
}

// BGM停止
void SoundManager_StopBGM()
{
    if (g_CurrentBGM != SOUND_MAX && g_AudioHandles[g_CurrentBGM] >= 0)
    {
        StopAudio(g_AudioHandles[g_CurrentBGM]);
        g_CurrentBGM = SOUND_MAX;
    }
}

// BGM音量設定
void SoundManager_SetBGMVolume(float volume)
{
    g_BGMVolume = fmaxf(0.0f, fminf(1.0f, volume));

    // 現在再生中のBGMに即座に適用
    if (g_CurrentBGM != SOUND_MAX && g_AudioHandles[g_CurrentBGM] >= 0)
    {
        SetAudioVolume(g_AudioHandles[g_CurrentBGM], g_BGMVolume);
    }
}

// SE再生
void SoundManager_PlaySE(SoundID id)
{
    if (id < 0 || id >= SOUND_MAX) return;
    if (g_AudioHandles[id] < 0) return;

    // ボリュームを設定してから再生
    SetAudioVolume(g_AudioHandles[id], g_SEVolume);
    PlayAudio(g_AudioHandles[id], false);
}

// SE音量設定
void SoundManager_SetSEVolume(float volume)
{
    g_SEVolume = fmaxf(0.0f, fminf(1.0f, volume));
}

// コンボ数に応じたSE再生
void SoundManager_PlayComboSE(int comboCount)
{
    SoundID id;

    if (comboCount >= COMBO_THRESHOLD_TIER4)
    {
        id = SOUND_SE_COMBO_4;
    }
    else if (comboCount >= COMBO_THRESHOLD_TIER3)
    {
        id = SOUND_SE_COMBO_3;
    }
    else if (comboCount >= COMBO_THRESHOLD_TIER2)
    {
        id = SOUND_SE_COMBO_2;
    }
    else
    {
        id = SOUND_SE_COMBO_1;
    }

    SoundManager_PlaySE(id);
}

// 更新（BGMピッチ補間）
void SoundManager_Update(float dt)
{
    // フィーバーティアに応じたターゲットピッチへ補間
    float target = GetTargetPitch(Fever_GetTier());
    float t = (std::min)(1.0f, PITCH_LERP_SPEED * dt);
    g_CurrentBGMPitch += (target - g_CurrentBGMPitch) * t;

    // 再生中のBGMにピッチを適用
    if (g_CurrentBGM != SOUND_MAX && g_AudioHandles[g_CurrentBGM] >= 0)
    {
        SetAudioPitch(g_AudioHandles[g_CurrentBGM], g_CurrentBGMPitch);
    }
}

// 全停止
void SoundManager_StopAll()
{
    // 全オーディオを停止
    StopAllAudio();

    // BGM状態をリセット
    g_CurrentBGM = SOUND_MAX;
}