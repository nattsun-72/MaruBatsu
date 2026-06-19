/****************************************
 * @file   boss_mirror.h
 * @brief  鏡写し — ボス「鏡写しの番人」のボス側アビリティ
 * @author Natsume Shidara
 * @date   2026/06/19
 *
 * 仕様: プレイヤーが駒を置くと、盤面中心に対して点対称のマスへ
 *       ボスの駒が即座に出現する(空マスのときのみ)。
 *       「自分の手が相手を利する」緊張を生むギミック。
 ****************************************/
#ifndef ABILITY_BOSS_MIRROR_H
#define ABILITY_BOSS_MIRROR_H

#include "ability/ability.h"
#include "ability/hooks.h"

//======================================
// 鏡写しアビリティ (ボス側)
//======================================
/**
 * @class  BossMirrorAbility
 * @brief  プレイヤー設置を点対称にミラー設置するボス側アビリティ
 */
class BossMirrorAbility : public Ability, public IPlacementHandler
{
public:
    BossMirrorAbility();

    //======================================
    // IPlacementHandler
    //======================================
    /** @brief プレイヤー設置時、点対称マスへボス駒を出現させる */
    void OnPlace(GameState& state, Vec2 pos, Piece placedBy) override;

    //======================================
    // Ability
    //======================================
    /** @brief 設置ハンドラとして registry に登録 */
    void RegisterTo(AbilityRegistry& registry) override;
};

#endif // ABILITY_BOSS_MIRROR_H
