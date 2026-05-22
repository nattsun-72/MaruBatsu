/****************************************
 * @file   reward.h
 * @brief  報酬シーン (3択UI)
 * @author Natsume Shidara
 * @date   2026/05/15
 *
 * ボス撃破後に3枚のカードからアビリティを1つ選ばせる画面。
 ****************************************/
#ifndef REWARD_H
#define REWARD_H

//======================================
// 報酬シーン API
//======================================
/** @brief 報酬シーンの初期化 (報酬候補を生成) */
void Reward_Initialize();

/** @brief 報酬シーンの終了処理 */
void Reward_Finalize();

/**
 * @brief 報酬シーンの更新 (ホバー判定・選択確定)
 * @param elapsed_time 前フレームからの経過秒数
 */
void Reward_Update(double elapsed_time);

/** @brief 報酬シーンの描画 (カード3枚) */
void Reward_Draw();

#endif // REWARD_H
