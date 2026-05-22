/****************************************
 * @file direction.h
 * @brief 4方向の列挙 + ベクトル変換ヘルパ
 * @author Natsume Shidara
 * @date 2026/05/15
 *
 * 氷盤の支配者・押し出し・ワープ駒など、方向を扱う
 * アビリティ全般で使用する。
 ****************************************/
#ifndef DIRECTION_H
#define DIRECTION_H

enum class Direction
{
    Up,
    Down,
    Left,
    Right,
};

inline int DirX(Direction d)
{
    switch (d)
    {
    case Direction::Left:  return -1;
    case Direction::Right: return  1;
    default:               return  0;
    }
}

inline int DirY(Direction d)
{
    switch (d)
    {
    case Direction::Up:   return -1;
    case Direction::Down: return  1;
    default:              return  0;
    }
}

inline const char* DirectionName(Direction d)
{
    switch (d)
    {
    case Direction::Up:    return "上";
    case Direction::Down:  return "下";
    case Direction::Left:  return "左";
    case Direction::Right: return "右";
    }
    return "?";
}

#endif // DIRECTION_H
