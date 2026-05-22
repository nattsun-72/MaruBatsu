/****************************************
 * @file   render_primitives.h
 * @brief  2D図形描画ヘルパ (矩形 / 直線 / 円 / ×)
 * @author Natsume Shidara
 * @date   2026/05/15
 *
 * white.png を Sprite_Draw に通すことで、専用テクスチャ無しで
 * ミニマル抽象ビジュアル(線・図形)を描画する。
 * 利用前に Prim::Initialize() を必ず呼ぶこと。
 ****************************************/
#ifndef RENDER_PRIMITIVES_H
#define RENDER_PRIMITIVES_H

#include <DirectXMath.h>

//======================================
// 2D図形描画 名前空間
//======================================
namespace Prim
{
    /** @brief 初期化 — 内部で white.png をロードする */
    void Initialize();

    /**
     * @brief  塗りつぶし矩形を描画
     * @param  x,y   左上座標
     * @param  w,h   幅・高さ
     * @param  color 塗り色
     */
    void DrawRect(float x, float y, float w, float h,
                  const DirectX::XMFLOAT4& color);

    /**
     * @brief  直線を描画
     * @param  x1,y1     始点
     * @param  x2,y2     終点
     * @param  thickness 線の太さ
     * @param  color     線の色
     */
    void DrawLine(float x1, float y1, float x2, float y2,
                  float thickness, const DirectX::XMFLOAT4& color);

    /**
     * @brief  円のアウトラインを描画
     * @detail segments 本の弦(直線)で円周を近似する。
     * @param  cx,cy     中心座標
     * @param  radius    半径
     * @param  thickness 線の太さ
     * @param  color     線の色
     * @param  segments  円周の分割数 (既定32)
     */
    void DrawCircle(float cx, float cy, float radius,
                    float thickness, const DirectX::XMFLOAT4& color,
                    int segments = 32);

    /**
     * @brief  × 記号を描画
     * @detail (cx,cy) を中心とした2本の交差線を引く。
     * @param  cx,cy     中心座標
     * @param  halfSize  中心から各端点までの距離
     * @param  thickness 線の太さ
     * @param  color     線の色
     */
    void DrawCross(float cx, float cy, float halfSize,
                   float thickness, const DirectX::XMFLOAT4& color);
}

#endif // RENDER_PRIMITIVES_H
