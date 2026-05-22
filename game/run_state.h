/****************************************
 * @file run_state.h
 * @brief 1ラン (5戦) の進行状態をシーン間で共有
 * @author Natsume Shidara
 * @date 2026/05/15
 *
 * Game/Reward シーンは GrantPrototypeAbilities 等のローカル状態を
 * 持たず、本モジュールを参照する。α版で RunManager クラスに
 * 統合する前提のシンプルな名前空間API。
 ****************************************/
#ifndef RUN_STATE_H
#define RUN_STATE_H

#include "ability/ability.h"

#include <memory>
#include <vector>

namespace RunState
{
    /** @brief プレイヤー所持アビリティ (累積) */
    std::vector<std::shared_ptr<Ability>>& PlayerAbilities();

    /** @brief 現在の報酬画面で提示している3択 */
    std::vector<std::shared_ptr<Ability>>& RewardChoices();

    /** @brief 現在挑戦中のボスindex (0=ボス1, ..., 4=ラスボス) */
    int  CurrentBossIndex();
    void IncrementBoss();

    /** @brief ラン状態を完全リセット (タイトルから新ラン開始時) */
    void ResetRun();

    /** @brief 報酬3択を生成 (プールからランダム抽選) */
    void GenerateRewardChoices();
}

#endif // RUN_STATE_H
