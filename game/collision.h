/****************************************
 * @file   collision.h
 * @brief  2D衝突判定モジュール
 * @author Natsume Shidara
 * @date   2025/01/02
 * @update 2026/05/15 - 〇×ローグライト用2D版へスリム化
 *
 * 円・矩形(AABB)同士の重なり判定と、点の内外判定を提供する。
 * マウス座標からマス目を求める処理などで使用する。
 ****************************************/
#ifndef COLLISION_H
#define COLLISION_H

#include <DirectXMath.h>

//======================================
// 形状定義
//======================================
/**
 * @struct Circle
 * @brief  2D 円形
 */
struct Circle {
    DirectX::XMFLOAT2 center;  // 中心座標
    float             radius;  // 半径
};

/**
 * @struct Box
 * @brief  2D 軸平行矩形 (AABB)
 * @detail 中心座標とハーフサイズ(中心から辺までの距離)で表現する。
 */
struct Box {
    DirectX::XMFLOAT2 center;       // 中心座標
    float             half_width;  // 横方向のハーフサイズ
    float             half_height; // 縦方向のハーフサイズ
};

//======================================
// 判定関数
//======================================
/**
 * @brief  円同士の重なり判定
 * @param  a,b 判定する2つの円
 * @return 重なっていれば true
 */
bool Collision_IsOverlapCircle(const Circle& a, const Circle& b);

/**
 * @brief  矩形(AABB)同士の重なり判定
 * @param  a,b 判定する2つの矩形
 * @return 重なっていれば true
 */
bool Collision_IsOverlapBox(const Box& a, const Box& b);

/**
 * @brief  点が矩形の内側にあるか判定
 * @detail マウス座標 → マス判定などに使用する。
 * @param  px,py 判定する点の座標
 * @param  box   判定対象の矩形
 * @return 点が矩形内(辺上を含む)なら true
 */
bool Collision_IsPointInBox(float px, float py, const Box& box);

#endif // COLLISION_H
