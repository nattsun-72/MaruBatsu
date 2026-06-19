/****************************************
 * @file   ai.cpp
 * @brief  AI思考の実装 (アルファベータ枝刈り付きミニマックス)
 * @author Natsume Shidara
 * @date   2026/05/15
 * @update 2026/06/19 - ランダム+1手読みからミニマックスへ刷新 (α版)
 *
 * 着手選択ロジック:
 *   盤面を再帰的に読み進め、自分(self)の勝ちを最大化、相手の勝ちを
 *   最小化する手を選ぶ。アルファベータ枝刈りで無駄な枝を切り落とす。
 *
 *   評価値:
 *     self の勝ち  = +(WIN_SCORE - 手数)   … 早い勝ちほど高評価
 *     相手の勝ち   = -(WIN_SCORE - 手数)   … 遅い負けほど高評価
 *     引き分け     = 0
 *     深さ/葉上限  = ヒューリスティック評価 (連続長の差)
 *
 *   勝利判定は AbilityRegistry の IWinCondition を経由するため、
 *   「ライン本数追加」「拡張勝利」等のアビリティが入っても自動追従する。
 *
 * 探索コスト制御:
 *   3×3 は空きマス9以下なので全探索しても安全(高々9! ≒ 36万、枝刈りで
 *   実際は遥かに少ない)。4×4 以上は空きマスが増えて爆発するため、
 *   空きマス数に応じて maxDepth を制限し、さらに葉ノード数が上限を
 *   超えたら探索を打ち切ってヒューリスティック評価へフォールバックする。
 *   ※ ギミック(スライド/回転/重力/spawn)は考慮せず純粋な設置のみ。
 ****************************************/
#include "ai.h"
#include "board.h"
#include "win_check.h"
#include "ability/hooks.h"

#include <vector>
#include <random>
#include <limits>
#include <algorithm>

//--------------------------------------
// 探索パラメータ
//--------------------------------------
namespace
{
    // 勝利の基準スコア。手数補正(深さ)を引いても符号が反転しない大きさにする。
    constexpr int WIN_SCORE = 1'000'000;

    // 探索した葉ノード数の上限。これを超えたら以降はヒューリスティック評価で打ち切る。
    constexpr int NODE_BUDGET = 200'000;

