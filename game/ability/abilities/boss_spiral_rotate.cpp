/****************************************
 * @file   boss_spiral_rotate.cpp
 * @brief  螺旋盤 (ボス側アビリティ) の実装
 * @author Natsume Shidara
 * @date   2026/06/05
 ****************************************/
#include "boss_spiral_rotate.h"

#include "ability/ability_registry.h"
#include "board_ops.h"
#include "game_state.h"

//======================================
// 構築
//======================================
BossSpiralRotateAbility::BossSpiralRotateAbility()
{
    name        = "螺旋盤";
    description = "駒を置くたび\n盤面が90度回転する";
    rarity      = Rarity::Rare;
}

//======================================
// IPlacementHandler
//======================================
void BossSpiralRotateAbility::OnPlace(GameState& state, Vec2 /*pos*/, Piece /*placedBy*/)
{
    // 盤面を90度回転し、その移動を1つの効果フェーズとして記録する(アニメ用)。
    BoardOps::MoveList moves;
    BoardOps::Rotate90(state.board, clockwise, &moves);
    if (!moves.empty())
    {
        state.animPhases.push_back(std::move(moves));
    }
}

//======================================
// Ability
//======================================
void BossSpiralRotateAbility::RegisterTo(AbilityRegistry& registry)
{
    // 設置ハンドラとして追加登録 (氷盤など他の設置効果と共存可能)
    auto self = shared_from_this();
    registry.AddPlacementHandler(std::dynamic_pointer_cast<IPlacementHandler>(self));
}
