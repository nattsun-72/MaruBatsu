/****************************************
 * @file   boss_ice_slide.h
 * @brief  氷盤 — ボス1「氷盤の支配者」のボス側アビリティ
 * @author Natsume Shidara
 * @date   2026/05/15
 *
 * 仕様: 駒が置かれると全ての駒が指定方向の端まで滑る。
 *       滑る方向はターン開始時にランダムで決まる。
 ****************************************/
#ifndef ABILITY_BOSS_ICE_SLIDE_H
#define ABILITY_BOSS_ICE_SLIDE_H

#include "ability/ability.h"
#include "ability/hooks.h"
#include "direction.h"

#include <memory>

//======================================
// 氷盤アビリティ (ボス側)
//======================================
/**
 * @class  BossIceSlideAbility
 * @brief  全駒スライドを起こすボス側アビリティ
 * @detail IPlacementHandler で設置時に全駒を滑らせ、
 *         ITurnHandler でターン開始時に滑る方向を抽選する。
 */
class BossIceSlideAbility
    : public Ability
    , public IPlacementHandler
    , public ITurnHandler
{
public:
    //--------------------------------------
    // メンバ変数
    //--------------------------------------
    Direction currentDir = Direction::Right;  // 現在の滑走方向

    // 旧 ITurnHandler (デフォルト等) をチェインで保持し、委譲する
    std::shared_ptr<ITurnHandler> chainedTurn;

    //======================================
    // 構築
    //======================================
    BossIceSlideAbility();

    //======================================
    // IPlacementHandler
    //======================================
    /** @brief 設置時、全駒を currentDir 方向へスライドさせる */
    void OnPlace(GameState& state, Vec2 pos, Piece placedBy) override;

    //======================================
    // ITurnHandler
    //======================================
    /** @brief 1ターンの着手数 (チェイン先へ委譲) */
    int  GetPlacementCount(Piece player) override;
    /** @brief ターン開始時、滑走方向をランダム抽選する */
    void OnTurnStart(GameState& state, Piece player) override;
    /** @brief ターン終了時 (チェイン先へ委譲) */
    void OnTurnEnd  (GameState& state, Piece player) override;

    //======================================
    // Ability
    //======================================
    /** @brief 設置ハンドラ・ターン処理として registry に登録 */
    void RegisterTo(AbilityRegistry& registry) override;
};

#endif // ABILITY_BOSS_ICE_SLIDE_H
