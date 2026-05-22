/****************************************
 * @file board_ops.h
 * @brief 盤面操作ユーティリティ
 * @author Natsume Shidara
 * @date 2026/05/15
 ****************************************/
#ifndef BOARD_OPS_H
#define BOARD_OPS_H

#include "direction.h"

class Board;

namespace BoardOps
{
    /** @brief 全ての駒を指定方向に端まで滑らせる (2048タイル風) */
    void SlideAll(Board& board, Direction dir);

    /** @brief 指定マスの駒を1マス滑らせる (滑り先が空かつ盤内のみ) */
    bool SlideOne(Board& board, int x, int y, Direction dir);
}

#endif // BOARD_OPS_H
