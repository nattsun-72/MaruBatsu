/****************************************
 * @file   boss_gravity.h
 * @brief  重力場 — ボス4「重力の専制者」のボス側アビリティ
 * @author Natsume Shidara
 * @date   2026/06/11
 *
 * 仕様: 駒が置かれると、その駒は列の一番下まで落下する
 *       (四目並べのような重力ルール)。両陣営の駒が対象。
 *       重駒アビリティ所持時は自駒は落下しない。
 ****************************************/
#ifndef ABILITY_BOSS_GRAVITY_H
#define ABILITY_BOSS_GRAVITY_H

#include "ability/ability.h"
#include "ability/hooks.h"

//======================================
// 重力場アビリティ (ボス側)
//======================================
/**
 * @class  BossGravityAbility
 * @brief  設置された駒を列の底まで落下させるボス側アビリティ
 * @detail IPlacementHandler のみを実装。落下は効果フェーズとして
 *         記録され、アニメ再生される。
 */
class BossGravityAbility
    : public Ability
    , public IPlacementHandler
{
public:
    //======================================
    // 構築
    //======================================
    BossGravityAbility();

    //======================================
    // IPlacementHandler
    //======================================
    /** @brief 設置時、置かれた駒を列の底まで落下させる */
    void OnPlace(GameState& state, Vec2 pos, Piece placedBy) override;

    //======================================
    // Ability
    //======================================
    /** @brief 設置ハンドラとして registry に登録 */
    void RegisterTo(AbilityRegistry& registry) override;
};

#endif // ABILITY_BOSS_GRAVITY_H
