/****************************************
 * @file   font_atlas.h
 * @brief  DirectWrite動的グリフアトラス
 * @author Natsume Shidara
 * @date   2026/05/22
 *
 * DirectWrite + Direct2D + WIC を用い、必要になった文字を実行時に
 * ラスタライズして D3D11 のアトラステクスチャにキャッシュする。
 * 日本語を含む任意のUnicode文字に対応する。
 *
 * グリフは「白RGB + カバレッジα」のストレートアルファで格納するため、
 * 2Dシェーダ (tex * vertexColor) でそのまま色付け描画できる。
 ****************************************/
#ifndef FONT_ATLAS_H
#define FONT_ATLAS_H

//--------------------------------------
// インクルード
//--------------------------------------
#include <d3d11.h>
#include <cstdint>

//======================================
// グリフアトラス 名前空間
//======================================
namespace FontAtlas
{
    /**
     * @struct GlyphUV
     * @brief  アトラステクスチャ内のグリフ位置 (正規化UV)
     */
    struct GlyphUV
    {
        float u0, v0, u1, v1;  // 左上(u0,v0)・右下(u1,v1) の正規化UV
    };

    /**
     * @brief  グリフアトラスの初期化
     * @detail Direct3D初期化後・COM初期化後に呼ぶこと。多重呼び出しは無視する。
     * @return 初期化に成功したら true
     */
    bool Initialize();

    /** @brief 終了処理 (Direct3D_Finalize より前に呼ぶこと) */
    void Finalize();

    /**
     * @brief  コードポイントのグリフUVを取得 (未キャッシュなら生成)
     * @param  codepoint 取得したい文字のUnicodeコードポイント
     * @param  outUV     [out] グリフのアトラスUV
     * @return 生成・取得に成功したら true
     */
    bool GetGlyph(uint32_t codepoint, GlyphUV& outUV);

    /** @brief アトラステクスチャのSRVを取得 (Sprite描画で使用) */
    ID3D11ShaderResourceView* GetAtlasSRV();
}

#endif // FONT_ATLAS_H
