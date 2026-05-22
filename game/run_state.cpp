/****************************************
 * @file   run_state.cpp
 * @brief  ラン進行状態の実装
 * @author Natsume Shidara
 * @date   2026/05/15
 ****************************************/
#include "run_state.h"
#include "ability/ability_pool.h"

//--------------------------------------
// 内部状態
//--------------------------------------
namespace
{
    std::vector<std::shared_ptr<Ability>> g_PlayerAbilities;  // 所持アビリティ(累積)
    std::vector<std::shared_ptr<Ability>> g_RewardChoices;    // 報酬画面の3択
    int g_BossIndex = 0;                                      // 現在挑戦中のボスindex
}

namespace RunState
{
    //======================================
    // アクセサ
    //======================================
    std::vector<std::shared_ptr<Ability>>& PlayerAbilities() { return g_PlayerAbilities; }
    std::vector<std::shared_ptr<Ability>>& RewardChoices()  { return g_RewardChoices;  }

    int  CurrentBossIndex() { return g_BossIndex; }
    void IncrementBoss()    { ++g_BossIndex; }

    //======================================
    // ラン制御
    //======================================
    void ResetRun()
    {
        // 所持アビリティ・報酬候補・ボス進行をすべて初期化
        g_PlayerAbilities.clear();
        g_RewardChoices.clear();
        g_BossIndex = 0;
    }

    void GenerateRewardChoices()
    {
        // アビリティプールから3つをランダム抽選して報酬候補とする
        g_RewardChoices = AbilityPool::PickRandom(3);
    }
}
