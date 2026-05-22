/****************************************
 * @file boss_ice_slide.h
 * @brief 氷盤の支配者 ボス側アビリティ
 * @author Natsume Shidara
 * @date 2026/05/15
 *
 * 仕様: 駒が置かれると全ての駒が指定方向に端まで滑る。
 *       方向はターン開始時にランダム決定。
 ****************************************/
#ifndef ABILITY_BOSS_ICE_SLIDE_H
#define ABILITY_BOSS_ICE_SLIDE_H

#include "ability/ability.h"
#include "ability/hooks.h"
#include "direction.h"

#include <memory>

class BossIceSlideAbility
    : public Ability
    , public IPlacementHandler
    , public ITurnHandler
{
public:
    Direction currentDir = Direction::Right;

    // 旧 ITurnHandler (デフォルト) をチェインで保持
    std::shared_ptr<ITurnHandler> chainedTurn;

    BossIceSlideAbility();

    // IPlacementHandler
    void OnPlace(GameState& state, Vec2 pos, Piece placedBy) override;

    // ITurnHandler
    int  GetPlacementCount(Piece player) override;
    void OnTurnStart(GameState& state, Piece player) override;
    void OnTurnEnd  (GameState& state, Piece player) override;

    // Ability
    void RegisterTo(AbilityRegistry& registry) override;
};

#endif // ABILITY_BOSS_ICE_SLIDE_H
