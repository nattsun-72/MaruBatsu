/****************************************
 * @file contemplation.cpp
 * @brief 熟考の実装
 * @author Natsume Shidara
 * @date 2026/05/15
 ****************************************/
#include "contemplation.h"
#include "ability/ability_registry.h"

ContemplationAbility::ContemplationAbility()
{
    name        = "Contemplation";
    description = "Player turn time +30s (permanent).";
    rarity      = Rarity::Common;
}

double ContemplationAbility::GetTurnTimeSeconds(Piece player)
{
    const double base = chained ? chained->GetTurnTimeSeconds(player) : baseSeconds;
    if (player == targetSide) return base + bonusSeconds;
    return base;
}

void ContemplationAbility::RegisterTo(AbilityRegistry& registry)
{
    auto self = std::dynamic_pointer_cast<ITurnTimeProvider>(shared_from_this());
    chained = registry.SetTurnTimeProvider(self);  // 旧実装をチェインに保存
}
