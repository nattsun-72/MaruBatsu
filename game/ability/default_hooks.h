/****************************************
 * @file default_hooks.h
 * @brief 標準フック実装 (3並び勝利、1ターン1着手、3分タイマー)
 * @author Natsume Shidara
 * @date 2026/05/15
 ****************************************/
#ifndef DEFAULT_HOOKS_H
#define DEFAULT_HOOKS_H

#include "hooks.h"

/** @brief 標準勝利条件 (縦/横/斜めの required 並び) */
class DefaultWinCondition : public IWinCondition
{
public:
    int required = 3;
    bool Check(const Board& board, Piece player) override;
};

/** @brief 標準ターン処理 (1ターン1着手) */
class DefaultTurnHandler : public ITurnHandler
{
public:
    int placementCount = 1;
    int  GetPlacementCount(Piece /*player*/) override { return placementCount; }
    void OnTurnStart(GameState& /*state*/, Piece /*player*/) override {}
    void OnTurnEnd  (GameState& /*state*/, Piece /*player*/) override {}
};

/** @brief 標準ターン時間 (3分) */
class DefaultTurnTimeProvider : public ITurnTimeProvider
{
public:
    double seconds = 180.0;
    double GetTurnTimeSeconds(Piece /*player*/) override { return seconds; }
};

#endif // DEFAULT_HOOKS_H
