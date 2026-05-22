/****************************************
 * @file default_hooks.cpp
 * @brief 標準フック実装
 * @author Natsume Shidara
 * @date 2026/05/15
 ****************************************/
#include "default_hooks.h"
#include "board.h"
#include "win_check.h"

bool DefaultWinCondition::Check(const Board& board, Piece player)
{
    return WinCheck::HasLine(board, player, required);
}
