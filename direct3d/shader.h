/****************************************
 * @file shader.h
 * @brief 2Dシェーダー管理モジュールのヘッダ
 * @author Natsume Shidara
 * @date 2025/06/06
 *
 * 2D描画用の頂点シェーダー・ピクセルシェーダーの
 * 初期化・行列設定・パイプライン設定インターフェースを提供する。
 ****************************************/

#ifndef SHADER_H
#define	SHADER_H

#include <d3d11.h>
#include <DirectXMath.h>

/** @brief 2Dシェーダー初期化（シェーダー読み込み・定数バッファ作成） */
bool Shader_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
/** @brief 2Dシェーダー終了処理 */
void Shader_Finalize();

/** @brief 投影行列を定数バッファに設定 */
void Shader_SetProjectionMatrix(const DirectX::XMMATRIX& matrix);
/** @brief ワールド行列を定数バッファに設定 */
void Shader_SetWorldMatrix(const DirectX::XMMATRIX& matrix);

/** @brief 2Dシェーダーを描画パイプラインに設定 */
void Shader_Begin();

#endif // SHADER_H

