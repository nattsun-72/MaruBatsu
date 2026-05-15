/****************************************
 * @file hitstop.h
 * @brief ヒットストップ（時間停止演出）
 * @detail 攻撃命中時に一瞬ゲーム時間を停止し、打撃の手応えを強調する
 * @author Natsume Shidara
 * @date 2026/02/22
 ****************************************/

#ifndef HITSTOP_H
#define HITSTOP_H

/** @brief ヒットストップ初期化 */
void HitStop_Initialize();

/** @brief ヒットストップ終了処理 */
void HitStop_Finalize();

/** @brief ヒットストップ更新
 *  @param raw_dt スケーリング前の生デルタタイム */
void HitStop_Update(float raw_dt);

/** @brief ヒットストップ発動（攻撃命中時に呼ぶ） */
void HitStop_Trigger();

/** @brief 現在の時間スケールを取得（0.0〜1.0） */
float HitStop_GetTimeScale();

/** @brief ヒットストップ中か */
bool HitStop_IsActive();

#endif // HITSTOP_H
