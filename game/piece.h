/****************************************
 * @file piece.h
 * @brief 駒の種別と座標
 * @author Natsume Shidara
 * @date 2026/05/15
 ****************************************/
#ifndef PIECE_H
#define PIECE_H

enum class Piece
{
    Empty,
    Player,  // 〇
    Enemy,   // ×
};

struct Vec2
{
    int x;
    int y;
};

inline Piece Opponent(Piece p)
{
    if (p == Piece::Player) return Piece::Enemy;
    if (p == Piece::Enemy)  return Piece::Player;
    return Piece::Empty;
}

#endif // PIECE_H
