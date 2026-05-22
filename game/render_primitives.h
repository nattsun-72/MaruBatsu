/****************************************
 * @file render_primitives.h
 * @brief 2D図形描画ヘルパ (矩形/直線/円/×)
 * @author Natsume Shidara
 * @date 2026/05/15
 *
 * white.png を Sprite_Draw に通すことで、テクスチャ無しの
 * ミニマル抽象ビジュアルを描画する。
 * 利用前に RenderPrim_Initialize() を必ず呼ぶこと。
 ****************************************/
#ifndef RENDER_PRIMITIVES_H
#define RENDER_PRIMITIVES_H

#include <DirectXMath.h>

namespace Prim
{
    /** @brief 内部で white.png をロード */
    void Initialize();

    /** @brief 塗りつぶし矩形 (display_x, display_y は左上) */
    void DrawRect(float x, float y, float w, float h,
                  const DirectX::XMFLOAT4& color);

    /** @brief 太さ thickness の直線 ((x1,y1) → (x2,y2)) */
    void DrawLine(float x1, float y1, float x2, float y2,
                  float thickness, const DirectX::XMFLOAT4& color);

    /** @brief 円のアウトライン (segments個の弦で近似) */
    void DrawCircle(float cx, float cy, float radius,
                    float thickness, const DirectX::XMFLOAT4& color,
                    int segments = 32);

    /** @brief × (cx,cy を中心とした2本の交差線) */
    void DrawCross(float cx, float cy, float halfSize,
                   float thickness, const DirectX::XMFLOAT4& color);
}

#endif // RENDER_PRIMITIVES_H
