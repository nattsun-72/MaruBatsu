/****************************************
 * @file   l_win.cpp
 * @brief  L字勝利の実装
 * @author Natsume Shidara
 * @date   2026/06/11
 ****************************************/
#include "l_win.h"

#include "ability/ability_registry.h"
#include "board.h"

//======================================
// 構築
//======================================
LWinAbility::LWinAbility()
{
    name        = "L字勝利";
    description = "2×2の3マス支配\n(L字)でも勝利";
    rarity      = Rarity::Rare;   // 企画書準拠(レア)
    unique      = true;   // 勝利条件の追加は1つで完結するため一度限り
}

//======================================
// IWinCondition
//======================================
bool LWinAbility::Check(const Board& board, Piece player)
{
    // 1) 既存の勝利条件 (3並び等) を満たしていれば勝利
    if (chained && chained->Check(board, player)) return true;

    // 2) 対象陣営でなければ追加条件は適用しない
    if (player != targetSide) return false;

    // 3) いずれかの2×2ブロックで3マス以上を支配していれば勝利 (L字)
    for (int y = 0; y + 1 < board.height; ++y)
    {
        for (int x = 0; x + 1 < board.width; ++x)
        {
            int count = 0;
            if (board.Get(x,     y    ) == player) ++count;
            if (board.Get(x + 1, y    ) == player) ++count;
            if (board.Get(x,     y + 1) == player) ++count;
            if (board.Get(x + 1, y + 1) == player) ++count;
            if (count >= 3) return true;
        }
    }
    return false;
}

//======================================
// Ability
//======================================
void LWinAbility::RegisterTo(AbilityRegistry& registry)
{
    auto self = std::dynamic_pointer_cast<IWinCondition>(shared_from_this());
    chained = registry.SetWinCondition(self);  // 旧条件をチェインに保存
}
