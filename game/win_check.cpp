/****************************************
 * @file   win_check.cpp
 * @brief  勝利条件判定の実装
 * @author Natsume Shidara
 * @date   2026/05/15
 ****************************************/
#include "win_check.h"
#include "board.h"

#include <algorithm>

//--------------------------------------
// 内部ヘルパ
//--------------------------------------
namespace
{
    // 走査する4方向の単位ベクトル: 右 / 下 / 右下 / 右上。
    // 残り4方向(左・上・左上・左下)は逆走査になるため省略できる。
    constexpr int DIR_COUNT = 4;
    constexpr int DIRS[DIR_COUNT][2] = {
        { 1,  0 },  // 右   (横ライン)
        { 0,  1 },  // 下   (縦ライン)
        { 1,  1 },  // 右下 (斜めライン ＼)
        { 1, -1 },  // 右上 (斜めライン ／)
    };

    /**
     * @brief  (sx,sy)から方向(dx,dy)へ、駒 p が連続する長さを数える
     * @return 連続して並んでいる p の数
     */
    int LineLengthFrom(const Board& board, int sx, int sy, int dx, int dy, Piece p)
    {
        int count = 0;
        int x = sx, y = sy;
        while (board.IsValid(x, y) && board.Get(x, y) == p)
        {
            ++count;
            x += dx;
            y += dy;
        }
        return count;
    }
}

namespace WinCheck
{
    //======================================
    // 連続並びの達成判定
    //======================================
    bool HasLine(const Board& board, Piece player, int required,
                 bool includeDiagonals)
    {
        if (player == Piece::Empty) return false;  // 空マスは勝利対象外
        if (required <= 0) return true;            // 0本要求は常に達成扱い

        // 斜め無効時は先頭2方向 (右/下) のみ走査する
        const int dirCount = includeDiagonals ? DIR_COUNT : 2;

        // 全マスを始点候補として、各方向のライン長を調べる
        for (int y = 0; y < board.height; ++y)
        {
            for (int x = 0; x < board.width; ++x)
            {
                if (board.Get(x, y) != player) continue;
                for (int d = 0; d < dirCount; ++d)
                {
                    const int dx = DIRS[d][0];
                    const int dy = DIRS[d][1];
                    // 1つ手前が同色なら、そのラインは別始点で数えるためスキップ
                    if (board.Get(x - dx, y - dy) == player) continue;
                    if (LineLengthFrom(board, x, y, dx, dy, player) >= required)
                    {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    //======================================
    // 最大連続長の取得
    //======================================
    int CountMaxLine(const Board& board, Piece player)
    {
        if (player == Piece::Empty) return 0;

        int best = 0;
        for (int y = 0; y < board.height; ++y)
        {
            for (int x = 0; x < board.width; ++x)
            {
                if (board.Get(x, y) != player) continue;
                for (int d = 0; d < DIR_COUNT; ++d)
                {
                    const int dx = DIRS[d][0];
                    const int dy = DIRS[d][1];
                    // ラインの始点からのみ計測 (重複計測を避ける)
                    if (board.Get(x - dx, y - dy) == player) continue;
                    best = (std::max)(best,
                                      LineLengthFrom(board, x, y, dx, dy, player));
                }
            }
        }
        return best;
    }
}
