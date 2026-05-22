/****************************************
 * @file   fade.h
 * @brief  画面のフェードイン・アウト制御
 * @author Natsume Shidara
 * @date   2025/07/10
 *
 * 画面全体を単色矩形で覆い、アルファ値を時間で変化させることで
 * フェード演出を行う。シーン遷移の繋ぎなどに利用する。
 ****************************************/
#ifndef FADE_H
#define FADE_H

#include "color.h"

//======================================
// フェード制御関数
//======================================
/** @brief フェードモジュールの初期化 */
void Fade_Initialize(void);

/** @brief フェードモジュールの終了処理 */
void Fade_Finalize(void);

/**
 * @brief  フェードの更新
 * @param  elapsed_time 前フレームからの経過時間(秒)
 */
void Fade_Update(double elapsed_time);

/** @brief フェードの描画 (画面最前面に重ねる) */
void Fade_Draw(void);

/**
 * @brief  フェードを開始する
 * @param  duration  フェードにかける秒数
 * @param  isFadeOut true:フェードアウト / false:フェードイン
 * @param  color     フェードに使う色 (既定は黒)
 */
void Fade_Start(double duration, bool isFadeOut, Color::COLOR color = Color::BLACK);

//======================================
// フェード状態
//======================================
/**
 * @enum  FADE_STATE
 * @brief フェードの進行状態
 */
enum class FADE_STATE
{
    NONE,          // フェードなし
    FADE_IN,       // フェードイン中(画面が明けていく)
    FADE_OUT,      // フェードアウト中(画面が暗転していく)
    FINISHED_IN,   // フェードイン完了
    FINISHED_OUT   // フェードアウト完了
};

/** @brief 現在のフェード状態を取得 */
FADE_STATE Fade_GetState(void);

#endif
