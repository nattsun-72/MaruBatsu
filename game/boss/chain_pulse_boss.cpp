/****************************************
 * @file   chain_pulse_boss.cpp
 * @brief  ボス3: 連鎖の鼓動 実装
 * @author Natsume Shidara
 * @date   2026/06/05
 ****************************************/
#include "chain_pulse_boss.h"
#include "ability/abilities/boss_chain_pulse.h"
#include "ability/abilities/chain_shard.h"

//======================================
// 構築
//======================================
ChainPulseBoss::ChainPulseBoss()
{
    name        = "連鎖の鼓動";
    description = "ボスが駒を置くたび、隣接マスへ拡張する";
    difficulty  = 3;
    // テーマ色: 深い赤 (鼓動)
    bgR = 0.14f; bgG = 0.04f; bgB = 0.05f;
}

//======================================
// Boss インターフェース実装
//======================================
std::vector<std::shared_ptr<Ability>> ChainPulseBoss::GetBossAbilities()
{
    // ボス側ギミック: ボスの設置のたび隣接マスへ同色駒を自動拡張させる
    return { std::make_shared<BossChainPulseAbility>() };
}

std::shared_ptr<Ability> ChainPulseBoss::GetRewardAbility()
{
    // 撃破報酬: 装填式の連鎖拡張に弱化した「連鎖の欠片」
    return std::make_shared<ChainShardAbility>();
}
