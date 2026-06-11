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
     * @brief  1駒の移動/出現の記録
     * @detail アニメーション再生で「どの駒がどこからどこへ動いたか」を
     *         再現するために使う。
     *         isSpawn=true の場合は「移動」ではなく「出現(新規生成)」を表し、
     *         from は出現元(連鎖の発生源。盤上に残る)、to が出現先となる。
     *         連鎖拡張・爆弾・召喚など、新たに駒が現れる効果で使う。
     */
    struct Move
    {
        int   fromX, fromY;       // 移動元 / (出現時)発生源のマス座標
        int   toX,   toY;         // 移動先 / 出現先のマス座標
        Piece piece;              // 移動/出現した駒の種別
        bool  isSpawn = false;    // true=出現(fromは消さず残す), false=移動
    };

    /** @brief 同時に起こる移動の集まり (1効果 = 1フェーズ) */
    using MoveList = std::vector<Move>;

    //--------------------------------------
    // スライド操作
    //--------------------------------------
    /**
     * @brief  全ての駒を指定方向の端まで滑らせる (2048タイル風)
     * @detail 各ライン(行または列)ごとに、駒の順序を保ったまま端へ詰める。
     *         anchoredSide の駒は「重い」ため動かず、他の駒は
     *         その駒や壁に当たるまで滑る (重駒アビリティ対応)。
     * @param  board        対象の盤面 (直接書き換える)
     * @param  dir          滑らせる方向
     * @param  outMoves     非nullなら、実際に移動した駒の記録を追記する
     * @param  anchoredSide 固定する陣営 (Empty なら全駒が滑る)
     */
    void SlideAll(Board& board, Direction dir, std::vector<Move>* outMoves = nullptr,
                  Piece anchoredSide = Piece::Empty);

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
     * @brief  指定マスの駒を、その列の一番下の空きマスまで落下させる
     * @detail 重力ギミック(ボス4等)から利用する。直下が埋まっていれば
     *         何もしない。
     * @param  board    対象の盤面 (直接書き換える)
     * @param  x,y      落下させる駒のマス座標
     * @param  outMoves 非nullなら、移動が起きた場合に記録を追記する
     * @return 実際に落下したら true
     */
    bool DropDown(Board& board, int x, int y, std::vector<Move>* outMoves = nullptr);

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
