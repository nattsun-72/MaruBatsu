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
    void SlideAll(Board& board, Direction dir, std::vector<Move>* outMoves,
                  Piece anchoredSide)
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
            // 1) ラインの内容を控えてから一旦全消去する
            std::vector<Piece> src(static_cast<size_t>(lineLen));
            for (int i = 0; i < lineLen; ++i)
            {
                const Vec2 c = cellAt(line, i);
                src[static_cast<size_t>(i)] = board.Get(c.x, c.y);
                board.Set(c.x, c.y, Piece::Empty);
            }

            // 2) 滑走先の端から順に走査して詰め直す。
            //    固定駒(anchoredSide)は元の位置に置き直し、それ以降の
            //    可動駒は固定駒の手前までしか滑れない (障害物として働く)。
            //    走査順: 末尾寄せなら lineLen-1→0、先頭寄せなら 0→lineLen-1。
            const int start = toEnd ? lineLen - 1 : 0;
            const int step  = toEnd ? -1 : 1;
            int dst = start;   // 次に可動駒が収まる位置
            for (int k = 0, i = start; k < lineLen; ++k, i += step)
            {
                const Piece p = src[static_cast<size_t>(i)];
                if (p == Piece::Empty) continue;

                if (anchoredSide != Piece::Empty && p == anchoredSide)
                {
                    // 固定駒: 動かず元の位置へ。後続の可動駒はこの先(手前)まで。
                    const Vec2 c = cellAt(line, i);
                    board.Set(c.x, c.y, p);
                    dst = i + step;
                    continue;
                }

                // 可動駒: dst へ詰める
                const Vec2 from = cellAt(line, i);
                const Vec2 to   = cellAt(line, dst);
                board.Set(to.x, to.y, p);
                if (outMoves && (to.x != from.x || to.y != from.y))
                {
                    outMoves->push_back({ from.x, from.y, to.x, to.y, p });
                }
                dst += step;
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

    //======================================
    // 落下
    //======================================
    bool DropDown(Board& board, int x, int y, std::vector<Move>* outMoves)
    {
        /*--- 落下可否の判定 ---*/
        if (!board.IsValid(x, y)) return false;
        const Piece p = board.Get(x, y);
        if (p == Piece::Empty) return false;

        // 直下から連続する空きマスの最下端を探す
        int destY = y;
        while (board.IsEmpty(x, destY + 1)) ++destY;
        if (destY == y) return false;   // 直下が埋まっている

        /*--- 落下の実行 ---*/
        board.Set(x, y, Piece::Empty);
        board.Set(x, destY, p);
        if (outMoves) outMoves->push_back({ x, y, x, destY, p });
        return true;
    }

    //======================================
    // 盤面回転
    //======================================
    void Rotate90(Board& board, bool clockwise, std::vector<Move>* outMoves)
    {
        const int n = board.width;
        if (n != board.height) return;   // 正方盤のみ対応
        if (n <= 1)            return;   // 1マス以下は回転不要

        /*--- 回転後の配置を一時バッファへ作る ---*/
        // 時計回り  : (x,y) -> (n-1-y, x)
        // 反時計回り: (x,y) -> (y, n-1-x)
        std::vector<Piece> rotated(static_cast<size_t>(n) * n, Piece::Empty);
        for (int y = 0; y < n; ++y)
        {
            for (int x = 0; x < n; ++x)
            {
                const Piece p  = board.Get(x, y);
                const int   nx = clockwise ? (n - 1 - y) : (y);
                const int   ny = clockwise ? (x)         : (n - 1 - x);
                rotated[static_cast<size_t>(ny) * n + nx] = p;

                // 実際に位置が変わる駒だけ移動記録へ (中央マスは動かない)
                if (outMoves && p != Piece::Empty && (nx != x || ny != y))
                {
                    outMoves->push_back({ x, y, nx, ny, p });
                }
            }
        }

        /*--- 盤面へ書き戻す ---*/
        for (int y = 0; y < n; ++y)
        {
            for (int x = 0; x < n; ++x)
            {
                board.Set(x, y, rotated[static_cast<size_t>(y) * n + x]);
            }
        }
    }
}
