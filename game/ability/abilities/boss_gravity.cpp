/****************************************
 * @file   boss_gravity.cpp
 * @brief  重力場 (ボス側アビリティ) の実装
 * @author Natsume Shidara
 * @date   2026/06/11
 ****************************************/
#include "boss_gravity.h"

#include "ability/ability_registry.h"
#include "board.h"
#include "board_ops.h"
#include "game_state.h"

//======================================
// 構築
//======================================
BossGravityAbility::BossGravityAbility()
{
    name        = "重力場";
    description = "置かれた駒は\n列の底まで落下する";
    rarity      = Rarity::Rare;
}

//======================================
// IPlacementHandler
//======================================
void BossGravityAbility::OnPlace(GameState& state, Vec2 pos, Piece placedBy)
{
    // 重駒所持時: その陣営の駒は重力の影響を受けない
    if (state.slideAnchorSide == placedBy) return;

    /*--- 設置駒の現在位置を特定する ---*/
    // 先行フック(氷駒等)で既に滑った後は設置マスが空のため、
    // 設置マスから下方向へ走査して駒の現在位置を求める。
    int cy = pos.y;
    while (state.board.IsValid(pos.x, cy)
        && state.board.Get(pos.x, cy) == Piece::Empty)
    {
        ++cy;
    }
    if (state.board.Get(pos.x, cy) != placedBy) return;  // 見つからなければ何もしない

    /*--- 列の底まで落下させ、効果フェーズとして記録する(アニメ用) ---*/
    BoardOps::MoveList moves;
    if (BoardOps::DropDown(state.board, pos.x, cy, &moves))
    {
        state.animPhases.push_back(std::move(moves));
    }
}

//======================================
// Ability
//======================================
void BossGravityAbility::RegisterTo(AbilityRegistry& registry)
{
    // 設置ハンドラとして追加登録 (他の設置効果と共存可能)
    auto self = shared_from_this();
    registry.AddPlacementHandler(std::dynamic_pointer_cast<IPlacementHandler>(self));
}
