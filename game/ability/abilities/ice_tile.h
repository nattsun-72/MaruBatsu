/****************************************
 * @file ice_tile.h
 * @brief 氷駒 (Ice Tile) — プレイヤー側アビリティ
 * @author Natsume Shidara
 * @date 2026/05/15
 *
 * 仕様: プレイヤーが置いた駒のみ、設置時に1マス分滑る (方向は固定)。
 * プロト段階では方向選択UIは無く Direction::Down に固定する。
 * ボス1 (氷盤の支配者) の弱化版報酬。
 ****************************************/
#ifndef ABILITY_ICE_TILE_H
#define ABILITY_ICE_TILE_H

#include "ability/ability.h"
#include "ability/hooks.h"
#include "direction.h"

class IceTileAbility : public Ability, public IPlacementHandler
{
public:
    Direction slideDir = Direction::Down;
    Piece     ownerSide = Piece::Player;  // この駒の効果対象

    IceTileAbility();

    void OnPlace(GameState& state, Vec2 pos, Piece placedBy) override;
    void RegisterTo(AbilityRegistry& registry) override;
};

#endif // ABILITY_ICE_TILE_H
