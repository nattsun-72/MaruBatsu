/****************************************
 * @file   corner_dominion.cpp
 * @brief  四隅の覇の実装
 * @author Natsume Shidara
 * @date   2026/06/19
 ****************************************/
#include "corner_dominion.h"
#include "ability/ability_registry.h"
#include "board.h"

//======================================
// 構築
//======================================
CornerDominionAbility::CornerDominionAbility()
{
    name        = "四隅の覇";
    description = "盤面の4隅すべての\n支配でも勝利";
    rarity      = Rarity::Epic;
    unique      = true;   // 勝利条件の追加は1つで完結するため一度限り
}

//======================================
// IWinCondition
//======================================
bool CornerDominionAbility::Check(const Board& board, Piece player)
{
    // 1) 既存の勝利条件 (3並び等) を満たしていれば勝利
    if (chained && chained->Check(board, player)) return true;

    // 2) 対象陣営でなければ追加条件は適用しない
    if (player != targetSide) return false;

    // 3) 4隅すべてを自駒で支配していれば勝利
    const int x1 = board.width  - 1;
    const int y1 = board.height - 1;
    return board.Get(0,  0)  == player
        && board.Get(x1, 0)  == player
        && board.Get(0,  y1) == player
        && board.Get(x1, y1) == player;
}

//======================================
// Ability
//======================================
void CornerDominionAbility::RegisterTo(AbilityRegistry& registry)
{
    auto self = std::dynamic_pointer_cast<IWinCondition>(shared_from_this());
    chained = registry.SetWinCondition(self);  // 旧条件をチェインに保存
}
