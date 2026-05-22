/****************************************
 * @file    collision.h
 * @brief   コリジョン判定モジュール (2D版)
 * @author  Natsume Shidara
 * @date    2025/01/02
 * @update  2026/05/15 - 〇×ローグライト用2D版へスリム化
 ****************************************/
#ifndef COLLISION_H
#define COLLISION_H

#include <DirectXMath.h>

//--------------------------------------
// 形状定義構造体群 (2D)
//--------------------------------------

/** @struct Circle @brief 2D 円形 */
struct Circle {
    DirectX::XMFLOAT2 center;
    float radius;
};

/** @struct Box @brief 2D AABB (中心とハーフサイズ) */
struct Box {
    DirectX::XMFLOAT2 center;
    float half_width;
    float half_height;
};

//--------------------------------------
// 判定関数プロトタイプ
//--------------------------------------

/** @brief 円同士の重なり判定 */
bool Collision_IsOverlapCircle(const Circle& a, const Circle& b);

/** @brief AABB同士の重なり判定 */
bool Collision_IsOverlapBox(const Box& a, const Box& b);

/** @brief 点がBox内にあるか判定 (マウス座標 → マス判定で使用) */
bool Collision_IsPointInBox(float px, float py, const Box& box);

#endif // COLLISION_H
