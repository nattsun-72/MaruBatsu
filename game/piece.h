/****************************************
 * @file   piece.h
 * @brief  駒の種別・2D整数座標・陣営反転ヘルパ
 * @author Natsume Shidara
 * @date   2026/05/15
 *
 * 〇×ゲームの最小データ型をまとめたヘッダ。
 * 盤面(Board)・各種フック・AI など、ほぼ全モジュールが参照する。
 ****************************************/
#ifndef PIECE_H
#define PIECE_H

//======================================
// 駒の種別
//======================================
/**
 * @enum   Piece
 * @brief  1マスに置かれている駒の状態
 * @detail Empty=空きマス / Player=プレイヤーの〇 / Enemy=AI(ボス)の×。
 *         盤面はこの値の集合で表現される。
 */
enum class Piece
{
    Empty,   // 空きマス
    Player,  // プレイヤーの駒（〇）
    Enemy,   // 敵(ボス)の駒（×）
};

//======================================
// 2D整数座標
//======================================
/**
 * @struct Vec2
 * @brief  盤面のマス座標などに使う2D整数ベクトル
 * @detail x は列(横)、y は行(縦)。負値は「無効座標」を表す用途にも使う。
 */
struct Vec2
{
    int x;  // 列(横方向)のインデックス
    int y;  // 行(縦方向)のインデックス
};

//======================================
// ヘルパ関数
//======================================
/**
 * @brief  相手陣営の駒種を返す
 * @detail Player⇔Enemy を反転する。ターン交代の判定などで使用。
 * @param  p 反転元の駒種
 * @return 相手の駒種。p が Empty の場合は Empty を返す。
 */
inline Piece Opponent(Piece p)
{
    if (p == Piece::Player) return Piece::Enemy;
    if (p == Piece::Enemy)  return Piece::Player;
    return Piece::Empty;
}

#endif // PIECE_H
