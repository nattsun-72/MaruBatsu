/****************************************
 * @file win_check.h
 * @brief 勝利条件判定 (Day 2 — ハードコード版)
 * @author Natsume Shidara
 * @date 2026/05/15
 *
 * Day 3 で IWinCondition インターフェイスに置き換える前提の
 * 暫定実装。3並び (縦・横・斜め) を全て走査する。
 ****************************************/
#ifndef WIN_CHECK_H
#define WIN_CHECK_H

#include "piece.h"

class Board;

namespace WinCheck
{
    /** @brief player が必要本数(デフォルト3)並びを達成したか判定 */
    bool HasLine(const Board& board, Piece player, int required = 3);

    /** @brief player の最大連続長 (任意方向) を取得 */
    int CountMaxLine(const Board& board, Piece player);
}

#endif // WIN_CHECK_H
