/****************************************
 * @file bullet.h
 * @brief 弾丸の生成・更新・描画管理
 * @author Natsume Shidara
 * @date 2025/11/12
 ****************************************/
#ifndef BULLET_H
#define BULLET_H

#include <DirectXmath.h>
#include "collision.h"

// 初期化・終了
void Bullet_Initialize();
void Bullet_Finalize();

// 更新・描画
void Bullet_Update(double elapsed_time);
void Bullet_Draw();

// 生成・破棄
void Bullet_Create(const DirectX::XMFLOAT3& position,const DirectX::XMFLOAT3& velocity);
void Bullet_Destory(int index);

// ゲッター
int Bullet_GetBulletCount();
const AABB Bullet_GetAABB(int index);
const DirectX::XMFLOAT3& Bullet_GetPosition(int index);

#endif
