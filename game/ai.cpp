/****************************************
 * @file   ai.cpp
 * @brief  AI思考の実装
 * @author Natsume Shidara
 * @date   2026/05/15
 *
 * 着手選択ロジック(優先順):
 *   1. 自分が勝てるマス   → 即座に置く
 *   2. 相手が次に勝つマス → ブロックする
 *   3. それ以外           → 空きマスからランダム
 *
 * 勝利判定は AbilityRegistry の IWinCondition を経由するため、
 * 「ライン本数追加」「拡張勝利」等のアビリティが入っても自動追従する。
 ****************************************/
#include "ai.h"
#include "board.h"
#include "win_check.h"
#include "ability/hooks.h"

#include <vector>
#include <random>

//--------------------------------------
// 内部ヘルパ
//--------------------------------------
namespace
{
    /** @brief 乱数エンジン (初回呼び出し時に生成し共有する) */
    std::mt19937& Rng()
    {
        static std::mt19937 rng(std::random_device{}());
        return rng;
    }

    /**
     * @brief  勝利条件で player が勝っているか判定
     * @detail win が指定されていればそれを使い、null なら3並びを既定使用。
     */
    bool CheckWinWith(const Board& board, Piece player, IWinCondition* win)
    {
        if (win) return win->Check(board, player);
        return WinCheck::HasLine(board, player, 3);
    }

    /**
     * @brief  self が1手で勝てるマスを探す
     * @return 勝てるマス座標。無ければ {-1, -1}。
     */
    Vec2 FindWinningMove(const Board& board, Piece self, IWinCondition* win)
    {
        Board test = board;  // 試し置き用のコピー
        for (int y = 0; y < board.height; ++y)
        {
            for (int x = 0; x < board.width; ++x)
            {
                if (!test.IsEmpty(x, y)) continue;
                test.Set(x, y, self);
                const bool won = CheckWinWith(test, self, win);
                test.Set(x, y, Piece::Empty);  // 試し置きを元に戻す
                if (won) return { x, y };
            }
        }
        return { -1, -1 };
    }

    /**
     * @brief  空きマスから1つをランダムに選ぶ
     * @return ランダムな空きマス座標。空きが無ければ {-1, -1}。
     */
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
    //======================================
    // 着手決定
    //======================================
    Vec2 ChooseMove_Random(const Board& board, Piece self, IWinCondition* win)
    {
        // 1) 自分が勝てるマスがあれば最優先で取る
        Vec2 v = FindWinningMove(board, self, win);
        if (v.x >= 0) return v;

        // 2) 相手が勝てるマスがあればブロックする
        v = FindWinningMove(board, Opponent(self), win);
        if (v.x >= 0) return v;

        // 3) それ以外は空きマスからランダムに選ぶ
        return PickRandomEmpty(board);
    }
}
