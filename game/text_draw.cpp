/****************************************
 * @file text_draw.cpp
 * @brief ゲーム内テキスト描画の実装
 * @author Natsume Shidara
 * @date 2026/05/15
 ****************************************/
#include "text_draw.h"

#include "sprite.h"
#include "texture.h"

namespace
{
    int g_TexFont = -1;
    constexpr float FONT_CELL = 32.0f;   // 512 / 16
    constexpr int   FONT_GRID = 16;
}

namespace Text
{
    void Initialize()
    {
        if (g_TexFont < 0)
        {
            g_TexFont = Texture_Load(L"assets/consolab_ascii_512.png");
        }
    }

    void Draw(float x, float y, const char* text,
              float charSize, const DirectX::XMFLOAT4& color)
    {
        if (g_TexFont < 0 || !text) return;

        float cursorX = x;
        float cursorY = y;
        for (const char* p = text; *p; ++p)
        {
            unsigned char c = static_cast<unsigned char>(*p);
            if (c == '\n')
            {
                cursorX = x;
                cursorY += charSize;
                continue;
            }
            const float u0 = (c % FONT_GRID) * FONT_CELL;
            const float v0 = (c / FONT_GRID) * FONT_CELL;
            Sprite_Draw(g_TexFont,
                        cursorX, cursorY,
                        u0, v0, FONT_CELL, FONT_CELL,
                        charSize, charSize,
                        0.0f, color);
            cursorX += charSize;
        }
    }

    float MeasureWidth(const char* text, float charSize)
    {
        if (!text) return 0.0f;
        int  maxLen = 0;
        int  cur = 0;
        for (const char* p = text; *p; ++p)
        {
            if (*p == '\n') { if (cur > maxLen) maxLen = cur; cur = 0; }
            else            { ++cur; }
        }
        if (cur > maxLen) maxLen = cur;
        return static_cast<float>(maxLen) * charSize;
    }
}
