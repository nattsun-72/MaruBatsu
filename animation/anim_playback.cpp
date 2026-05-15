/****************************************
 * @file anim_playback.cpp
 * @brief アニメーション再生制御（Play/Stop/Pause/Blend/Update）
 * @author Natsume Shidara
 * @date 2025/06/06
 *
 * - 再生・停止・一時停止・逆再生
 * - アニメーションブレンド（クロスフェード）
 * - フレーム更新・ループ処理
 ****************************************/

#include "anim_playback.h"
#include "anim_bone.h"

#include <cmath>

namespace {
    constexpr double ANIM_END_EPSILON = 0.0001; // アニメーション終了判定のイプシロン
}

// 再生制御
void AnimPlayback_Play(ANIM_MODEL* model, const char* clipName, bool loop, float speed)
{
    if (!model) return;
    auto it = model->clipNameToIndex.find(clipName);
    if (it == model->clipNameToIndex.end()) return;
    AnimPlayback_PlayByIndex(model, it->second, loop, speed);
}

void AnimPlayback_PlayByIndex(ANIM_MODEL* model, int clipIndex, bool loop, float speed)
{
    if (!model) return;
    if (clipIndex < 0 || clipIndex >= static_cast<int>(model->clips.size())) return;

    model->currentClipIndex = clipIndex;
    model->currentTime = 0.0;
    model->playSpeed = speed;
    model->playState = AnimPlayState::Playing;
    model->loop = loop;
    model->pausedFromReverse = false;
    model->blend.isBlending = false;
}

void AnimPlayback_Stop(ANIM_MODEL* model)
{
    if (!model) return;
    model->playState = AnimPlayState::Stopped;
    model->currentTime = 0.0;
}

void AnimPlayback_Pause(ANIM_MODEL* model)
{
    if (!model) return;
    if (model->playState == AnimPlayState::PlayingReverse)
    {
        model->pausedFromReverse = true;
        model->playState = AnimPlayState::Paused;
    }
    else if (model->playState == AnimPlayState::Playing)
    {
        model->pausedFromReverse = false;
        model->playState = AnimPlayState::Paused;
    }
}

void AnimPlayback_Resume(ANIM_MODEL* model)
{
    if (!model) return;
    if (model->playState == AnimPlayState::Paused)
        model->playState = model->pausedFromReverse ? AnimPlayState::PlayingReverse : AnimPlayState::Playing;
}

void AnimPlayback_PlayReverse(ANIM_MODEL* model, float speed)
{
    if (!model) return;
    if (model->currentClipIndex < 0 || model->currentClipIndex >= static_cast<int>(model->clips.size())) return;
    model->playSpeed = speed;
    model->playState = AnimPlayState::PlayingReverse;
}

void AnimPlayback_SetSpeed(ANIM_MODEL* model, float speed)
{
    if (!model) return;
    model->playSpeed = speed;
}

void AnimPlayback_SetTime(ANIM_MODEL* model, double time)
{
    if (!model) return;
    model->currentTime = time;
}

// ブレンド
void AnimPlayback_BlendTo(ANIM_MODEL* model, const char* clipName, float blendDuration, bool loop)
{
    if (!model) return;
    auto it = model->clipNameToIndex.find(clipName);
    if (it == model->clipNameToIndex.end()) return;
    AnimPlayback_BlendToByIndex(model, it->second, blendDuration, loop);
}

void AnimPlayback_BlendToByIndex(ANIM_MODEL* model, int clipIndex, float blendDuration, bool loop)
{
    if (!model) return;
    if (clipIndex < 0 || clipIndex >= static_cast<int>(model->clips.size())) return;

    if (model->currentClipIndex < 0)
    {
        AnimPlayback_PlayByIndex(model, clipIndex, loop, model->playSpeed);
        return;
    }

    // ブレンド中に新しいブレンド要求 → 現在のブレンド先を確定して遷移元にする
    if (model->blend.isBlending)
    {
        // 既に同じクリップへのブレンド中ならスキップ
        if (model->blend.clipIndexB == clipIndex) return;

        // 現在のブレンド先を確定（スナップ）
        AnimationClip& prevClipB = model->clips[model->blend.clipIndexB];
        double ticksB = prevClipB.ticksPerSecond;
        model->currentClipIndex = model->blend.clipIndexB;
        model->currentTime = fmod(model->blend.blendTimer * ticksB, prevClipB.duration);
        model->blend.isBlending = false;
    }

    // 同じクリップへのブレンド要求はスキップ
    if (model->currentClipIndex == clipIndex) return;

    if (blendDuration <= 0.0f)
    {
        AnimPlayback_PlayByIndex(model, clipIndex, loop, model->playSpeed);
        return;
    }

    model->blend.clipIndexA = model->currentClipIndex;
    model->blend.clipIndexB = clipIndex;
    model->blend.blendDuration = blendDuration;
    model->blend.blendTimer = 0.0f;
    model->blend.blendFactor = 0.0f;
    model->blend.startTimeA = model->currentTime;
    model->blend.isBlending = true;

    model->loop = loop;
    model->playState = AnimPlayState::Playing;
}

