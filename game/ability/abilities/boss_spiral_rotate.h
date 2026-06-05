/****************************************
 * @file   boss_spiral_rotate.h
 * @brief  螺旋盤 — ボス2「螺旋の女王」のボス側アビリティ
 * @author Natsume Shidara
 * @date   2026/06/05
 *
 * 仕様: 駒が置かれるたび、盤面全体が90度回転する。
 *       氷盤(全駒スライド)と同じく IPlacementHandler で実現し、
 *       回転による駒移動を効果フェーズとしてアニメ再生する。
 ****************************************/
#ifndef ABILITY_BOSS_SPIRAL_ROTATE_H
#define ABILITY_BOSS_SPIRAL_ROTATE_H

//--------------------------------------
// インクルード
//--------------------------------------
#include "ability/ability.h"
#include "ability/hooks.h"

//======================================
// 螺旋盤アビリティ (ボス側)
//======================================
/**
 * @class  BossSpiralRotateAbility
 * @brief  設置のたび盤面を90度回転させるボス側アビリティ
 * @detail IPlacementHandler のみを実装する。氷盤と異なりターン処理は
 *         持たず、回転方向は固定(時計回り)で学習可能にする。
 */
class BossSpiralRotateAbility
    : public Ability
    , public IPlacementHandler
{
public:
    //--------------------------------------
    // メンバ変数
    //--------------------------------------
    bool clockwise = true;   // 回転方向 (true=時計回り)

    //======================================
    // 構築
    //======================================
    BossSpiralRotateAbility();

    //======================================
    // IPlacementHandler
    //======================================
    /** @brief 設置時、盤面全体を90度回転させる */
    void OnPlace(GameState& state, Vec2 pos, Piece placedBy) override;

    //======================================
    // Ability
    //======================================
    /** @brief 設置ハンドラとして registry に登録 */
    void RegisterTo(AbilityRegistry& registry) override;
};

#endif // ABILITY_BOSS_SPIRAL_ROTATE_H
