/****************************************
 * @file shader_field.h
 * @brief メッシュフィールド描画用シェーダー管理
 * @author Natsume Shidara
 * @date 2025/09/26
 *
 * 地形描画に使用する頂点・ピクセルシェーダーの
 * 初期化・設定・描画パイプラインバインドAPIを提供する。
 ****************************************/

#ifndef SHADERF_IELD_H
#define SHADER_FIELD_H

#include <DirectXMath.h>
#include <d3d11.h>

/** @brief シェーダーリソースの初期化 */
bool ShaderField_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);

/** @brief シェーダーリソースの解放 */
void ShaderField_Finalize();

/** @brief ワールド行列の設定 */
void ShaderField_SetWorldMatrix(const DirectX::XMMATRIX& matrix);

/** @brief マテリアルカラーの設定 */
void ShaderField_SetColor(const DirectX::XMFLOAT4& color);

/** @brief 描画パイプラインにシェーダーをバインド */
void ShaderField_Begin();
#endif // SHADER_FIELD_H
