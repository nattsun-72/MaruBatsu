/****************************************
 * @file   ai.h
 * @brief  AI思考(着手選択)
 * @author Natsume Shidara
 * @date   2026/05/15
 * @update 2026/05/15 - IWinCondition フック対応 (Day 3)
 *
 * プロト版のシンプルAI。「勝てるマスがあれば置く / 相手の勝てる
 * マスをブロック / それ以外はランダム」で着手を決める。
 * α版で MinimaxAI への差し替えを予定。
 ****************************************/
#ifndef AI_H
#define AI_H

#include "piece.h"

class Board;          // 前方宣言
class IWinCondition;  // 前方宣言

//======================================
// AI思考 名前空間
//======================================
namespace AI
{
    /**
     * @brief  AIの着手先を決定する
     * @detail 勝利 → ブロック → ランダム の優先順で1手を選ぶ。
     * @param  board 現在の盤面
     * @param  self  AIが操作する陣営
     * @param  win   勝利条件 (AbilityRegistry の IWinCondition*)。
     *               null の場合は3並びを既定として使用する。
     * @return 着手するマス座標。空きマスが無い場合は {-1, -1}。
     */
    Vec2 ChooseMove_Random(const Board& board, Piece self, IWinCondition* win);
}

#endif // AI_H
