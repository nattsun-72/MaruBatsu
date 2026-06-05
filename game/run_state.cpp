/****************************************
 * @file   run_state.cpp
 * @brief  ラン進行状態の実装
 * @author Natsume Shidara
 * @date   2026/05/15
 ****************************************/
#include "run_state.h"
#include "ability/ability_pool.h"
#include "boss/boss_roster.h"

#include <string>
#include <vector>

//--------------------------------------
// 内部状態
//--------------------------------------
namespace
{
    std::vector<std::shared_ptr<Ability>> g_PlayerAbilities;  // 所持アビリティ(累積)
    std::vector<std::shared_ptr<Ability>> g_RewardChoices;    // 報酬画面の3択
    int  g_BossIndex   = 0;                                   // 現在挑戦中のボスindex
    bool g_RunCleared  = false;                               // 直近ランをクリアしたか(制覇表示用)
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

    int  BossCount()        { return BossRoster::Count(); }
    bool IsRunComplete()    { return g_BossIndex >= BossCount(); }

    bool IsRunCleared()     { return g_RunCleared; }
    void MarkRunCleared()   { g_RunCleared = true; }

    //======================================
    // ラン制御
    //======================================
    void ResetRun()
    {
        // 所持アビリティ・報酬候補・ボス進行・クリア表示をすべて初期化
        g_PlayerAbilities.clear();
        g_RewardChoices.clear();
        g_BossIndex   = 0;
        g_RunCleared  = false;
    }

    void GenerateRewardChoices()
    {
        // 取得済みの「一度限り」アビリティ名を集め、抽選候補から除外する
        std::vector<std::string> excludeNames;
        for (const auto& a : g_PlayerAbilities)
        {
            if (a->unique) excludeNames.push_back(a->name);
        }
        // アビリティプールから3つをランダム抽選して報酬候補とする
        g_RewardChoices = AbilityPool::PickRandom(3, excludeNames);
    }
}
