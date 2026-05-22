/****************************************
 * @file run_state.cpp
 * @brief RunState 実装
 * @author Natsume Shidara
 * @date 2026/05/15
 ****************************************/
#include "run_state.h"
#include "ability/ability_pool.h"

namespace
{
    std::vector<std::shared_ptr<Ability>> g_PlayerAbilities;
    std::vector<std::shared_ptr<Ability>> g_RewardChoices;
    int g_BossIndex = 0;
}

namespace RunState
{
    std::vector<std::shared_ptr<Ability>>& PlayerAbilities() { return g_PlayerAbilities; }
    std::vector<std::shared_ptr<Ability>>& RewardChoices()  { return g_RewardChoices;  }

    int  CurrentBossIndex() { return g_BossIndex; }
    void IncrementBoss()    { ++g_BossIndex; }

    void ResetRun()
    {
        g_PlayerAbilities.clear();
        g_RewardChoices.clear();
        g_BossIndex = 0;
    }

    void GenerateRewardChoices()
    {
        g_RewardChoices = AbilityPool::PickRandom(3);
    }
}
