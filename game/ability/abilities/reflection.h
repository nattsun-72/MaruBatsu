/****************************************
 * @file   reflection.h
 * @brief  鏡映 — 置いた自駒を点対称マスにも出現させる (鏡ボス撃破報酬)
 * @author Natsume Shidara
 * @date   2026/06/19
 *
 * 鏡写しの番人のギミックの反転(味方版)。自駒を置くと、盤面中心に
 * 対して点対称のマスにも自駒が出現する(空マスのときのみ)。
 ****************************************/
#ifndef ABILITY_REFLECTION_H
#define ABILITY_REFLECTION_H

#include "ability/ability.h"
#include "ability/hooks.h"

//======================================
// 鏡映アビリティ
//======================================
/**
 * @class  ReflectionAbility
 * @brief  自駒設置時に点対称マスへ自駒を出現させるアビリティ
 */
class ReflectionAbility : public Ability, public IPlacementHandler
{
public:
    ReflectionAbility();

    //======================================
    // IPlacementHandler
    //======================================
    /** @brief 自駒設置時、点対称マスが空なら自駒を出現させる */
    void OnPlace(GameState& state, Vec2 pos, Piece placedBy) override;

    //======================================
    // Ability
    //======================================
    /** @brief 設置ハンドラとして registry に登録 */
    void RegisterTo(AbilityRegistry& registry) override;
};

#endif // ABILITY_REFLECTION_H