// 更新
void AnimPlayback_Update(ANIM_MODEL* model, float deltaTime)
{
    if (!model) return;
    if (model->playState == AnimPlayState::Stopped || model->playState == AnimPlayState::Paused) return;
    if (model->clips.empty()) return;

    // ブレンド中
    if (model->blend.isBlending)
    {
        model->blend.blendTimer += deltaTime;
        model->blend.blendFactor = model->blend.blendTimer / model->blend.blendDuration;

        if (model->blend.blendFactor >= 1.0f)
        {
            model->currentClipIndex = model->blend.clipIndexB;
            model->blend.isBlending = false;

            AnimationClip& clipB = model->clips[model->blend.clipIndexB];
            double ticksB = clipB.ticksPerSecond;
            model->currentTime = fmod(model->blend.blendTimer * ticksB * model->playSpeed, clipB.duration);
            if (model->currentTime < 0.0) model->currentTime += clipB.duration;

            AnimBone_CalcBoneMatrices(model, &clipB, model->currentTime, nullptr, 0.0, 0.0f);
            return;
        }

        AnimationClip& clipA = model->clips[model->blend.clipIndexA];
        AnimationClip& clipB = model->clips[model->blend.clipIndexB];

        double ticksA = clipA.ticksPerSecond;
        double ticksB = clipB.ticksPerSecond;

        // startTimeA: ブレンド開始時のclipA再生時刻（tick単位）
        // playSpeed を反映してclipAを進行させる
        double timeA = fmod(model->blend.startTimeA + model->blend.blendTimer * ticksA * model->playSpeed, clipA.duration);
        if (timeA < 0.0) timeA += clipA.duration;
        double timeB = fmod(model->blend.blendTimer * ticksB * model->playSpeed, clipB.duration);
        if (timeB < 0.0) timeB += clipB.duration;

        AnimBone_CalcBoneMatrices(model, &clipA, timeA, &clipB, timeB, model->blend.blendFactor);
        return;
    }

    // 通常再生
    if (model->currentClipIndex < 0 || model->currentClipIndex >= static_cast<int>(model->clips.size()))
        return;

    AnimationClip& clip = model->clips[model->currentClipIndex];
    double ticks = clip.ticksPerSecond;

    if (model->playState == AnimPlayState::Playing)
    {
        model->currentTime += deltaTime * ticks * model->playSpeed;
    }
    else if (model->playState == AnimPlayState::PlayingReverse)
    {
        model->currentTime -= deltaTime * ticks * model->playSpeed;
    }

    // ループ処理
    if (model->loop)
    {
        if (clip.duration > 0.0)
        {
            if (model->currentTime >= clip.duration)
                model->currentTime = fmod(model->currentTime, clip.duration);
            if (model->currentTime < 0.0)
            {
                model->currentTime = fmod(model->currentTime, clip.duration) + clip.duration;
                if (model->currentTime >= clip.duration)
                    model->currentTime = 0.0;
            }
        }
        else
        {
            model->currentTime = 0.0;
        }
    }
    else
    {
        double maxTime = (clip.duration > 0.0) ? clip.duration - ANIM_END_EPSILON : 0.0;
        if (model->currentTime >= clip.duration)
        {
            model->currentTime = maxTime;
            model->playState = AnimPlayState::Stopped;
        }
        if (model->currentTime < 0.0)
        {
            model->currentTime = 0.0;
            model->playState = AnimPlayState::Stopped;
        }
    }

    AnimBone_CalcBoneMatrices(model, &clip, model->currentTime, nullptr, 0.0, 0.0f);
}
