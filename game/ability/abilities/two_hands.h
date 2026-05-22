/****************************************
 * @file   two_hands.h
 * @brief  二手打ち — 1ターンに2個連続で配置
 * @author Natsume Shidara
 * @date   2026/05/15
 *
 * ITurnHandler をチェインし、対象陣営の1ターン着手数を2に増やす。
 ****************************************/
#ifndef ABILITY_TWO_HANDS_H
#define ABILITY_TWO_HANDS_H

#include "ability/ability.h"
#include "ability/hooks.h"

#include <memory>

//======================================
// 二手打ちアビリティ
//======================================
/**
 * @class  TwoHandsAbility
 * @brief  1ターンの着手数を2に増やすアビリティ
 */
class TwoHandsAbility : public Ability, public ITurnHandler
{
public:
    //--------------------------------------
    // メンバ変数
    //--------------------------------------
    Piece targetSide          = Piece::Player;  // 効果を適用する陣営
    int   doublePlacementCount = 2;             // 適用時の着手数

    // 旧 ITurnHandler をチェインで保持し、対象外陣営は委譲する
    std::shared_ptr<ITurnHandler> chained;

    TwoHandsAbility();

    //======================================
    // ITurnHandler
    //======================================
    /** @brief 1ターンの着手数 (targetSide は2、他はチェイン先へ委譲) */
    int  GetPlacementCount(Piece player) override;
    /** @brief ターン開始時 (チェイン先へ委譲) */
    void OnTurnStart(GameState& state, Piece player) override;
    /** @brief ターン終了時 (チェイン先へ委譲) */
    void OnTurnEnd  (GameState& state, Piece player) override;

    //======================================
    // Ability
    //======================================
    /** @brief ターン処理として registry に登録 */
    void RegisterTo(AbilityRegistry& registry) override;
};

#endif // ABILITY_TWO_HANDS_H
