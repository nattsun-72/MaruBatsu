/****************************************
 * @file   ai.cpp
 * @brief  AI思考の実装 (アルファベータ枝刈り付きミニマックス)
 * @author Natsume Shidara
 * @date   2026/05/15
 * @update 2026/06/19 - ランダム+1手読みからミニマックスへ刷新 (α版)
 * @update 2026/06/19 - ボスギミック/着手数を遷移関数へ織り込み (β化)
 *
 * 着手選択ロジック:
 *   盤面を再帰的に読み進め、自分(self)の勝ちを最大化、相手の勝ちを
 *   最小化する手を選ぶ。アルファベータ枝刈りで無駄な枝を切り落とす。
 *
 *   遷移(着手)は注入された PlacementSim を通すため、ボスギミック
 *   (氷盤スライド/回転/重力/連鎖/spawn等)を適用した「実際の盤面」を
 *   評価する。連続着手(二手打ち/焦燥)も PlacementCountFn で再現する。
 *
 *   評価値:
 *     self の勝ち  = +(WIN_SCORE - 手数)   … 早い勝ちほど高評価
 *     相手の勝ち   = -(WIN_SCORE - 手数)   … 遅い負けほど高評価
 *     引き分け     = 0
 *     深さ/葉上限  = ヒューリスティック評価 (連続長の差)
 *
 *   勝利判定は AbilityRegistry の IWinCondition を経由するため、
 *   「ライン本数追加」「拡張勝利」等のアビリティが入っても自動追従する。
 *   同時成立はゲーム本体と同じくプレイヤー優先で裁定する。
 *
 * 探索コスト制御:
 *   3×3 は空きマス9以下なので全探索しても安全。4×4 以上は空きマスが
 *   増えて爆発するため、空きマス数に応じて maxDepth を制限し、さらに葉
 *   ノード数が上限を超えたら探索を打ち切ってヒューリスティック評価へ
 *   フォールバックする。ギミック適用で盤面が変わるため、各手は試し置きの
 *   undo ではなく結果盤面のコピーを生成して読み進める。
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
     * @brief  探索全体で共有する状態 (陣営情報・遷移関数・消費ノード数)
     */
    struct SearchContext
    {
        Piece          self;        // AI自身の陣営
        Piece          opp;         // 相手の陣営
        IWinCondition* win;         // 勝利条件 (null可)
        int            maxDepth;    // 探索深さ上限
        long long      leafCount;   // 評価した葉ノード数(予算管理用)

        const AI::PlacementSim&      sim;     // 設置+ギミック適用の遷移関数
        const AI::PlacementCountFn&  counts;  // 着手数プロバイダ

        SearchContext(Piece s, IWinCondition* w, int depth,
                      const AI::PlacementSim& sm, const AI::PlacementCountFn& ct)
            : self(s), opp(Opponent(s)), win(w), maxDepth(depth), leafCount(0),
              sim(sm), counts(ct) {}

        /** @brief 葉ノード予算を使い切ったか */
        bool Exhausted() const { return leafCount >= NODE_BUDGET; }

        /** @brief 指定陣営の着手数 (最低1にクランプ) */
        int PlacementCount(Piece p) const
        {
            const int c = counts ? counts(p) : 1;
            return (c < 1) ? 1 : c;
        }

        /** @brief 設置+ギミック適用後の盤面を返す (sim 未指定なら純粋設置) */
        Board Apply(const Board& board, int x, int y, Piece who) const
        {
            if (sim) return sim(board, { x, y }, who);
            Board next = board;
            next.Set(x, y, who);
            return next;
        }
    };

    /**
     * @brief  終端(勝敗)判定。決着していれば self 視点スコアを out へ返す。
     * @detail 同時成立はゲーム本体と同じくプレイヤー優先で裁定する。
     * @return 決着していれば true (out に評価値を格納)
     */
    bool TerminalScore(const Board& board, int depth, SearchContext& ctx, int& out)
    {
        const bool playerWon = CheckWinWith(board, Piece::Player, ctx.win);
        const bool enemyWon  = CheckWinWith(board, Piece::Enemy,  ctx.win);
        if (!playerWon && !enemyWon) return false;

        const Piece winner = playerWon ? Piece::Player : Piece::Enemy;  // 同時成立はプレイヤー優先
        out = (winner == ctx.self) ? (WIN_SCORE - depth) : -(WIN_SCORE - depth);
        return true;
    }

    /**
     * @brief  アルファベータ枝刈り付きミニマックス本体
     * @param  board          現在の盤面 (この局面から toMove が着手する)
     * @param  toMove         この手番で着手する陣営
     * @param  placementsLeft toMove がこのターンに残す着手数 (連続着手対応)
     * @param  depth          現在の探索深さ(着手1回ごとに+1)
     * @param  alpha,beta     αβ枝刈り境界
     * @param  ctx            探索コンテキスト(共有状態)
     * @return self 視点の評価値
     */
    int Minimax(const Board& board, Piece toMove, int placementsLeft, int depth,
                int alpha, int beta, SearchContext& ctx)
    {
        // --- 終端判定(直前の着手/ギミックで決着していないか) ---
        int term = 0;
        if (TerminalScore(board, depth, ctx, term)) return term;

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

                // 設置+ギミック適用後の盤面を生成して読み進める。
                const Board next = ctx.Apply(board, x, y, toMove);

                int score;
                if (placementsLeft > 1)
                {
                    // 同一陣営の連続着手が残っている (二手打ち等)
                    score = Minimax(next, toMove, placementsLeft - 1,
                                    depth + 1, alpha, beta, ctx);
                }
                else
                {
                    // 相手のターンへ (相手の着手数を取得)
                    const Piece nextMover = Opponent(toMove);
                    score = Minimax(next, nextMover, ctx.PlacementCount(nextMover),
                                    depth + 1, alpha, beta, ctx);
                }

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
    Vec2 ChooseMove(const Board& board, Piece self, IWinCondition* win,
                    const PlacementSim& simulate, const PlacementCountFn& placements)
    {
        const int emptyCount = board.EmptyCount();
        if (emptyCount <= 0) return { -1, -1 };  // 空きマス無し → 着手不可

        SearchContext ctx(self, win, DepthLimitFor(emptyCount), simulate, placements);

        // self がこのターンに残す着手数 (連続着手なら2手目以降も self が続ける)
        const int selfPlacements = ctx.PlacementCount(self);

        int bestScore = std::numeric_limits<int>::min();
        std::vector<Vec2> bestMoves;  // 同評価(同点)の手の候補
        bestMoves.reserve(static_cast<size_t>(emptyCount));

        int alpha = std::numeric_limits<int>::min();
        const int beta = std::numeric_limits<int>::max();

        for (int y = 0; y < board.height; ++y)
        {
            for (int x = 0; x < board.width; ++x)
            {
                if (!board.IsEmpty(x, y)) continue;

                // self の一手を適用 (設置+ギミック)
                const Board next = ctx.Apply(board, x, y, self);

                int score;
                if (selfPlacements > 1)
                {
                    // この手番でまだ self が置ける → self 継続として評価
                    score = Minimax(next, self, selfPlacements - 1, 1, alpha, beta, ctx);
                }
                else
                {
                    const Piece nextMover = Opponent(self);
                    score = Minimax(next, nextMover, ctx.PlacementCount(nextMover),
                                    1, alpha, beta, ctx);
                }

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

                alpha = (std::max)(alpha, bestScore);  // ルートでも枝刈りに活用
            }
        }

        if (bestMoves.empty()) return { -1, -1 };  // 念のための保険

        // 同評価の手が複数あればランダムに選ぶ(乱数は同点選択のみに使用)。
        if (bestMoves.size() == 1) return bestMoves.front();
        std::uniform_int_distribution<size_t> dist(0, bestMoves.size() - 1);
        return bestMoves[dist(Rng())];
    }
}
