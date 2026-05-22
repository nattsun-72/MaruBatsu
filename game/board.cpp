/****************************************
 * @file   board.cpp
 * @brief  盤面データの実装
 * @author Natsume Shidara
 * @date   2026/05/15
 ****************************************/
#include "board.h"

//======================================
// コンストラクタ
//======================================
/** @brief デフォルト構築 — 既定サイズ(3×3)で全マスを空に初期化 */
Board::Board()
{
    Reset();
}

/** @brief サイズ指定で構築 — w×h で全マスを空に初期化 */
Board::Board(int w, int h)
    : width(w), height(h)
{
    Reset();
}

//======================================
// 盤面操作
//======================================
void Board::Reset()
{
    // width × height 個のマスを全て Empty で埋め直す
    cells.assign(static_cast<size_t>(width) * height, Piece::Empty);
}

void Board::Resize(int w, int h)
{
    width  = w;
    height = h;
    Reset();   // サイズ変更に伴い全マスを再確保
}

Piece Board::Get(int x, int y) const
{
    if (!IsValid(x, y)) return Piece::Empty;   // 盤外は空マス扱い
    return cells[Index(x, y)];
}

void Board::Set(int x, int y, Piece p)
{
    if (!IsValid(x, y)) return;                // 盤外への設置は無視
    cells[Index(x, y)] = p;
}

//======================================
// 状態判定
//======================================
bool Board::IsValid(int x, int y) const
{
    return x >= 0 && x < width && y >= 0 && y < height;
}

bool Board::IsEmpty(int x, int y) const
{
    return IsValid(x, y) && cells[Index(x, y)] == Piece::Empty;
}

bool Board::IsFull() const
{
    // 1つでも空マスがあれば満杯ではない
    for (Piece p : cells)
    {
        if (p == Piece::Empty) return false;
    }
    return true;
}

int Board::EmptyCount() const
{
    int count = 0;
    for (Piece p : cells)
    {
        if (p == Piece::Empty) ++count;
    }
    return count;
}
