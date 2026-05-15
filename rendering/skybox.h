/****************************************
 * @file skybox.h
 * @brief スカイボックス描画モジュール
 * @author Natsume Shidara
 * @date 2025/11/21
 *
 * カメラ位置を追従するスカイボックスモデルの
 * 読み込み・描画を管理する。
 ****************************************/

#ifndef SKYBOX_H
#define SKYBOX_H

#include <DirectXMath.h>
#include <d3d11.h>

/** @brief スカイボックスモデルの読み込み */
void Skybox_Initialize();

/** @brief モデルリソースの解放 */
void Skybox_Finalize(void);

/** @brief スカイボックスの表示位置を設定（カメラ追従用） */
void Skybox_SetPosition(const DirectX::XMFLOAT3& position);

/** @brief スカイボックスの描画 */
void Skybox_Draw();

#endif // SKYBOX_H
