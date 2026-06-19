/****************************************
 * @file   diagonal_void.cpp
 * @brief  対角線無効の実装
 * @author Natsume Shidara
 * @date   2026/06/11
 ****************************************/
#include "diagonal_void.h"

#include "ability/ability_registry.h"
#include "board.h"
#include "win_check.h"

//======================================
// 構築
//======================================
DiagonalVoidAbility::DiagonalVoidAbility()
{
    name        = "対角線無効";
    description = "相手の斜めラインを\n勝利と認めない";
    rarity      = Rarity::Epic;   // 企画書準拠(エピック)
    unique      = true;   // 相手の斜めは1つで完全に無効化されるため一度限り
}

//======================================
// IWinCondition
//======================================
bool DiagonalVoidAbility::Check(const Board& board, Piece player)
{
    if (player != protectedSide)
    {
        // 相手側: 斜めを除いた縦横のみの並び判定で評価する。
        // ※ チェイン先(3並び等)を通さないため、相手に追加の勝利条件が
        //    付くボスが現れた場合は本実装の見直しが必要(現状は存在しない)。
        return WinCheck::HasLine(board, player, 3, false);
    }
    // 自分側: 既存の勝利条件へそのまま委譲する
    return chained ? chained->Check(board, player) : false;
}

//======================================
// Ability
//======================================
void DiagonalVoidAbility::RegisterTo(AbilityRegistry& registry)
{
    auto self = std::dynamic_pointer_cast<IWinCondition>(shared_from_this());
    chained = registry.SetWinCondition(self);  // 旧条件をチェインに保存
}
