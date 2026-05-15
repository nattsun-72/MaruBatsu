/****************************************
 * @file shader_skinned.h
 * @brief スキニングモデル描画用シェーダー管理
 * @author Natsume Shidara
 * @date 2025/12/10
 *
 * ボーンアニメーション付きモデルの描画用シェーダーの
 * 初期化・定数バッファ設定・シャドウマップ参照APIを提供する。
 ****************************************/

#ifndef SHADER_SKINNED_H
#define SHADER_SKINNED_H

#include <d3d11.h>
#include <DirectXMath.h>

/** @brief シェーダーリソースの初期化 */
bool ShaderSkinned_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);

/** @brief シェーダーリソースの解放 */
void ShaderSkinned_Finalize();

/** @brief 描画パイプラインにシェーダーをバインド */
void ShaderSkinned_Begin();

/** @brief ワールド行列の設定 */
void ShaderSkinned_SetWorldMatrix(const DirectX::XMMATRIX& matrix);

/** @brief ボーン行列配列の設定 */
void ShaderSkinned_SetBoneMatrices(const DirectX::XMFLOAT4X4* bones, int boneCount);

/** @brief マテリアルカラーの設定 */
void ShaderSkinned_SetColor(const DirectX::XMFLOAT4& color);

/** @brief ライトのビュー・プロジェクション行列を設定（影描画用） */
void ShaderSkinned_SetLightViewProjection(const DirectX::XMFLOAT4X4& matrix);

/** @brief シャドウマップテクスチャをピクセルシェーダーにバインド */
void ShaderSkinned_SetShadowMap(ID3D11ShaderResourceView* pShadowSRV);

#endif // SHADER_SKINNED_H
