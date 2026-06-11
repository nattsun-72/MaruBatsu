/****************************************
 * @file   ice_tile.cpp
 * @brief  氷駒の実装
 * @author Natsume Shidara
 * @date   2026/05/15
 ****************************************/
#include "ice_tile.h"

#include "ability/ability_registry.h"
#include "board_ops.h"
#include "game_state.h"

//======================================
// 構築
//======================================
IceTileAbility::IceTileAbility()
{
    name        = "氷駒";
    description = "置いた自分の駒が\n1マス滑る";
    rarity      = Rarity::Rare;
}

//======================================
// IPlacementHandler
//======================================
void IceTileAbility::OnPlace(GameState& state, Vec2 pos, Piece placedBy)
{
    if (placedBy != ownerSide) return;   // 自分の駒以外は対象外

    // 重駒所持時: 自駒はあらゆる滑りの対象外 (氷駒の自滑走も含む)
    if (state.slideAnchorSide == ownerSide) return;

    /*--- 重ね掛け対応: 駒の現在位置を特定する ---*/
    // 氷駒を複数所持していると、本ハンドラは所持数だけ OnPlace される。
    // 先行コピーで既に滑った後は設置マス pos が空になっているため、
    // pos から滑走方向へ走査して「設置した駒の現在位置」を求める。
    // これにより各回で1マスずつ滑り、滑走距離が所持数に応じて伸びる。
    int cx = pos.x;
    int cy = pos.y;
    while (state.board.IsValid(cx, cy) && state.board.Get(cx, cy) == Piece::Empty)
    {
        cx += DirX(slideDir);
        cy += DirY(slideDir);
    }
    if (state.board.Get(cx, cy) != placedBy) return;  // 駒が見つからなければ何もしない

    /*--- 現在位置から1マス滑らせ、効果フェーズとして記録する(アニメ用) ---*/
    BoardOps::MoveList moves;
    if (BoardOps::SlideOne(state.board, cx, cy, slideDir, &moves))
    {
        state.animPhases.push_back(std::move(moves));
    }
}

//======================================
// Ability
//======================================
void IceTileAbility::RegisterTo(AbilityRegistry& registry)
{
    auto self = shared_from_this();
    registry.AddPlacementHandler(std::dynamic_pointer_cast<IPlacementHandler>(self));
}
