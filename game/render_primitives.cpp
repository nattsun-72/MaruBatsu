/****************************************
 * @file   render_primitives.cpp
 * @brief  2D図形描画ヘルパの実装
 * @author Natsume Shidara
 * @date   2026/05/15
 ****************************************/
#include "render_primitives.h"

#include "sprite.h"
#include "texture.h"

#include <cmath>

using namespace DirectX;

//--------------------------------------
// 内部状態・定数
//--------------------------------------
namespace
{
    int g_TexWhite = -1;                              // 白1pxテクスチャのID
    constexpr float PI_F = 3.14159265358979323846f;   // 円周率
}

namespace Prim
{
    //======================================
    // 初期化
    //======================================
    void Initialize()
    {
        // 多重初期化を避けるため、未ロード時のみ読み込む
        if (g_TexWhite < 0)
        {
            g_TexWhite = Texture_Load(L"assets/white.png");
        }
    }

    //======================================
    // 図形描画
    //======================================
    void DrawRect(float x, float y, float w, float h, const XMFLOAT4& color)
    {
        if (g_TexWhite < 0) return;
        Sprite_Draw(g_TexWhite, x, y, w, h, 0.0f, color);
    }

    void DrawLine(float x1, float y1, float x2, float y2,
                  float thickness, const XMFLOAT4& color)
    {
        if (g_TexWhite < 0) return;

        // 始点→終点のベクトルから、線の長さと角度を求める
        const float dx = x2 - x1;
        const float dy = y2 - y1;
        const float length = std::sqrt(dx * dx + dy * dy);
        if (length < 1e-4f) return;          // 長さ0の線は描かない
        const float angle = std::atan2(dy, dx);
        const float midX  = (x1 + x2) * 0.5f;
        const float midY  = (y1 + y2) * 0.5f;

        // Sprite_Draw は矩形の中心を回転軸として扱うため、
        // 描画原点 (x,y) には「中心 - 半サイズ」を渡す。
        Sprite_Draw(g_TexWhite,
                    midX - length * 0.5f,
                    midY - thickness * 0.5f,
                    length, thickness, angle, color);
    }

    void DrawCircle(float cx, float cy, float radius,
                    float thickness, const XMFLOAT4& color, int segments)
    {
        if (segments < 3) segments = 3;      // 最低限の分割数を保証

        // 円周上の点を順に結ぶ弦で、円のアウトラインを近似する
        float prevX = cx + radius;
        float prevY = cy;
        for (int i = 1; i <= segments; ++i)
        {
            const float a  = static_cast<float>(i) / segments * 2.0f * PI_F;
            const float nx = cx + std::cos(a) * radius;
            const float ny = cy + std::sin(a) * radius;
            DrawLine(prevX, prevY, nx, ny, thickness, color);
            prevX = nx;
            prevY = ny;
        }
    }

    void DrawCross(float cx, float cy, float halfSize,
                   float thickness, const XMFLOAT4& color)
    {
        // 左上→右下、左下→右上 の2本で × を構成する
        DrawLine(cx - halfSize, cy - halfSize, cx + halfSize, cy + halfSize,
                 thickness, color);
        DrawLine(cx - halfSize, cy + halfSize, cx + halfSize, cy - halfSize,
                 thickness, color);
    }
}
