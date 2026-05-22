/****************************************
 * @file line_added.cpp
 * @brief ライン本数追加の実装
 * @author Natsume Shidara
 * @date 2026/05/15
 ****************************************/
#include "line_added.h"
#include "ability/ability_registry.h"
#include "board.h"

LineAddedAbility::LineAddedAbility()
{
    name        = "Line Added";
    description = "You also win by controlling 3+ corners.";
    rarity      = Rarity::Rare;
}

bool LineAddedAbility::Check(const Board& board, Piece player)
{
    if (chained && chained->Check(board, player)) return true;

    if (player != targetSide && targetSide != Piece::Empty) return false;

    // 4コーナーの自分の駒数を数える
    const Piece corners[4] = {
        board.Get(0,                  0),
        board.Get(board.width  - 1,   0),
        board.Get(0,                  board.height - 1),
        board.Get(board.width  - 1,   board.height - 1),
    };
    int count = 0;
    for (Piece c : corners)
    {
        if (c == player) ++count;
    }
    return count >= cornerThreshold;
}

void LineAddedAbility::RegisterTo(AbilityRegistry& registry)
{
    auto self = std::dynamic_pointer_cast<IWinCondition>(shared_from_this());
    chained = registry.SetWinCondition(self);  // 旧条件 (3並び等) をチェイン
}
