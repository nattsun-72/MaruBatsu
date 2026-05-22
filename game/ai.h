/****************************************
 * @file ai.h
 * @brief AI思考
 * @author Natsume Shidara
 * @date 2026/05/15
 * @update 2026/05/15 - IWinCondition フック対応 (Day 3)
 *
 * α版で MinimaxAI に差し替え予定。
 * 「勝てるマスがあれば置く、相手の勝てるマスをブロック、無ければランダム」。
 ****************************************/
#ifndef AI_H
#define AI_H

#include "piece.h"

class Board;
class IWinCondition;

namespace AI
{
    /**
     * @brief 着手先を決定 (空きマスがない場合 {-1,-1})
     * @param board 現在の盤面
     * @param self  AIが操作する駒
     * @param win   勝利条件 (registryのIWinCondition*)。nullなら3並びを既定使用。
     */
    Vec2 ChooseMove_Random(const Board& board, Piece self, IWinCondition* win);
}

#endif // AI_H
