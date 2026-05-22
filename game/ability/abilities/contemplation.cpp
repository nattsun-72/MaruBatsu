/****************************************
 * @file   contemplation.cpp
 * @brief  熟考の実装
 * @author Natsume Shidara
 * @date   2026/05/15
 ****************************************/
#include "contemplation.h"
#include "ability/ability_registry.h"

//======================================
// 構築
//======================================
ContemplationAbility::ContemplationAbility()
{
    name        = "熟考";
    description = "毎ターン思考時間\n+30秒 (恒久)";
    rarity      = Rarity::Common;
}

//======================================
// ITurnTimeProvider
//======================================
double ContemplationAbility::GetTurnTimeSeconds(Piece player)
{
    // チェイン先があればその値を、無ければ baseSeconds を基準にする
    const double base = chained ? chained->GetTurnTimeSeconds(player) : baseSeconds;
    if (player == targetSide) return base + bonusSeconds;
    return base;
}

//======================================
// Ability
//======================================
void ContemplationAbility::RegisterTo(AbilityRegistry& registry)
{
    auto self = std::dynamic_pointer_cast<ITurnTimeProvider>(shared_from_this());
    chained = registry.SetTurnTimeProvider(self);  // 旧実装をチェインに保存
}
