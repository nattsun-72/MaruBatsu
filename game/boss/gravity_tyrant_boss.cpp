/****************************************
 * @file   gravity_tyrant_boss.cpp
 * @brief  ボス4: 重力の専制者 実装 (W1スタブ)
 * @author Natsume Shidara
 * @date   2026/06/05
 ****************************************/
#include "gravity_tyrant_boss.h"
#include "ability/abilities/contemplation.h"

//======================================
// 構築
//======================================
GravityTyrantBoss::GravityTyrantBoss()
{
    name        = "重力の専制者";
    description = "重い駒で盤面を支配する（実装予定）";
    difficulty  = 4;
}

//======================================
// Boss インターフェース実装
//======================================
std::vector<std::shared_ptr<Ability>> GravityTyrantBoss::GetBossAbilities()
{
    // W1第1増分: ギミック未実装。空を返すと通常の〇×として成立する。
    // 駒強化ギミック(IPieceModifier系)は後続週でここに追加する。
    return {};
}

std::shared_ptr<Ability> GravityTyrantBoss::GetRewardAbility()
{
    // 暫定報酬。実際の報酬画面はプール抽選のため現状この戻り値は未使用。
    return std::make_shared<ContemplationAbility>();
}
