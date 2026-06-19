/****************************************
 * @file   ai.h
 * @brief  AI思考(着手選択)
 * @author Natsume Shidara
 * @date   2026/05/15
 * @update 2026/05/15 - IWinCondition フック対応 (Day 3)
 * @update 2026/06/19 - 内部をアルファベータ枝刈り付きミニマックスへ強化 (α版)
 *
 * α版AI。内部実装はアルファベータ枝刈り付きミニマックス探索。
 * 終端判定は IWinCondition* を経由するため、勝利条件アビリティが
 * 入っても自動追従する。3×3 は完全読み切り、4×4 以上は探索深さと
 * 葉ノード数の上限でコストを抑え、上限到達時はヒューリスティック評価する。
 * ※ 公開関数名は呼び出し側互換のため "Random" のまま据え置く。
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
     * @detail 内部はアルファベータ枝刈り付きミニマックス探索。
     *         self の勝ちを最大化、相手の勝ちを最小化する手を選ぶ。
     *         同点(同評価)の手が複数ある場合はランダムに選択する。
     *         ギミック(スライド/回転/重力/spawn)は考慮せず、純粋な
     *         駒の設置のみをシミュレートする(現行AIと同じ割り切り)。
     * @param  board 現在の盤面
     * @param  self  AIが操作する陣営
     * @param  win   勝利条件 (AbilityRegistry の IWinCondition*)。
     *               null の場合は3並びを既定として使用する。
     * @return 着手するマス座標。空きマスが無い場合は {-1, -1}。
     */
    Vec2 ChooseMove_Random(const Board& board, Piece self, IWinCondition* win);
}

#endif // AI_H
