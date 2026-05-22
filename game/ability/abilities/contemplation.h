/****************************************
 * @file contemplation.h
 * @brief 熟考 (Contemplation) — 思考時間 +30s (恒久)
 * @author Natsume Shidara
 * @date 2026/05/15
 *
 * ITurnTimeProvider をラップして、プレイヤーのターン時間に
 * bonusSeconds だけ追加する。
 ****************************************/
#ifndef ABILITY_CONTEMPLATION_H
#define ABILITY_CONTEMPLATION_H

#include "ability/ability.h"
#include "ability/hooks.h"

#include <memory>

class ContemplationAbility : public Ability, public ITurnTimeProvider
{
public:
    double bonusSeconds = 30.0;
    double baseSeconds  = 180.0;
    Piece  targetSide   = Piece::Player;

    // 既存のプロバイダを保持しておき、上書きしつつ加算する
    std::shared_ptr<ITurnTimeProvider> chained;

    ContemplationAbility();

    double GetTurnTimeSeconds(Piece player) override;
    void   RegisterTo(AbilityRegistry& registry) override;
};

#endif // ABILITY_CONTEMPLATION_H
