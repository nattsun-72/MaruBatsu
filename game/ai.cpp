/****************************************
 * @file ai.cpp
 * @brief AI思考の実装
 * @author Natsume Shidara
 * @date 2026/05/15
 *
 * 着手選択ロジック:
 *   1. 自分が勝てるマス → 即座に置く
 *   2. 相手が次に勝つマス → ブロック
 *   3. それ以外 → 空きマスからランダム
 *
 * 勝利条件は AbilityRegistry の IWinCondition を経由するため、
 * 「ライン本数追加」「拡張勝利」等のアビリティが入っても自動追従する。
 ****************************************/
#include "ai.h"
#include "board.h"
#include "win_check.h"
#include "ability/hooks.h"

#include <vector>
#include <random>

namespace
{
    std::mt19937& Rng()
    {
        static std::mt19937 rng(std::random_device{}());
        return rng;
    }

    bool CheckWinWith(const Board& board, Piece player, IWinCondition* win)
    {
        if (win) return win->Check(board, player);
        return WinCheck::HasLine(board, player, 3);
    }

    Vec2 FindWinningMove(const Board& board, Piece self, IWinCondition* win)
    {
        Board test = board;
        for (int y = 0; y < board.height; ++y)
        {
            for (int x = 0; x < board.width; ++x)
            {
                if (!test.IsEmpty(x, y)) continue;
                test.Set(x, y, self);
                const bool won = CheckWinWith(test, self, win);
                test.Set(x, y, Piece::Empty);
                if (won) return { x, y };
            }
        }
        return { -1, -1 };
    }

    Vec2 PickRandomEmpty(const Board& board)
    {
        std::vector<Vec2> empties;
        empties.reserve(static_cast<size_t>(board.width) * board.height);
        for (int y = 0; y < board.height; ++y)
        {
            for (int x = 0; x < board.width; ++x)
            {
                if (board.IsEmpty(x, y)) empties.push_back({ x, y });
            }
        }
        if (empties.empty()) return { -1, -1 };
        std::uniform_int_distribution<size_t> dist(0, empties.size() - 1);
        return empties[dist(Rng())];
    }
}

namespace AI
{
    Vec2 ChooseMove_Random(const Board& board, Piece self, IWinCondition* win)
    {
        Vec2 v = FindWinningMove(board, self, win);
        if (v.x >= 0) return v;

        v = FindWinningMove(board, Opponent(self), win);
        if (v.x >= 0) return v;

        return PickRandomEmpty(board);
    }
}
