/****************************************
 * @file grid.h
 * @brief XZ平面デバッググリッド描画モジュールのヘッダ
 * @author Natsume Shidara
 * @date 2025/06/06
 *
 * 3D空間のXZ平面にライングリッドを描画する。
 ****************************************/

#ifndef GRID_H
#define GRID_H

#include <d3d11.h>

/** @brief グリッド初期化（頂点バッファ・テクスチャ生成） */
void Grid_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
/** @brief グリッド終了処理 */
void Grid_Finalize(void);
/** @brief グリッド描画 */
void Grid_Draw(void);

#endif // GRID_H
