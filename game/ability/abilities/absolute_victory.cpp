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
    description = "ライン達成時、\n強制的に勝利する";
    rarity      = Rarity::Legendary;
    unique      = true;   // 確定勝利は1つで完結するため一度限り
}

//======================================
// IWinCondition
//======================================
bool AbsoluteVictoryAbility::Check(const Board& board, Piece player)
{
    // 自陣が標準の3並びを達成していれば、他の勝利条件改変に依らず必ず勝利。
    if (player == targetSide && WinCheck::HasLine(board, player, 3))
    {
        return true;
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
