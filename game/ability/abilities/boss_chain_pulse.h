/****************************************
 * @file   boss_chain_pulse.h
 * @brief  連鎖鼓動 — ボス3「連鎖の鼓動」のボス側アビリティ
 * @author Natsume Shidara
 * @date   2026/06/05
 *
 * 仕様: 対象陣営(既定=ボス)が駒を置くと、隣接する空きマスへ同色の駒が
 *       自動で「拡張(出現)」する。pulseSteps 回まで、出現した駒を起点に
 *       連鎖的に広がる。盤面を侵食する圧をかける★3ボスのギミック。
 ****************************************/
#ifndef ABILITY_BOSS_CHAIN_PULSE_H
#define ABILITY_BOSS_CHAIN_PULSE_H

//--------------------------------------
// インクルード
//--------------------------------------
#include "ability/ability.h"
#include "ability/hooks.h"
#include "piece.h"

//======================================
// 連鎖鼓動アビリティ (ボス側)
//======================================
/**
 * @class  BossChainPulseAbility
 * @brief  設置のたび隣接マスへ同色駒を自動拡張させるボス側アビリティ
 * @detail IPlacementHandler を実装。対象陣営の設置時のみ発動し、
 *         出現を効果フェーズ(isSpawn)としてアニメ再生する。
 */
class BossChainPulseAbility
    : public Ability
    , public IPlacementHandler
{
public:
    //--------------------------------------
    // メンバ変数
    //--------------------------------------
    Piece targetSide = Piece::Enemy;  // 拡張する陣営 (報酬版は Player)
    int   pulseSteps = 1;             // 1設置あたりの連鎖(拡張)回数

    //======================================
    // 構築
    //======================================
    BossChainPulseAbility();

    //======================================
    // IPlacementHandler
    //======================================
    /** @brief 設置時、対象陣営なら隣接空きマスへ同色駒を自動拡張する */
    void OnPlace(GameState& state, Vec2 pos, Piece placedBy) override;

    //======================================
    // Ability
    //======================================
    /** @brief 設置ハンドラとして registry に登録 */
    void RegisterTo(AbilityRegistry& registry) override;
};

#endif // ABILITY_BOSS_CHAIN_PULSE_H
