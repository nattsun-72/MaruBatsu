/****************************************
 * @file board.cpp
 * @brief 盤面データの実装
 * @author Natsume Shidara
 * @date 2026/05/15
 ****************************************/
#include "board.h"

Board::Board()
{
    Reset();
}

Board::Board(int w, int h)
    : width(w), height(h)
{
    Reset();
}

void Board::Reset()
{
    cells.assign(static_cast<size_t>(width) * height, Piece::Empty);
}

void Board::Resize(int w, int h)
{
    width  = w;
    height = h;
    Reset();
}

Piece Board::Get(int x, int y) const
{
    if (!IsValid(x, y)) return Piece::Empty;
    return cells[Index(x, y)];
}

void Board::Set(int x, int y, Piece p)
{
    if (!IsValid(x, y)) return;
    cells[Index(x, y)] = p;
}

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
