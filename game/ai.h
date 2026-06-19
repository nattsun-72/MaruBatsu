/****************************************
 * @file   ai.h
 * @brief  AI思考(着手選択)
 * @author Natsume Shidara
 * @date   2026/05/15
 * @update 2026/05/15 - IWinCondition フック対応 (Day 3)
 * @update 2026/06/19 - 内部をアルファベータ枝刈り付きミニマックスへ強化 (α版)
 * @update 2026/06/19 - ボスギミック/着手数を考慮する探索へ拡張 (β化)
 *
 * α版AI。内部実装はアルファベータ枝刈り付きミニマックス探索。
 * 終端判定は IWinCondition* を経由するため、勝利条件アビリティが
 * 入っても自動追従する。3×3 は完全読み切り、4×4 以上は探索深さと
 * 葉ノード数の上限でコストを抑え、上限到達時はヒューリスティック評価する。
 *
 * β化で、着手後に発火するボスギミック(氷盤スライド/回転/重力/連鎖等)を
 * 探索の遷移関数へ織り込めるよう、設置シミュレータ(PlacementSim)と
 * 着手数プロバイダ(PlacementCountFn)を注入する形にした。これにより
 * AIは「設置 → ギミック適用後の実際の盤面」を読むようになる。
 ****************************************/
#ifndef AI_H
#define AI_H

#include "piece.h"
#include "board.h"

#include <functional>

class IWinCondition;  // 前方宣言

//======================================
// AI思考 名前空間
//======================================
namespace AI
{
    /**
     * @brief  1手の設置 + 付随するギミックを適用し、結果盤面を返す関数型
     * @detail board に who が pos へ着手し、その後ボスギミック(OnPlace相当)を
     *         適用した「最終盤面」を返す。呼び出し側(ゲームシーン)が
     *         AbilityRegistry を用いて実装する。AIはこれを純粋な遷移関数として
     *         扱う(乱択ギミックは1サンプルとして評価する)。
     */
    using PlacementSim = std::function<Board(const Board& board, Vec2 pos, Piece who)>;

    /**
     * @brief  指定陣営が1ターンに置く駒数を返す関数型
     * @detail 二手打ち/焦燥等で 2 になり得る。AIはターン構造の再現に用いる。
     */
    using PlacementCountFn = std::function<int(Piece player)>;

    /**
     * @brief  AIの着手先を決定する (アルファベータ枝刈り付きミニマックス)
     * @detail self の勝ちを最大化、相手の勝ちを最小化する手を選ぶ。
     *         同評価の手が複数ある場合はランダムに選択する。
     *         simulate でギミック適用後の盤面を読み、placements で各陣営の
     *         着手数(連続着手)を再現する。
     * @param  board      現在の盤面
     * @param  self       AIが操作する陣営
     * @param  win        勝利条件 (null の場合は3並びを既定使用)
     * @param  simulate   設置+ギミック適用の遷移関数 (null可: 純粋設置として扱う)
     * @param  placements 着手数プロバイダ (null可: 常に1として扱う)
     * @return 着手するマス座標。空きマスが無い場合は {-1, -1}。
     */
    Vec2 ChooseMove(const Board& board, Piece self, IWinCondition* win,
                    const PlacementSim& simulate, const PlacementCountFn& placements);
}

#endif // AI_H
