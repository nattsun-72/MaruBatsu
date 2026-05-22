/****************************************
 * @file    collision.cpp
 * @brief   コリジョン判定実装 (2D版)
 * @author  Natsume Shidara
 * @date    2025/01/02
 * @update  2026/05/15 - 〇×ローグライト用2D版へスリム化
 ****************************************/

#include "collision.h"

bool Collision_IsOverlapCircle(const Circle& a, const Circle& b)
{
    float dx = b.center.x - a.center.x;
    float dy = b.center.y - a.center.y;
    float rSum = a.radius + b.radius;
    return (dx * dx + dy * dy) <= (rSum * rSum);
}

bool Collision_IsOverlapBox(const Box& a, const Box& b)
{
    float aLeft   = a.center.x - a.half_width;
    float aRight  = a.center.x + a.half_width;
    float aTop    = a.center.y - a.half_height;
    float aBottom = a.center.y + a.half_height;

    float bLeft   = b.center.x - b.half_width;
    float bRight  = b.center.x + b.half_width;
    float bTop    = b.center.y - b.half_height;
    float bBottom = b.center.y + b.half_height;

    return !(aRight < bLeft || aLeft > bRight || aBottom < bTop || aTop > bBottom);
}

bool Collision_IsPointInBox(float px, float py, const Box& box)
{
    return (px >= box.center.x - box.half_width)
        && (px <= box.center.x + box.half_width)
        && (py >= box.center.y - box.half_height)
        && (py <= box.center.y + box.half_height);
}
