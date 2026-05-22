/****************************************
 * @file ice_tile.cpp
 * @brief 氷駒の実装
 * @author Natsume Shidara
 * @date 2026/05/15
 ****************************************/
#include "ice_tile.h"

#include "ability/ability_registry.h"
#include "board_ops.h"
#include "game_state.h"

IceTileAbility::IceTileAbility()
{
    name        = "Ice Tile";
    description = "Your placed piece slides 1 cell on placement.";
    rarity      = Rarity::Rare;
}

void IceTileAbility::OnPlace(GameState& state, Vec2 pos, Piece placedBy)
{
    if (placedBy != ownerSide) return;
    // 設置直後の位置から1マス滑らせる
    BoardOps::SlideOne(state.board, pos.x, pos.y, slideDir);
}

void IceTileAbility::RegisterTo(AbilityRegistry& registry)
{
    auto self = shared_from_this();
    registry.AddPlacementHandler(std::dynamic_pointer_cast<IPlacementHandler>(self));
}
