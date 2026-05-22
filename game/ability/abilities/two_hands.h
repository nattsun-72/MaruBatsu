/****************************************
 * @file two_hands.h
 * @brief 二手打ち — 1ターンに2個連続配置
 * @author Natsume Shidara
 * @date 2026/05/15
 ****************************************/
#ifndef ABILITY_TWO_HANDS_H
#define ABILITY_TWO_HANDS_H

#include "ability/ability.h"
#include "ability/hooks.h"

#include <memory>

class TwoHandsAbility : public Ability, public ITurnHandler
{
public:
    Piece targetSide = Piece::Player;
    int   doublePlacementCount = 2;

    // 旧 ITurnHandler を保持してチェインさせる
    std::shared_ptr<ITurnHandler> chained;

    TwoHandsAbility();

    int  GetPlacementCount(Piece player) override;
    void OnTurnStart(GameState& state, Piece player) override;
    void OnTurnEnd  (GameState& state, Piece player) override;

    void RegisterTo(AbilityRegistry& registry) override;
};

#endif // ABILITY_TWO_HANDS_H
