/****************************************
 * @file camera.h
 * @brief カメラ制御モジュールのヘッダ
 * @author Natsume Shidara
 * @date 2025/06/06
 *
 * FPSカメラの移動・回転操作、ビュー行列・投影行列の
 * 生成・取得インターフェースを提供する。
 ****************************************/

#ifndef CAMERA_H
#define CAMERA_H

#include <DirectXMath.h>

/** @brief カメラ初期化（位置・前方・上方を指定） */
void Camera_Initialize(
    const DirectX::XMFLOAT3& position,
    const DirectX::XMFLOAT3& front,
    const DirectX::XMFLOAT3& up
    );

/** @brief カメラ基本初期化（既定値） */
void Camera_Initialize();
/** @brief カメラ終了処理 */
void Camera_Finalize(void);
/** @brief カメラ更新処理（移動・回転・行列再計算） */
void Camera_Update(double elapsed_time);

/** @brief ビュー行列を取得 */
const DirectX::XMFLOAT4X4& Camera_GetViewMatrix();
/** @brief 投影行列を取得 */
const DirectX::XMFLOAT4X4& Camera_GetPerspectiveMatrix();

/** @brief 前方向ベクトルを取得 */
const DirectX::XMFLOAT3& Camera_GetFront();
/** @brief 右方向ベクトルを取得 */
const DirectX::XMFLOAT3& Camera_GetRight();
/** @brief 上方向ベクトルを取得 */
const DirectX::XMFLOAT3& Camera_GetUp();
/** @brief 視野角（ラジアン）を取得 */
const float& Camera_GetFov();

/** @brief カメラ位置を取得 */
const DirectX::XMFLOAT3& Camera_GetPosition();

/** @brief カメラ情報デバッグ描画 */
void Camera_DebugDraw();

/** @brief ビュー行列・投影行列を定数バッファに設定 */
void Camera_SetMatrix(const DirectX::XMMATRIX& view, const DirectX::XMMATRIX& projection);

#endif // CAMERA_H
