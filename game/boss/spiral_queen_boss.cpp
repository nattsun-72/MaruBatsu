/****************************************
 * @file   spiral_queen_boss.cpp
 * @brief  ボス2: 螺旋の女王 実装 (W1スタブ)
 * @author Natsume Shidara
 * @date   2026/06/05
 ****************************************/
#include "spiral_queen_boss.h"
#include "ability/abilities/contemplation.h"

//======================================
// 構築
//======================================
SpiralQueenBoss::SpiralQueenBoss()
{
    name        = "螺旋の女王";
    description = "盤面が回転する（実装予定）";
    difficulty  = 2;
}

//======================================
// Boss インターフェース実装
//======================================
std::vector<std::shared_ptr<Ability>> SpiralQueenBoss::GetBossAbilities()
{
    // W1第1増分: ギミック未実装。空を返すと通常の〇×として成立する。
    // 回転ギミック(IBoardModifier系)は後続増分でここに追加する。
    return {};
}

std::shared_ptr<Ability> SpiralQueenBoss::GetRewardAbility()
{
    // 暫定報酬。実際の報酬画面はプール抽選のため現状この戻り値は未使用。
    return std::make_shared<ContemplationAbility>();
}
