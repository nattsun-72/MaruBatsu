/****************************************
 * @file   ice_tile.h
 * @brief  氷駒 — ボス1撃破報酬のプレイヤー側アビリティ
 * @author Natsume Shidara
 * @date   2026/05/15
 *
 * 仕様: プレイヤーが置いた駒のみ、設置時に1マス分滑る。
 * 本アビリティを重ね掛け(複数所持)すると、所持数だけ滑走距離が
 * +1マスずつ伸びる。
 * プロト段階では方向選択UIは無く、Direction::Down に固定する。
 * ボス1「氷盤の支配者」の弱化版報酬にあたる。
 ****************************************/
#ifndef ABILITY_ICE_TILE_H
#define ABILITY_ICE_TILE_H

#include "ability/ability.h"
#include "ability/hooks.h"
#include "direction.h"

//======================================
// 氷駒アビリティ
//======================================
/**
 * @class  IceTileAbility
 * @brief  設置した自分の駒を滑らせるアビリティ (重ね掛けで滑走距離+1)
 */
class IceTileAbility : public Ability, public IPlacementHandler
{
public:
    //--------------------------------------
    // メンバ変数
    //--------------------------------------
    Direction slideDir  = Direction::Down;   // 滑る方向
    Piece     ownerSide = Piece::Player;     // 効果対象の陣営

    IceTileAbility();

    /** @brief 設置時、自分の駒を1マス滑らせる (重ね掛け分だけ累積する) */
    void OnPlace(GameState& state, Vec2 pos, Piece placedBy) override;
    /** @brief 設置ハンドラとして registry に登録 */
    void RegisterTo(AbilityRegistry& registry) override;
};

#endif // ABILITY_ICE_TILE_H
