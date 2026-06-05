/****************************************
 * @file   board_ops.h
 * @brief  盤面操作ユーティリティ(スライド処理)
 * @author Natsume Shidara
 * @date   2026/05/15
 * @update 2026/05/22 - スライド移動の記録に対応 (アニメーション用)
 *
 * 氷盤(全駒スライド)・氷駒(1マススライド)など、駒を移動させる
 * 効果フックから共通で呼ばれる、純粋な盤面操作をまとめる。
 ****************************************/
#ifndef BOARD_OPS_H
#define BOARD_OPS_H

//--------------------------------------
// インクルード
//--------------------------------------
#include "direction.h"
#include "piece.h"

#include <vector>

class Board;  // 前方宣言

//======================================
// 盤面操作 名前空間
//======================================
namespace BoardOps
{
    //--------------------------------------
    // 移動記録の型
    //--------------------------------------
    /**
     * @struct Move
     * @brief  1駒の移動記録 (from → to)
     * @detail アニメーション再生で「どの駒がどこからどこへ動いたか」を
     *         再現するために使う。
     */
    struct Move
    {
        int   fromX, fromY;  // 移動元のマス座標
        int   toX,   toY;    // 移動先のマス座標
        Piece piece;         // 移動した駒の種別
    };

    /** @brief 同時に起こる移動の集まり (1効果 = 1フェーズ) */
    using MoveList = std::vector<Move>;

    //--------------------------------------
    // スライド操作
    //--------------------------------------
    /**
     * @brief  全ての駒を指定方向の端まで滑らせる (2048タイル風)
     * @detail 各ライン(行または列)ごとに、駒の順序を保ったまま端へ詰める。
     * @param  board    対象の盤面 (直接書き換える)
     * @param  dir      滑らせる方向
     * @param  outMoves 非nullなら、実際に移動した駒の記録を追記する
     */
    void SlideAll(Board& board, Direction dir, std::vector<Move>* outMoves = nullptr);

    /**
     * @brief  指定マスの駒を1マスだけ滑らせる
     * @detail 滑り先が盤内かつ空マスのときのみ移動する。
     * @param  board    対象の盤面 (直接書き換える)
     * @param  x,y      滑らせる駒のマス座標
     * @param  dir      滑らせる方向
     * @param  outMoves 非nullなら、移動が起きた場合に記録を追記する
     * @return 実際に移動したら true、移動しなかったら false
     */
    bool SlideOne(Board& board, int x, int y, Direction dir,
                  std::vector<Move>* outMoves = nullptr);

    //--------------------------------------
    // 回転操作
    //--------------------------------------
    /**
     * @brief  盤面を90度回転させる (正方盤のみ)
     * @detail 盤面の全駒を時計回り/反時計回りに90度回転する。
     *         縦横サイズが異なる盤面では何もしない(現状の盤面は常に正方)。
     *         「螺旋の女王」の盤面回転ギミックから利用する。
     * @param  board     対象の盤面 (直接書き換える)
     * @param  clockwise true=時計回り / false=反時計回り
     * @param  outMoves  非nullなら、移動した駒の記録を追記する(アニメ用)
     */
    void Rotate90(Board& board, bool clockwise, std::vector<Move>* outMoves = nullptr);
}

#endif // BOARD_OPS_H
