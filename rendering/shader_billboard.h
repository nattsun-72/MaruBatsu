/****************************************
 * @file shader_billboard.h
 * @brief ビルボード用シェーダー管理
 * @author Natsume Shidara
 * @date 2025/06/10
 *
 * ビルボード描画用の頂点・ピクセルシェーダーおよび
 * 定数バッファ（ワールド行列・カラー・UV制御）の設定APIを提供する。
 ****************************************/

#ifndef SHADER_BILLBOARD_H
#define SHADER_BILLBOARD_H

#include <DirectXMath.h>
#include <d3d11.h>

/** @brief UV変換パラメータ（スケールとオフセット） */
struct UVParameter
{
    DirectX::XMFLOAT2 scale;
    DirectX::XMFLOAT2 translation;
};

/** @brief シェーダーリソースの初期化 */
bool Shader_Billboard_Initialize();

/** @brief シェーダーリソースの解放 */
void Shader_Billboard_Finalize();

/** @brief ワールド行列の設定 */
void Shader_Billboard_SetWorldMatrix(const DirectX::XMMATRIX& matrix);

/** @brief マテリアルカラーの設定 */
void Shader_Billboard_SetColor(const DirectX::XMFLOAT4& color);

/** @brief UVパラメータの設定（テクスチャ切り抜き用） */
void Shader_Billboard_SetUVParameter(const UVParameter& parameter);

/** @brief 描画パイプラインにシェーダーをバインド */
void Shader_Billboard_Begin();

#endif // SHADER_BILLBOARD_H
