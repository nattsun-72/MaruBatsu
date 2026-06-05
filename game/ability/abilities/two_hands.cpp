/****************************************
 * @file   two_hands.cpp
 * @brief  二手打ちの実装
 * @author Natsume Shidara
 * @date   2026/05/15
 ****************************************/
#include "two_hands.h"
#include "ability/ability_registry.h"

//======================================
// 構築
//======================================
TwoHandsAbility::TwoHandsAbility()
{
    name        = "二手打ち";
    description = "1ターンに2回\n連続で配置できる";
    rarity      = Rarity::Rare;
    unique      = true;   // 着手数は2に固定で、2個目を取得しても増えないため一度限り
}

//======================================
// ITurnHandler
//======================================
int TwoHandsAbility::GetPlacementCount(Piece player)
{
    // 対象陣営は2着手、それ以外はチェイン先(デフォルト等)へ委譲
    if (player == targetSide) return doublePlacementCount;
    return chained ? chained->GetPlacementCount(player) : 1;
}

void TwoHandsAbility::OnTurnStart(GameState& state, Piece player)
{
    if (chained) chained->OnTurnStart(state, player);
}

void TwoHandsAbility::OnTurnEnd(GameState& state, Piece player)
{
    if (chained) chained->OnTurnEnd(state, player);
}

//======================================
// Ability
//======================================
void TwoHandsAbility::RegisterTo(AbilityRegistry& registry)
{
    auto self = std::dynamic_pointer_cast<ITurnHandler>(shared_from_this());
    chained = registry.SetTurnHandler(self);  // 旧ハンドラ (BossIce等) をチェイン
}
