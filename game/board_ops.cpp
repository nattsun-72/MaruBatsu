/****************************************
 * @file board_ops.cpp
 * @brief 盤面操作ユーティリティの実装
 * @author Natsume Shidara
 * @date 2026/05/15
 ****************************************/
#include "board_ops.h"
#include "board.h"

#include <vector>

namespace
{
    // 1ラインを片側へ寄せる
    // cells: そのラインの駒ポインタ列 (盤面のセルを直接参照)
    // toEnd: true なら末尾(右/下)寄せ、false なら先頭(左/上)寄せ
    void CompactLine(std::vector<Piece*>& cells, bool toEnd)
    {
        std::vector<Piece> kept;
        kept.reserve(cells.size());
        for (Piece* c : cells)
        {
            if (*c != Piece::Empty) kept.push_back(*c);
        }
        for (Piece* c : cells) *c = Piece::Empty;

        if (toEnd)
        {
            // 末尾から逆に詰める (kept順を保つ)
            const size_t offset = cells.size() - kept.size();
            for (size_t i = 0; i < kept.size(); ++i)
            {
                *cells[offset + i] = kept[i];
            }
        }
        else
        {
            for (size_t i = 0; i < kept.size(); ++i)
            {
                *cells[i] = kept[i];
            }
        }
    }
}

namespace BoardOps
{
    void SlideAll(Board& board, Direction dir)
    {
        const bool horizontal = (dir == Direction::Left || dir == Direction::Right);
        const bool toEnd      = (dir == Direction::Right || dir == Direction::Down);

        if (horizontal)
        {
            std::vector<Piece*> line(static_cast<size_t>(board.width));
            for (int y = 0; y < board.height; ++y)
            {
                for (int x = 0; x < board.width; ++x)
                {
                    line[static_cast<size_t>(x)] = &board.cells[board.Index(x, y)];
                }
                CompactLine(line, toEnd);
            }
        }
        else
        {
            std::vector<Piece*> line(static_cast<size_t>(board.height));
            for (int x = 0; x < board.width; ++x)
            {
                for (int y = 0; y < board.height; ++y)
                {
                    line[static_cast<size_t>(y)] = &board.cells[board.Index(x, y)];
                }
                CompactLine(line, toEnd);
            }
        }
    }

    bool SlideOne(Board& board, int x, int y, Direction dir)
    {
        if (!board.IsValid(x, y)) return false;
        const Piece p = board.Get(x, y);
        if (p == Piece::Empty) return false;

        const int nx = x + DirX(dir);
        const int ny = y + DirY(dir);
        if (!board.IsValid(nx, ny)) return false;
        if (board.Get(nx, ny) != Piece::Empty) return false;

        board.Set(x, y, Piece::Empty);
        board.Set(nx, ny, p);
        return true;
    }
}
