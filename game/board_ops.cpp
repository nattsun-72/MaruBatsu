/****************************************
 * @file   board_ops.cpp
 * @brief  盤面操作ユーティリティの実装
 * @author Natsume Shidara
 * @date   2026/05/15
 * @update 2026/05/22 - スライド移動の記録に対応
 ****************************************/
#include "board_ops.h"
#include "board.h"

#include <vector>

namespace BoardOps
{
    //======================================
    // 全駒スライド
    //======================================
    void SlideAll(Board& board, Direction dir, std::vector<Move>* outMoves)
    {
        /*--- 走査方向の決定 ---*/
        // horizontal: 左右スライドなら行ごと、上下なら列ごとに処理する
        // toEnd     : 右・下方向は「末尾寄せ」、左・上方向は「先頭寄せ」
        const bool horizontal = (dir == Direction::Left || dir == Direction::Right);
        const bool toEnd      = (dir == Direction::Right || dir == Direction::Down);
        const int  lineLen    = horizontal ? board.width  : board.height;
        const int  lineCount  = horizontal ? board.height : board.width;

        // ラインインデックス i から盤面座標へ変換するヘルパ
        auto cellAt = [&](int line, int i) -> Vec2 {
            return horizontal ? Vec2{ i, line } : Vec2{ line, i };
        };

        /*--- ライン単位で駒を端へ詰める ---*/
        for (int line = 0; line < lineCount; ++line)
        {
            // 1) 非空の駒を出現順に収集 (順序を保ったまま詰めるため)
            struct Item { Piece piece; Vec2 from; };
            std::vector<Item> kept;
            kept.reserve(static_cast<size_t>(lineLen));
            for (int i = 0; i < lineLen; ++i)
            {
                const Vec2 c = cellAt(line, i);
                const Piece p = board.Get(c.x, c.y);
                if (p != Piece::Empty) kept.push_back({ p, c });
            }

            // 2) ラインを一旦全消去
            for (int i = 0; i < lineLen; ++i)
            {
                const Vec2 c = cellAt(line, i);
                board.Set(c.x, c.y, Piece::Empty);
            }

            // 3) 端へ詰めて再配置し、移動した駒を記録する
            const int offset = toEnd
                ? (lineLen - static_cast<int>(kept.size()))  // 末尾寄せの開始位置
                : 0;                                         // 先頭寄せ
            for (int k = 0; k < static_cast<int>(kept.size()); ++k)
            {
                const Vec2 dst = cellAt(line, offset + k);
                board.Set(dst.x, dst.y, kept[k].piece);

                // 実際に位置が変わった駒だけ移動記録へ追加
                if (outMoves
                    && (dst.x != kept[k].from.x || dst.y != kept[k].from.y))
                {
                    outMoves->push_back({ kept[k].from.x, kept[k].from.y,
                                          dst.x, dst.y, kept[k].piece });
                }
            }
        }
    }

    //======================================
    // 単駒スライド
    //======================================
    bool SlideOne(Board& board, int x, int y, Direction dir,
                  std::vector<Move>* outMoves)
    {
        /*--- 移動可否の判定 ---*/
        if (!board.IsValid(x, y)) return false;               // 元座標が盤外
        const Piece p = board.Get(x, y);
        if (p == Piece::Empty) return false;                  // 動かす駒が無い

        const int nx = x + DirX(dir);
        const int ny = y + DirY(dir);
        if (!board.IsValid(nx, ny)) return false;             // 滑り先が盤外
        if (board.Get(nx, ny) != Piece::Empty) return false;  // 滑り先が埋まっている

        /*--- 移動の実行 ---*/
        board.Set(x, y, Piece::Empty);
        board.Set(nx, ny, p);
        if (outMoves) outMoves->push_back({ x, y, nx, ny, p });
        return true;
    }
}
