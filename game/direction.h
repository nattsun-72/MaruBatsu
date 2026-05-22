/****************************************
 * @file   direction.h
 * @brief  4方向の列挙と、方向→ベクトル/名称への変換ヘルパ
 * @author Natsume Shidara
 * @date   2026/05/15
 *
 * 氷盤の支配者(全駒スライド)・押し出し・ワープ駒など、
 * 「方向」を扱うアビリティ全般で共通利用する。
 ****************************************/
#ifndef DIRECTION_H
#define DIRECTION_H

//======================================
// 方向の種別
//======================================
/**
 * @enum   Direction
 * @brief  上下左右の4方向
 * @detail スライド方向・押し出し方向などに用いる。
 */
enum class Direction
{
    Up,     // 上
    Down,   // 下
    Left,   // 左
    Right,  // 右
};

//======================================
// 変換ヘルパ
//======================================
/**
 * @brief  方向の X 成分(-1/0/+1)を返す
 * @param  d 対象の方向
 * @return Left=-1 / Right=+1 / その他=0
 */
inline int DirX(Direction d)
{
    switch (d)
    {
    case Direction::Left:  return -1;
    case Direction::Right: return  1;
    default:               return  0;
    }
}

/**
 * @brief  方向の Y 成分(-1/0/+1)を返す
 * @detail 画面座標系に合わせ、Up が -1・Down が +1。
 * @param  d 対象の方向
 * @return Up=-1 / Down=+1 / その他=0
 */
inline int DirY(Direction d)
{
    switch (d)
    {
    case Direction::Up:   return -1;
    case Direction::Down: return  1;
    default:              return  0;
    }
}

/**
 * @brief  方向の日本語名(1文字)を返す
 * @detail UI(方向インジケータ等)の表示に使用する。
 * @param  d 対象の方向
 * @return "上"/"下"/"左"/"右"。不正値は "?"。
 */
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
