/****************************************
 * @file   text_draw.cpp
 * @brief  ゲーム内テキスト描画の実装
 * @author Natsume Shidara
 * @date   2026/05/15
 * @update 2026/05/22 - 日本語対応 (UTF-8デコード + FontAtlas)
 *
 * 等幅描画: 各コードポイントを charSize 四方のセルとして送る。
 * 日本語は元々ほぼ正方形のため、等幅でも破綻しない。
 ****************************************/
#include "text_draw.h"

#include "font_atlas.h"
#include "sprite.h"

#include <cstdint>

//--------------------------------------
// 内部ヘルパ
//--------------------------------------
namespace
{
    /**
     * @brief  UTF-8文字列から1文字をデコードし、ポインタを次の文字へ進める
     * @param  p [in,out] 文字列ポインタ。デコード後、次の文字位置へ進む。
     * @return デコードしたUnicodeコードポイント。不正バイトは U+FFFD。
     */
    uint32_t DecodeUtf8(const char*& p)
    {
        const unsigned char c0 = static_cast<unsigned char>(*p);

        // 1バイト (ASCII): そのまま返す
        if (c0 < 0x80)
        {
            ++p;
            return c0;
        }

        // 先頭バイトのビットパターンから、続く継続バイト数を判定する
        uint32_t cp = 0;
        int extra = 0;
        if ((c0 & 0xE0) == 0xC0)      { cp = c0 & 0x1F; extra = 1; }  // 2バイト文字
        else if ((c0 & 0xF0) == 0xE0) { cp = c0 & 0x0F; extra = 2; }  // 3バイト文字
        else if ((c0 & 0xF8) == 0xF0) { cp = c0 & 0x07; extra = 3; }  // 4バイト文字
        else { ++p; return 0xFFFD; }                                 // 不正な先頭バイト

        // 継続バイトを連結してコードポイントを組み立てる
        ++p;
        for (int i = 0; i < extra; ++i)
        {
            const unsigned char cc = static_cast<unsigned char>(*p);
            if ((cc & 0xC0) != 0x80) return 0xFFFD;  // 継続バイトでない(不正)
            cp = (cp << 6) | (cc & 0x3F);
            ++p;
        }
        return cp;
    }
}

namespace Text
{
    //======================================
    // 初期化・終了
    //======================================
    void Initialize()
    {
        FontAtlas::Initialize();
    }

    void Finalize()
    {
        FontAtlas::Finalize();
    }

    //======================================
    // 描画・計測
    //======================================
    void Draw(float x, float y, const char* text,
              float charSize, const DirectX::XMFLOAT4& color)
    {
        if (!text) return;
        ID3D11ShaderResourceView* atlas = FontAtlas::GetAtlasSRV();
        if (!atlas) return;

        float cursorX = x;
        float cursorY = y;
        for (const char* p = text; *p; )
        {
            // 改行: カーソルを次の行頭へ送る
            if (*p == '\n')
            {
                cursorX = x;
                cursorY += charSize;
                ++p;
                continue;
            }

            const uint32_t cp = DecodeUtf8(p);
            if (cp == ' ')                  // 空白はグリフ無し、送りのみ進める
            {
                cursorX += charSize;
                continue;
            }

            // グリフのアトラスUVを取得し、1文字分のスプライトを描画する
            FontAtlas::GlyphUV uv;
            if (FontAtlas::GetGlyph(cp, uv))
            {
                Sprite_Draw(atlas, cursorX, cursorY, charSize, charSize,
                            uv.u0, uv.v0, uv.u1, uv.v1, 0.0f, color);
            }
            cursorX += charSize;
        }
    }

    float MeasureWidth(const char* text, float charSize)
    {
        if (!text) return 0.0f;

        // 各行のコードポイント数を数え、最長行を採用する
        int maxLen = 0;
        int cur    = 0;
        for (const char* p = text; *p; )
        {
            if (*p == '\n')
            {
                if (cur > maxLen) maxLen = cur;
                cur = 0;
                ++p;
                continue;
            }
            DecodeUtf8(p);
            ++cur;
        }
        if (cur > maxLen) maxLen = cur;
        return static_cast<float>(maxLen) * charSize;
    }
}
