/****************************************
 * @file sound_manager.h
 * @brief ゲーム全体のサウンド管理 (2D版・最小構成)
 * @author Natsume Shidara
 * @date 2026/01/13
 * @update 2026/05/15 - 〇×ローグライト用にスリム化 (BGM/UI SE のみ)
 ****************************************/

#ifndef SOUND_MANAGER_H
#define SOUND_MANAGER_H

// サウンドID定義
enum SoundID
{
    // BGM
    SOUND_BGM_TITLE,
    SOUND_BGM_GAME,
    SOUND_BGM_RESULT,

    // SE - UI（β以降で実装）
    SOUND_SE_SELECT,
    SOUND_SE_DECIDE,
    SOUND_SE_CANCEL,
    SOUND_SE_PAUSE,

    // 総数
    SOUND_MAX
};

// 初期化・終了
void SoundManager_Initialize();
void SoundManager_Finalize();

// BGM制御
void SoundManager_PlayBGM(SoundID id);
void SoundManager_StopBGM();
void SoundManager_SetBGMVolume(float volume);  // 0.0 ~ 1.0

// SE再生
void SoundManager_PlaySE(SoundID id);
void SoundManager_SetSEVolume(float volume);   // 0.0 ~ 1.0

// 更新
void SoundManager_Update(float dt);

// 一括停止
void SoundManager_StopAll();

#endif // SOUND_MANAGER_H
