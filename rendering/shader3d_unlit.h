/****************************************
 * @file shader3d_unlit.h
 * @brief 3Dモデル用アンリット（ライティング無し）シェーダー管理
 * @author Natsume Shidara
 * @date 2025/11/21
 *
 * ライティングを適用しない3Dモデル描画用シェーダーの
 * 初期化・設定・描画パイプラインバインドAPIを提供する。
 * スカイボックスやデバッグ表示等に使用。
 ****************************************/

#ifndef SHADER3D_UNLIT_H
#define SHADER3D_UNLIT_H

#include <DirectXMath.h>
#include <d3d11.h>

/** @brief シェーダーリソースの初期化 */
void Shader3D_Unlit_Initialize();

/** @brief シェーダーリソースの解放 */
void Shader3D_Unlit_Finalize();

/** @brief ワールド行列の設定 */
void Shader3D_Unlit_SetWorldMatrix(const DirectX::XMMATRIX& matrix);

/** @brief マテリアルカラーの設定 */
void Shader3D_Unlit_SetColor(const DirectX::XMFLOAT4& color);

/** @brief 描画パイプラインにシェーダーをバインド */
void Shader3D_Unlit_Begin();

#endif // SHADER3D_UNLIT_H