/****************************************
 * @file font_atlas.h
 * @brief DirectWrite動的グリフアトラス
 * @author Natsume Shidara
 * @date 2026/05/22
 *
 * DirectWrite + Direct2D + WIC で、必要になった文字を実行時に
 * ラスタライズし、D3D11のアトラステクスチャにキャッシュする。
 * 日本語を含む任意のUnicode文字に対応。
 *
 * グリフは「白RGB + カバレッジα」のストレートアルファで格納するため、
 * 2Dシェーダ (tex * vertexColor) でそのまま色付け描画できる。
 ****************************************/
#ifndef FONT_ATLAS_H
#define FONT_ATLAS_H

#include <d3d11.h>
#include <cstdint>

namespace FontAtlas
{
    /** @brief アトラス内のグリフ位置 (正規化UV) */
    struct GlyphUV
    {
        float u0, v0, u1, v1;
    };

    /** @brief 初期化 (Direct3D初期化後・COM初期化後に呼ぶ。多重呼び出しは無視) */
    bool Initialize();

    /** @brief 終了処理 (Direct3D_Finalize より前に呼ぶこと) */
    void Finalize();

    /**
     * @brief コードポイントのグリフUVを取得 (未キャッシュなら生成)
     * @return 生成・取得に成功したら true
     */
    bool GetGlyph(uint32_t codepoint, GlyphUV& outUV);

    /** @brief アトラステクスチャのSRV (Sprite描画用) */
    ID3D11ShaderResourceView* GetAtlasSRV();
}

#endif // FONT_ATLAS_H
