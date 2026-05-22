/****************************************
 * @file board.h
 * @brief 盤面データ
 * @author Natsume Shidara
 * @date 2026/05/15
 *
 * width × height のマス目で駒を管理。
 * 1次元 vector で保持 (index = y * width + x)。
 ****************************************/
#ifndef BOARD_H
#define BOARD_H

#include "piece.h"
#include <vector>

class Board
{
public:
    int width  = 3;
    int height = 3;
    std::vector<Piece> cells;

    Board();
    explicit Board(int w, int h);

    void Reset();
    void Resize(int w, int h);

    Piece Get(int x, int y) const;
    void  Set(int x, int y, Piece p);

    bool  IsValid(int x, int y) const;
    bool  IsEmpty(int x, int y) const;
    bool  IsFull() const;

    int   EmptyCount() const;
    int   Index(int x, int y) const { return y * width + x; }
};

#endif // BOARD_H
