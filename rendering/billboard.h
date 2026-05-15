/****************************************
 * @file billboard.h
 * @brief ビルボード（常にカメラ正面を向く平面）描画モジュール
 * @author Natsume Shidara
 * @date 2025/06/12
 *
 * テクスチャ付きビルボードの単体描画およびバッチ描画APIを提供する。
 * パーティクルやUI表示等に使用。
 ****************************************/

#ifndef BILLBOARD_H
#define BILLBOARD_H

#include <DirectXMath.h>
#include <d3d11.h>
#include "collision.h"

/** @brief 初期化（頂点バッファ生成） */
void Billboard_Initialize();

/** @brief 終了処理（リソース解放） */
void Billboard_Finalize(void);

/** @brief 更新処理 */
void Billboard_Update(double elapsed_time);

/** @brief ビルボード描画（全面テクスチャ） */
void Billboard_Draw(int texId, const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT2& scale, const DirectX::XMFLOAT2& pivot = { 0.0f, 0.0f }, const DirectX::XMFLOAT4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

/** @brief ビルボード描画（テクスチャ切り抜き対応） */
void Billboard_Draw(int texId, const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT2& scale, const DirectX::XMFLOAT4& tex_cut, const DirectX::XMFLOAT2& pivot = { 0.0f, 0.0f }, const DirectX::XMFLOAT4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

// バッチ描画API（大量ビルボード描画時の高速化）
// Begin: シェーダー/VB/トポロジー/ビルボード行列をキャッシュ
// DrawBatch: ワールド行列・テクスチャ・カラーのみ更新して描画
// End: 何もしない（将来の拡張用）
void Billboard_BeginBatch();
void Billboard_DrawBatch(int texId, const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT2& scale, const DirectX::XMFLOAT2& pivot = { 0.0f, 0.0f }, const DirectX::XMFLOAT4& color = { 1.0f, 1.0f, 1.0f, 1.0f });
void Billboard_DrawBatch(int texId, const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT2& scale, const DirectX::XMFLOAT4& tex_cut, const DirectX::XMFLOAT2& pivot = { 0.0f, 0.0f }, const DirectX::XMFLOAT4& color = { 1.0f, 1.0f, 1.0f, 1.0f });
void Billboard_EndBatch();

/** @brief ビュー行列の設定（ビルボード回転計算に使用） */
void Billboard_SetViewMatrix(const DirectX::XMFLOAT4X4& view);

#endif // BILLBOARD_H
