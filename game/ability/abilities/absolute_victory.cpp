/****************************************
 * @file   absolute_victory.cpp
 * @brief  絶対勝利の実装
 * @author Natsume Shidara
 * @date   2026/06/19
 ****************************************/
#include "absolute_victory.h"

#include "ability/ability_registry.h"
#include "board.h"
#include "win_check.h"

//======================================
// 構築
//======================================
AbsoluteVictoryAbility::AbsoluteVictoryAbility()
{
    name        = "絶対勝利";
    description = "ライン達成は必ず勝利\n引き分けも勝利になる";
    rarity      = Rarity::Legendary;
    unique      = true;   // 確定勝利は1つで完結するため一度限り
}

//======================================
// IWinCondition
//======================================
bool AbsoluteVictoryAbility::Check(const Board& board, Piece player)
{
    if (player == targetSide)
    {
        // 1) 標準の3並び達成 → 勝利条件改変(抑制)に依らず必ず勝利。
        //    チェイン先(対角線無効などの抑制)を経由せず直接判定するため、
        //    相手が自陣の勝利条件を弱める効果を持っていても覆せる。
        if (WinCheck::HasLine(board, player, 3)) return true;

        // 2) 盤面が満杯で、どちらも決着ラインを持たない(=本来は引き分け)場合も
        //    勝利をもぎ取る。相手がラインを持つなら本来そちらが先に成立して
        //    いるため、ここでは奪わない。これにより「引き分け＝負け進行」を
        //    覆し、レジェンダリーらしい決定力を与える。
        if (board.IsFull() && !WinCheck::HasLine(board, Opponent(player), 3))
        {
            return true;
        }
    }
    // それ以外は既存の勝利条件へ委譲する
    return chained ? chained->Check(board, player) : false;
}

//======================================
// Ability
//======================================
void AbsoluteVictoryAbility::RegisterTo(AbilityRegistry& registry)
{
    auto self = std::dynamic_pointer_cast<IWinCondition>(shared_from_this());
    chained = registry.SetWinCondition(self);  // 旧条件をチェインに保存
}
