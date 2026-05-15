/****************************************
 * @file light_camera.h
 * @brief シャドウマップ生成用ライトカメラ管理
 * @author Natsume Shidara
 * @date 2025/12/10
 *
 * 平行光源の視点からの正射影カメラを管理し、
 * シャドウマップ描画に必要なビュー・プロジェクション行列を提供する。
 ****************************************/

#ifndef LIGHT_CAMERA_H
#define LIGHT_CAMERA_H

#include <DirectXMath.h>

/** @brief ライトカメラの初期化 */
void LightCamera_Initialize(const DirectX::XMFLOAT3& world_directional, const DirectX::XMFLOAT3& world_Position);

/** @brief 終了処理 */
void LightCamera_Finalize();

/** @brief ライトカメラの位置を設定 */
void LightCamera_SetPosition(const DirectX::XMFLOAT3& position);

/** @brief ライトカメラの正面方向を設定 */
void LightCamera_SetFront(const DirectX::XMFLOAT3& front);

/** @brief 正射影の範囲を設定（幅・高さ・近/遠クリップ面） */
void LightCamera_SetRange(float w, float h, float n, float f);

/** @brief ビュー・プロジェクション行列を再計算 */
void LightCamera_Update();

/** @brief ビュー行列を取得 */
const DirectX::XMFLOAT4X4& LightCamera_GetViewMatrix();

/** @brief プロジェクション行列を取得 */
const DirectX::XMFLOAT4X4& LightCamera_GetProjectionMatrix();

#endif // LIGHT_CAMERA_H