    // 空きマス数に応じた探索深さ上限(残り空きマスが多いほど浅くする)。
    //   ・ 9マス以下(3×3全域含む) → 上限なし(完全読み切り)
    //   ・10〜12マス               → 深さ8
    //   ・13マス以上(4×4序盤等)   → 深さ6
    // 上記に加え NODE_BUDGET でも保険を掛ける。
    int DepthLimitFor(int emptyCount)
    {
        if (emptyCount <= 9)  return 64;  // 実質無制限(空きマス数で自然に止まる)
        if (emptyCount <= 12) return 8;
        return 6;
    }
}

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
     * @brief  探索打ち切り時のヒューリスティック評価
     * @detail self と相手の「最大連続長」の差で局面の有利不利を概算する。
     * @return self 視点の評価値 (正=self有利)
     */
    int Heuristic(const Board& board, Piece self)
    {
        const Piece opp = Opponent(self);
        return WinCheck::CountMaxLine(board, self) - WinCheck::CountMaxLine(board, opp);
    }

    /**
     * @struct SearchContext
     * @brief  探索全体で共有する状態 (固定の陣営情報と消費ノード数)
     */
    struct SearchContext
    {
        Piece          self;        // AI自身の陣営
        Piece          opp;         // 相手の陣営
        IWinCondition* win;         // 勝利条件 (null可)
        int            maxDepth;    // 探索深さ上限
        long long      leafCount;   // 評価した葉ノード数(予算管理用)

        SearchContext(Piece s, IWinCondition* w, int depth)
            : self(s), opp(Opponent(s)), win(w), maxDepth(depth), leafCount(0) {}

        /** @brief 葉ノード予算を使い切ったか */
        bool Exhausted() const { return leafCount >= NODE_BUDGET; }
    };

    /**
     * @brief  アルファベータ枝刈り付きミニマックス本体
     * @detail self 手番=最大化、相手手番=最小化。終端(勝敗/満杯/深さ上限/
     *         予算枯渇)で評価値を返す。盤面は試し置き→評価→戻すで再利用する。
     * @param  board   現在の盤面(参照渡し・破壊的に試し置きするが戻すので不変)
     * @param  toMove  この手番で着手する陣営
     * @param  depth   現在の探索深さ(ルート=0)
     * @param  alpha   αカット用の下限
     * @param  beta    βカット用の上限
     * @param  ctx     探索コンテキスト(共有状態)
     * @return self 視点の評価値
     */
    int Minimax(Board& board, Piece toMove, int depth,
                int alpha, int beta, SearchContext& ctx)
    {
        // --- 終端判定(直前の着手で決着していないか) ---
        // 手数が浅い決着ほど高評価になるよう depth で補正する。
        if (CheckWinWith(board, ctx.self, ctx.win))
            return  WIN_SCORE - depth;   // self の勝ち(早いほど高い)
        if (CheckWinWith(board, ctx.opp, ctx.win))
            return -(WIN_SCORE - depth); // 相手の勝ち(遅いほどマシ=高い)

        // --- 深さ上限 / 葉ノード予算 / 満杯 → ヒューリスティックで打ち切り ---
        if (depth >= ctx.maxDepth || ctx.Exhausted() || board.IsFull())
        {
            ++ctx.leafCount;
            return Heuristic(board, ctx.self);
        }

        const bool maximizing = (toMove == ctx.self);
        int best = maximizing ? std::numeric_limits<int>::min()
                              : std::numeric_limits<int>::max();

        for (int y = 0; y < board.height; ++y)
        {
            for (int x = 0; x < board.width; ++x)
            {
                if (!board.IsEmpty(x, y)) continue;

                board.Set(x, y, toMove);                       // 試し置き
                const int score = Minimax(board, Opponent(toMove),
                                          depth + 1, alpha, beta, ctx);
                board.Set(x, y, Piece::Empty);                 // 元に戻す

                if (maximizing)
                {
                    best  = (std::max)(best, score);
                    alpha = (std::max)(alpha, best);
                }
                else
                {
                    best  = (std::min)(best, score);
                    beta  = (std::min)(beta, best);
                }

                if (beta <= alpha) return best;  // αβカット
            }
        }
        return best;
    }
}

namespace AI
{
    //======================================
    // 着手決定 (アルファベータ枝刈り付きミニマックス)
    //======================================
    Vec2 ChooseMove_Random(const Board& board, Piece self, IWinCondition* win)
    {
        const int emptyCount = board.EmptyCount();
        if (emptyCount <= 0) return { -1, -1 };  // 空きマス無し → 着手不可

        // 空きマス数に応じて探索深さを決め、探索コンテキストを用意する。
        SearchContext ctx(self, win, DepthLimitFor(emptyCount));

        Board test = board;  // ルートの試し置き用コピー(以降この1枚を使い回す)

        int bestScore = std::numeric_limits<int>::min();
        std::vector<Vec2> bestMoves;  // 同評価(同点)の手の候補
        bestMoves.reserve(static_cast<size_t>(emptyCount));

        int alpha = std::numeric_limits<int>::min();
        const int beta = std::numeric_limits<int>::max();

        for (int y = 0; y < test.height; ++y)
        {
            for (int x = 0; x < test.width; ++x)
            {
                if (!test.IsEmpty(x, y)) continue;

                test.Set(x, y, self);  // self の一手を試し置き
                // self が着手した直後の局面を、相手手番(深さ1)から評価する。
                const int score = Minimax(test, Opponent(self), 1,
                                          alpha, beta, ctx);
                test.Set(x, y, Piece::Empty);  // 元に戻す

                if (score > bestScore)
                {
                    bestScore = score;
                    bestMoves.clear();
                    bestMoves.push_back({ x, y });
                }
                else if (score == bestScore)
                {
                    bestMoves.push_back({ x, y });  // 同点手として保持
                }

                // ルートでも α を更新して以降の評価を枝刈りに活かす。
                alpha = (std::max)(alpha, bestScore);
            }
        }

        if (bestMoves.empty()) return { -1, -1 };  // 念のための保険

        // 同評価の手が複数あればランダムに選ぶ(乱数は同点選択のみに使用)。
        if (bestMoves.size() == 1) return bestMoves.front();
        std::uniform_int_distribution<size_t> dist(0, bestMoves.size() - 1);
        return bestMoves[dist(Rng())];
    }
}
