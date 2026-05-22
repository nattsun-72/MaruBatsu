/****************************************
 * @file text_draw.h
 * @brief ゲーム内テキスト描画 (consolab ASCIIフォント)
 * @author Natsume Shidara
 * @date 2026/05/15
 *
 * consolab_ascii_512.png (16x16 ASCIIアトラス, セル32px) を
 * Sprite_Draw のUV切り抜きで描く。任意の位置・サイズ・色に対応。
 ****************************************/
#ifndef TEXT_DRAW_H
#define TEXT_DRAW_H

#include <DirectXMath.h>

namespace Text
{
    /** @brief フォントテクスチャのロード */
    void Initialize();

    /**
     * @brief 文字列をスクリーン座標 (x, y) を左上として描画
     * @param x        左上X座標
     * @param y        左上Y座標
     * @param text     ASCII文字列 (\n で改行)
     * @param charSize 1文字の表示サイズ(正方形)
     * @param color    頂点カラー (グロー演出は color のRGBを高めに指定)
     */
    void Draw(float x, float y, const char* text,
              float charSize, const DirectX::XMFLOAT4& color = {1, 1, 1, 1});

    /** @brief 文字列の表示幅 (charSize × 文字数のうち最長行) */
    float MeasureWidth(const char* text, float charSize);
}

#endif // TEXT_DRAW_H
