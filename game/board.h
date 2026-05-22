/****************************************
 * @file   board.h
 * @brief  盤面データ(マス目と駒の管理)
 * @author Natsume Shidara
 * @date   2026/05/15
 *
 * width × height のマス目で駒を保持する。内部は1次元 vector で、
 * index = y * width + x の対応で2D座標と紐付ける。
 * 盤面拡張アビリティ(α版予定)に備え Resize も用意している。
 ****************************************/
#ifndef BOARD_H
#define BOARD_H

//--------------------------------------
// インクルード
//--------------------------------------
#include "piece.h"
#include <vector>

//======================================
// 盤面クラス
//======================================
/**
 * @class  Board
 * @brief  〇×ゲームの盤面
 * @detail マス目(セル)の集合を保持し、駒の取得/設置/状態判定を提供する。
 *         デフォルトサイズは 3×3。
 */
class Board
{
public:
    //--------------------------------------
    // メンバ変数（盤面データ）
    //--------------------------------------
    int width  = 3;             // 横のマス数
    int height = 3;             // 縦のマス数
    std::vector<Piece> cells;   // 全マスの駒(1次元配列, index = y*width + x)

    //======================================
    // コンストラクタ
    //======================================
    /** @brief デフォルト構築 (3×3で初期化) */
    Board();
    /**
     * @brief  サイズ指定で構築
     * @param  w 横マス数
     * @param  h 縦マス数
     */
    explicit Board(int w, int h);

    //======================================
    // 盤面操作
    //======================================
    /** @brief 全マスを空に戻す */
    void Reset();
    /**
     * @brief  盤面サイズを変更し、全マスを空に戻す
     * @param  w 新しい横マス数
     * @param  h 新しい縦マス数
     */
    void Resize(int w, int h);

    /**
     * @brief  指定マスの駒を取得
     * @param  x,y マス座標
     * @return マスの駒。盤外の場合は Piece::Empty。
     */
    Piece Get(int x, int y) const;
    /**
     * @brief  指定マスに駒を設置
     * @detail 盤外座標を渡した場合は何もしない。
     * @param  x,y マス座標
     * @param  p   設置する駒
     */
    void  Set(int x, int y, Piece p);

    //======================================
    // 状態判定
    //======================================
    /** @brief 座標が盤内かを判定 @return 盤内なら true */
    bool  IsValid(int x, int y) const;
    /** @brief 指定マスが盤内かつ空かを判定 @return 空マスなら true */
    bool  IsEmpty(int x, int y) const;
    /** @brief 盤面が満杯(空きマス無し)かを判定 @return 満杯なら true */
    bool  IsFull() const;

    /** @brief 空きマスの数を返す */
    int   EmptyCount() const;
    /**
     * @brief  2D座標を cells の1次元インデックスへ変換
     * @param  x,y マス座標
     * @return cells 内のインデックス
     */
    int   Index(int x, int y) const { return y * width + x; }
};

#endif // BOARD_H
