/****************************************
 * @file   default_hooks.cpp
 * @brief  標準フック実装
 * @author Natsume Shidara
 * @date   2026/05/15
 ****************************************/
#include "default_hooks.h"
#include "board.h"
#include "win_check.h"

//======================================
// 標準勝利条件
//======================================
bool DefaultWinCondition::Check(const Board& board, Piece player)
{
    // 縦・横・斜めのいずれかに required 並びがあれば勝利
    return WinCheck::HasLine(board, player, required);
}
