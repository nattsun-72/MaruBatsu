/****************************************
 * @file shader_field.h
 * @brief フィールド描画用シェーダー管理（行列・ライティング制御対応版）
 * @author Natsume Shidara
 * @date 2025/09/26
 *
 * 地形描画に使用する頂点・ピクセルシェーダーの初期化・設定APIを提供する。
 * プロジェクション・ビュー・ワールド行列の個別設定および
 * 環境光・平行光のライティング制御に対応。
 ****************************************/

#ifndef SHADERF_IELD_H
#define SHADER_FIELD_H

#include <DirectXMath.h>
#include <d3d11.h>

/** @brief シェーダーリソースの初期化 */
bool ShaderField_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);

/** @brief シェーダーリソースの解放 */
void ShaderField_Finalize();

/** @brief プロジェクション行列の設定 */
void ShaderField_SetProjectionMatrix(const DirectX::XMMATRIX& matrix);

/** @brief ワールド行列の設定 */
void ShaderField_SetWorldMatrix(const DirectX::XMMATRIX& matrix);

/** @brief ビュー行列の設定 */
void ShaderField_SetViewMatrix(const DirectX::XMMATRIX& matrix);

/** @brief 環境光カラーの設定 */
void ShaderField_SetAmbientColor(const DirectX::XMFLOAT4& ambient);

/** @brief 平行光源の方向と色を設定 */
void ShaderField_SetDirectional(const DirectX::XMFLOAT4& dirWorldVec, const DirectX::XMFLOAT4& dirColor);

/** @brief 描画パイプラインにシェーダーをバインド */
void ShaderField_Begin();
#endif // SHADER_FIELD_H
