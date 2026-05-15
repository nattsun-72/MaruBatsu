/****************************************
 * @file shader_shadow_map.h
 * @brief シャドウマップ生成用シェーダー管理
 * @author Natsume Shidara
 * @date 2025/12/10
 ****************************************/

#ifndef SHADER_SHADOW_MAP_H
#define SHADER_SHADOW_MAP_H

#include <d3d11.h>
#include <DirectXMath.h>

/** @brief シェーダーリソースの初期化（HLSLランタイムコンパイル） */
bool ShaderShadowMap_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);

/** @brief シェーダーリソースの解放 */
void ShaderShadowMap_Finalize();

/** @brief 描画パイプラインにシェーダーをバインド */
void ShaderShadowMap_Begin();

/** @brief ライトのビュー・プロジェクション行列を設定（GPU転送はSetWorldMatrix時に一括実行） */
void ShaderShadowMap_SetLightViewProjection(const DirectX::XMFLOAT4X4& matrix);

/** @brief ワールド行列を設定しGPUへ転送 */
void ShaderShadowMap_SetWorldMatrix(const DirectX::XMMATRIX& matrix);

#endif // SHADER_SHADOW_MAP_H