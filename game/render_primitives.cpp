/****************************************
 * @file render_primitives.cpp
 * @brief 2D図形描画ヘルパ実装
 * @author Natsume Shidara
 * @date 2026/05/15
 ****************************************/
#include "render_primitives.h"

#include "sprite.h"
#include "texture.h"

#include <cmath>

using namespace DirectX;

namespace
{
    int g_TexWhite = -1;
    constexpr float PI_F = 3.14159265358979323846f;
}

namespace Prim
{
    void Initialize()
    {
        if (g_TexWhite < 0)
        {
            g_TexWhite = Texture_Load(L"assets/white.png");
        }
    }

    void DrawRect(float x, float y, float w, float h, const XMFLOAT4& color)
    {
        if (g_TexWhite < 0) return;
        Sprite_Draw(g_TexWhite, x, y, w, h, 0.0f, color);
    }

    void DrawLine(float x1, float y1, float x2, float y2,
                  float thickness, const XMFLOAT4& color)
    {
        if (g_TexWhite < 0) return;
        const float dx = x2 - x1;
        const float dy = y2 - y1;
        const float length = std::sqrt(dx * dx + dy * dy);
        if (length < 1e-4f) return;
        const float angle = std::atan2(dy, dx);
        const float midX  = (x1 + x2) * 0.5f;
        const float midY  = (y1 + y2) * 0.5f;
        // Sprite_Draw は矩形の中心を回転軸として扱うため、
        // 描画原点 (dx,dy) は「中心 - 半サイズ」になる。
        Sprite_Draw(g_TexWhite,
                    midX - length * 0.5f,
                    midY - thickness * 0.5f,
                    length, thickness, angle, color);
    }

    void DrawCircle(float cx, float cy, float radius,
                    float thickness, const XMFLOAT4& color, int segments)
    {
        if (segments < 3) segments = 3;
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
        DrawLine(cx - halfSize, cy - halfSize, cx + halfSize, cy + halfSize,
                 thickness, color);
        DrawLine(cx - halfSize, cy + halfSize, cx + halfSize, cy - halfSize,
                 thickness, color);
    }
}
