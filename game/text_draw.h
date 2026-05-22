/****************************************
 * @file text_draw.h
 * @brief ゲーム内テキスト描画 (DirectWrite動的グリフアトラス)
 * @author Natsume Shidara
 * @date 2026/05/15
 * @update 2026/05/22 - 日本語対応 (DirectWriteグリフアトラス)
 *
 * UTF-8文字列を受け取り、FontAtlas が実行時にラスタライズした
 * グリフをスプライト描画する。日本語を含む任意のUnicodeに対応。
 ****************************************/
#ifndef TEXT_DRAW_H
#define TEXT_DRAW_H

#include <DirectXMath.h>

namespace Text
{
    /** @brief フォントアトラスの初期化 (多重呼び出しは無視) */
    void Initialize();

    /** @brief 終了処理 (Direct3D_Finalize より前に呼ぶこと) */
    void Finalize();

    /**
     * @brief 文字列をスクリーン座標 (x, y) を左上として描画
     * @param x        左上X座標
     * @param y        左上Y座標
     * @param text     UTF-8文字列 (\n で改行)。日本語可。
     * @param charSize 1文字の表示サイズ(正方形・等幅)
     * @param color    頂点カラー
     */
    void Draw(float x, float y, const char* text,
              float charSize, const DirectX::XMFLOAT4& color = {1, 1, 1, 1});

    /** @brief 文字列の表示幅 (charSize × コードポイント数のうち最長行) */
    float MeasureWidth(const char* text, float charSize);
}

#endif // TEXT_DRAW_H
