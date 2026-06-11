/****************************************
 * @file   heavy_piece.cpp
 * @brief  重駒の実装
 * @author Natsume Shidara
 * @date   2026/06/11
 ****************************************/
#include "heavy_piece.h"

#include "ability/ability_registry.h"
#include "game_state.h"

//======================================
// 構築
//======================================
HeavyPieceAbility::HeavyPieceAbility()
{
    name        = "重駒";
    description = "自駒が重くなり\n滑り・落下を受けない";
    rarity      = Rarity::Epic;
    unique      = true;   // 全自駒に効くため2個目は無意味
}

//======================================
// ITurnHandler
//======================================
int HeavyPieceAbility::GetPlacementCount(Piece player)
{
    return chained ? chained->GetPlacementCount(player) : 1;
}

void HeavyPieceAbility::OnTurnStart(GameState& state, Piece player)
{
    if (chained) chained->OnTurnStart(state, player);
    // 毎ターン宣言し直す (GameState は試合ごとに初期化されるため)
    state.slideAnchorSide = Piece::Player;
}

void HeavyPieceAbility::OnTurnEnd(GameState& state, Piece player)
{
    if (chained) chained->OnTurnEnd(state, player);
}

//======================================
// Ability
//======================================
void HeavyPieceAbility::RegisterTo(AbilityRegistry& registry)
{
    auto self = std::dynamic_pointer_cast<ITurnHandler>(shared_from_this());
    chained = registry.SetTurnHandler(self);  // 旧ハンドラをチェイン
}
