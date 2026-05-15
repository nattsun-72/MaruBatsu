/****************************************
 * @file shader_shadow_map_skinned.h
 * @brief スキニングモデル用シャドウマップ生成シェーダー管理
 * @author Natsume Shidara
 * @date 2025/12/10
 *
 * ボーンアニメーション付きモデルのシャドウマップ描画用
 * シェーダー初期化・定数バッファ設定APIを提供する。
 ****************************************/

#ifndef SHADER_SHADOW_MAP_SKINNED_H
#define SHADER_SHADOW_MAP_SKINNED_H

#include <d3d11.h>
#include <DirectXMath.h>

/** @brief シェーダーリソースの初期化 */
bool ShaderShadowMapSkinned_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);

/** @brief シェーダーリソースの解放 */
void ShaderShadowMapSkinned_Finalize();

/** @brief 描画パイプラインにシェーダーをバインド */
void ShaderShadowMapSkinned_Begin();

/** @brief ライトのビュー・プロジェクション行列を設定 */
void ShaderShadowMapSkinned_SetLightViewProjection(const DirectX::XMFLOAT4X4& matrix);

/** @brief ワールド行列を設定（GPU転送を実行） */
void ShaderShadowMapSkinned_SetWorldMatrix(const DirectX::XMMATRIX& matrix);

/** @brief ボーン行列配列を設定 */
void ShaderShadowMapSkinned_SetBoneMatrices(const DirectX::XMFLOAT4X4* bones, int boneCount);

#endif // SHADER_SHADOW_MAP_SKINNED_H
