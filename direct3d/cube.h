/****************************************
 * @file cube.h
 * @brief 3Dキューブ描画モジュールのヘッダ
 * @author Natsume Shidara
 * @date 2025/06/06
 *
 * テクスチャアトラス対応の立方体メッシュの
 * 初期化・描画・AABB取得インターフェースを提供する。
 ****************************************/
#ifndef CUBE_H
#define CUBE_H

#include <d3d11.h>
#include <DirectXMath.h>
#include "collision.h"

/** @brief キューブの初期化（頂点バッファ作成） */
void Cube_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
/** @brief キューブの終了処理 */
void Cube_Finalize(void);
/** @brief キューブの更新処理 */
void Cube_Update(double elapsed_time);

/** @brief キューブ描画（テクスチャID・ワールド行列・カラー指定） */
void Cube_Draw(int texId, const DirectX::XMMATRIX& mtxWorld = DirectX::XMMatrixIdentity(), const DirectX::XMFLOAT4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

/** @brief 指定位置のキューブAABBを取得 */
AABB Cube_GetAABB(const DirectX::XMFLOAT3& position);

/** @brief シャドウマップ用キューブ描画 */
void Cube_DrawShadow(const DirectX::XMMATRIX& mtxWorld);

#endif // CUBE_H