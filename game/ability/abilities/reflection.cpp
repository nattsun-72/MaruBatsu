/****************************************
 * @file   reflection.cpp
 * @brief  鏡映の実装
 * @author Natsume Shidara
 * @date   2026/06/19
 ****************************************/
#include "reflection.h"

#include "ability/ability_registry.h"
#include "board.h"
#include "board_ops.h"
#include "game_state.h"

//======================================
// 構築
//======================================
ReflectionAbility::ReflectionAbility()
{
    name        = "鏡映";
    description = "置いた自駒が\n点対称マスにも現れる";
    rarity      = Rarity::Epic;
    unique      = true;   // 点対称化は1つで完結するため一度限り
}

//======================================
// IPlacementHandler
//======================================
void ReflectionAbility::OnPlace(GameState& state, Vec2 pos, Piece placedBy)
{
    if (placedBy != Piece::Player) return;   // 自駒の設置のみ対象

    // 盤面中心に対する点対称マス
    const int mx = (state.board.width  - 1) - pos.x;
    const int my = (state.board.height - 1) - pos.y;
    if (mx == pos.x && my == pos.y) return;          // 中央マスは対象外
    if (!state.board.IsEmpty(mx, my)) return;        // 既に埋まっていれば何もしない

    // 点対称マスへ自駒を出現させる (発生源=設置マス)
    state.board.Set(mx, my, Piece::Player);
    state.animPhases.push_back(
        { { pos.x, pos.y, mx, my, Piece::Player, true } });
}

//======================================
// Ability
//======================================
void ReflectionAbility::RegisterTo(AbilityRegistry& registry)
{
    auto self = shared_from_this();
    registry.AddPlacementHandler(std::dynamic_pointer_cast<IPlacementHandler>(self));
}
