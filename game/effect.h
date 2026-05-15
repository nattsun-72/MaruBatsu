/****************************************
 * @file effect.h
 * @brief 2Dエフェクト管理（爆発アニメーション等）
 * @author Natsume Shidara
 * @date 2025/07/04
 ****************************************/

#ifndef EFFECT_H
#define EFFECT_H

#include "DirectXMath.h"

/** @brief エフェクトシステム初期化 */
void Effect_Initialize();

/** @brief エフェクトシステム終了処理 */
void Effect_Finalize();

/** @brief エフェクト更新（アニメーション進行） */
void Effect_Update(double elapsed_time);

/** @brief エフェクト描画 */
void Effect_Draw();

/** @brief エフェクト生成 */
void Effect_Create(const DirectX::XMFLOAT2& position);

#endif
