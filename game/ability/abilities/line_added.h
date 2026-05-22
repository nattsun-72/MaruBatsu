/****************************************
 * @file line_added.h
 * @brief ライン本数追加 — 勝利条件に4コーナー判定を追加
 * @author Natsume Shidara
 * @date 2026/05/15
 *
 * プロト段階では「3並び勝利 OR 3コーナー支配」を追加し、
 * IWinCondition フックの差し替え動作を検証する。
 * 3x3盤面で勝利機会を +1 する。
 ****************************************/
#ifndef ABILITY_LINE_ADDED_H
#define ABILITY_LINE_ADDED_H

#include "ability/ability.h"
#include "ability/hooks.h"

#include <memory>

class LineAddedAbility : public Ability, public IWinCondition
{
public:
    int   cornerThreshold = 3;     // 何個コーナー支配で勝利か
    Piece targetSide      = Piece::Player;

    // 既存条件をチェインしてOR判定する
    std::shared_ptr<IWinCondition> chained;

    LineAddedAbility();

    bool Check(const Board& board, Piece player) override;
    void RegisterTo(AbilityRegistry& registry) override;
};

#endif // ABILITY_LINE_ADDED_H
