/****************************************
 * @file   boss_reward.h
 * @brief  ボス撃破スキル獲得シーン (はい/いいえ)
 * @author Natsume Shidara
 * @date   2026/06/24
 *
 * 通常ボス撃破時に「「○○」を撃破！スキル「△△」を獲得しますか？」を提示し、
 * ボス/スキルの説明を見せたうえで獲得可否(はい/いいえ)を選ばせる。
 * いずれを選んでも、その後に通常抽選(REWARD)へ進む。
 ****************************************/
#ifndef BOSS_REWARD_H
#define BOSS_REWARD_H

//======================================
// ボス撃破スキル獲得シーン API
//======================================
/** @brief 初期化 (保留中のボススキルを参照) */
void BossReward_Initialize();

/** @brief 終了処理 */
void BossReward_Finalize();

/**
 * @brief 更新 (はい/いいえ の選択処理)
 * @param elapsed_time 前フレームからの経過秒数
 */
void BossReward_Update(double elapsed_time);

/** @brief 描画 (撃破見出し + スキル説明 + はい/いいえ) */
void BossReward_Draw();

#endif // BOSS_REWARD_H
