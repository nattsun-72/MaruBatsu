/****************************************
 * @file blood_effect.h
 * @brief 敵斬撃時の血しぶきパーティクルエフェクト
 * @author Natsume Shidara
 * @date 2026/02/18
 ****************************************/
#ifndef BLOOD_EFFECT_H
#define BLOOD_EFFECT_H

#include <DirectXMath.h>

void BloodEffect_Initialize();
void BloodEffect_Finalize();
void BloodEffect_Update(double elapsed_time);
void BloodEffect_Draw();

void BloodEffect_Create(const DirectX::XMFLOAT3& position);

#endif // BLOOD_EFFECT_H
