/****************************************
 * @file   spiral_queen_boss.cpp
 * @brief  ボス2: 螺旋の女王 実装 (W1スタブ)
 * @author Natsume Shidara
 * @date   2026/06/05
 ****************************************/
#include "spiral_queen_boss.h"
#include "ability/abilities/boss_spiral_rotate.h"
#include "ability/abilities/spiral_shard.h"

//======================================
// 構築
//======================================
SpiralQueenBoss::SpiralQueenBoss()
{
    name        = "螺旋の女王";
    description = "駒を置くたび、盤面が90度回転する";
    difficulty  = 2;
    // テーマ色: 深い紫 (螺旋)
    bgR = 0.10f; bgG = 0.04f; bgB = 0.14f;
}

//======================================
// Boss インターフェース実装
//======================================
std::vector<std::shared_ptr<Ability>> SpiralQueenBoss::GetBossAbilities()
{
    // ボス側ギミック: 設置のたび盤面を90度回転させる
    return { std::make_shared<BossSpiralRotateAbility>() };
}

std::shared_ptr<Ability> SpiralQueenBoss::GetRewardAbility()
{
    // 撃破報酬: 任意発動の盤面回転に弱化した「螺旋の欠片」
    return std::make_shared<SpiralShardAbility>();
}
