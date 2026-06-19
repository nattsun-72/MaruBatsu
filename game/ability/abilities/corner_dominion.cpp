/****************************************
 * @file   corner_dominion.cpp
 * @brief  四隅の覇の実装
 * @author Natsume Shidara
 * @date   2026/06/19
 ****************************************/
#include "corner_dominion.h"
#include "ability/ability_registry.h"
#include "board.h"
#include "config.h"

#include <cstdio>

//======================================
// 構築
//======================================
CornerDominionAbility::CornerDominionAbility()
{
    name            = "隅取り";
    rarity          = Rarity::Rare;   // 勝利条件追加系はレア帯に統一
    unique          = true;   // 勝利条件の追加は1つで完結するため一度限り
    cornerThreshold = Config::GetInt("abilities.cornerDominion.threshold", 3);

    char buf[64];
    std::snprintf(buf, sizeof(buf), "隅を%d箇所 支配\nしても勝利", cornerThreshold);
    description = buf;
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

    // 3) 4隅のうち自駒で支配している数が しきい値以上 なら勝利
    const int x1 = board.width  - 1;
    const int y1 = board.height - 1;
    const Piece corners[4] = {
        board.Get(0,  0),
        board.Get(x1, 0),
        board.Get(0,  y1),
        board.Get(x1, y1),
    };
    int count = 0;
    for (Piece c : corners)
    {
        if (c == player) ++count;
    }
    return count >= cornerThreshold;
}

//======================================
// Ability
//======================================
void CornerDominionAbility::RegisterTo(AbilityRegistry& registry)
{
    auto self = std::dynamic_pointer_cast<IWinCondition>(shared_from_this());
    chained = registry.SetWinCondition(self);  // 旧条件をチェインに保存
}
