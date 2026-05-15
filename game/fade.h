/****************************************
 * @file fade.h
 * @brief フェードイン・アウト制御
 * @author Natsume Shidara
 * @date 2025/07/10
 ****************************************/

#ifndef FADE_H
#define FADE_H
#include "color.h"

/** @brief フェード初期化 */
void Fade_Initialize(void);

/** @brief フェード終了処理 */
void Fade_Finalize(void);

/** @brief フェード更新 */
void Fade_Update(double elapsed_time);

/** @brief フェード描画 */
void Fade_Draw(void);

/** @brief フェード開始 */
void Fade_Start(double duration, bool isFadeOut, Color::COLOR color = Color::BLACK);

/** @brief フェード状態 */
enum class FADE_STATE
{
    NONE,
    FADE_IN,
    FADE_OUT,
    FINISHED_IN,
    FINISHED_OUT
};

/** @brief 現在のフェード状態を取得 */
FADE_STATE Fade_GetState(void);

#endif
