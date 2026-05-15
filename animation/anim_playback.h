/****************************************
 * @file anim_playback.h
 * @brief アニメーション再生制御（Play/Stop/Pause/Blend/Update）
 * @author Natsume Shidara
 * @date 2025/06/06
 ****************************************/

#ifndef ANIM_PLAYBACK_H
#define ANIM_PLAYBACK_H

#include "animation_model.h"

// 再生制御
void AnimPlayback_Play(ANIM_MODEL* model, const char* clipName, bool loop, float speed);
void AnimPlayback_PlayByIndex(ANIM_MODEL* model, int clipIndex, bool loop, float speed);
void AnimPlayback_Stop(ANIM_MODEL* model);
void AnimPlayback_Pause(ANIM_MODEL* model);
void AnimPlayback_Resume(ANIM_MODEL* model);
void AnimPlayback_PlayReverse(ANIM_MODEL* model, float speed);
void AnimPlayback_SetSpeed(ANIM_MODEL* model, float speed);
void AnimPlayback_SetTime(ANIM_MODEL* model, double time);

// ブレンド
void AnimPlayback_BlendTo(ANIM_MODEL* model, const char* clipName, float blendDuration, bool loop);
void AnimPlayback_BlendToByIndex(ANIM_MODEL* model, int clipIndex, float blendDuration, bool loop);

// 更新
void AnimPlayback_Update(ANIM_MODEL* model, float deltaTime);

#endif // ANIM_PLAYBACK_H
