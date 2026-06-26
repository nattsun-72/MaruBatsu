/****************************************
 * @file   win_check.h
 * @brief  勝利条件判定 (連続並びの走査ロジック)
 * @author Natsume Shidara
 * @date   2026/05/15
 *
 * 縦・横・斜めの連続並びを走査する純粋な判定処理。
 * デフォルト勝利条件 (DefaultWinCondition) や AI の先読みから利用される。
 ****************************************/
#ifndef WIN_CHECK_H
#define WIN_CHECK_H

#include "piece.h"

#include <vector>

class Board;  // 前方宣言

//======================================
// 勝利条件判定 名前空間
//======================================
namespace WinCheck
{
    /**
     * @brief  player が指定本数の連続並びを達成しているか判定
     * @detail 縦・横・斜めの全方向を走査する。
     * @param  board            対象の盤面
     * @param  player           判定する陣営
     * @param  required         勝利に必要な連続数 (デフォルト3)
     * @param  includeDiagonals false なら斜めを除き縦横のみ判定する
     *                          (対角線無効アビリティ等が使用)
     * @return 達成していれば true
     */
    bool HasLine(const Board& board, Piece player, int required = 3,
                 bool includeDiagonals = true);

    /**
     * @brief  player の最大連続長を取得
     * @detail 任意方向で最も長く繋がっている駒数を返す。AI評価などで使用。
     * @param  board  対象の盤面
     * @param  player 判定する陣営
     * @return 最大連続長 (0以上)
     */
    int CountMaxLine(const Board& board, Piece player);

    /**
     * @brief  player の勝利ラインを構成するマス列を返す (勝利演出用)
     * @detail HasLine と同じ走査で、最初に見つかった required 連続の
     *         マス座標を始点から順に返す。勝利ラインが無ければ空を返す。
     *         斜め以外の特殊勝利(隅支配・四角支配等)は対象外(空が返る)。
     * @param  board            対象の盤面
     * @param  player           判定する陣営
     * @param  required         勝利に必要な連続数
     * @param  includeDiagonals 斜めを含めるか
     * @return 勝利ラインのマス列 (長さ required)。無ければ空。
     */
    std::vector<Vec2> FindWinningLine(const Board& board, Piece player,
                                      int required = 3, bool includeDiagonals = true);
}

#endif // WIN_CHECK_H
