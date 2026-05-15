/****************************************
 * @file damage_effect.h
 * @brief 被弾エフェクト（画面フラッシュ・瀕死時赤演出）
 * @author Natsume Shidara
 * @date 2026/02/19
 ****************************************/

#ifndef DAMAGE_EFFECT_H
#define DAMAGE_EFFECT_H

// 初期化・終了
void DamageEffect_Initialize();
void DamageEffect_Finalize();

// 更新・描画
void DamageEffect_Update(float dt);
void DamageEffect_Draw();

// トリガー

/**
 * @brief 被弾フラッシュを発生させる
 */
void DamageEffect_TriggerHitFlash();

#endif // DAMAGE_EFFECT_H
