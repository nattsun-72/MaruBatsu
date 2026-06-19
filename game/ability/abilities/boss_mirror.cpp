/****************************************
 * @file   boss_mirror.cpp
 * @brief  鏡写し (ボス側アビリティ) の実装
 * @author Natsume Shidara
 * @date   2026/06/19
 ****************************************/
#include "boss_mirror.h"

#include "ability/ability_registry.h"
#include "board.h"
#include "board_ops.h"
#include "game_state.h"

//======================================
// 構築
//======================================
BossMirrorAbility::BossMirrorAbility()
{
    name        = "鏡写し";
    description = "あなたの設置を\n点対称にミラーする";
    rarity      = Rarity::Rare;
}

//======================================
// IPlacementHandler
//======================================
void BossMirrorAbility::OnPlace(GameState& state, Vec2 pos, Piece placedBy)
{
    if (placedBy != Piece::Player) return;   // プレイヤー設置のみミラーする

    // 盤面中心に対する点対称マス
    const int mx = (state.board.width  - 1) - pos.x;
    const int my = (state.board.height - 1) - pos.y;
    if (mx == pos.x && my == pos.y) return;          // 中央マスは対象外
    if (!state.board.IsEmpty(mx, my)) return;        // 既に埋まっていれば何もしない

    // 点対称マスへボス駒を出現させる (発生源=プレイヤーの設置マス)
    state.board.Set(mx, my, Piece::Enemy);
    state.animPhases.push_back(
        { { pos.x, pos.y, mx, my, Piece::Enemy, true } });
}

//======================================
// Ability
//======================================
void BossMirrorAbility::RegisterTo(AbilityRegistry& registry)
{
    auto self = shared_from_this();
    registry.AddPlacementHandler(std::dynamic_pointer_cast<IPlacementHandler>(self));
}
