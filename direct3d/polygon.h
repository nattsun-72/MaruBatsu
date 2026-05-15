/****************************************
 * @file polygon.h
 * @brief 2Dポリゴン（点群円）描画モジュールのヘッダ
 * @author Natsume Shidara
 * @date 2025/06/06
 *
 * 円形のポイントリストを2D描画するための
 * インターフェースを提供する。
 ****************************************/

#ifndef POLYGON_H
#define POLYGON_H

#include <d3d11.h>

/** @brief ポリゴン初期化（頂点バッファ作成） */
void Polygon_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
/** @brief ポリゴン終了処理 */
void Polygon_Finalize(void);
/** @brief ポリゴン描画（ポイントリスト） */
void Polygon_Draw(void);

#endif // POLYGON_H
