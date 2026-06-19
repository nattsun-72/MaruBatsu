/****************************************
 * @file   impatience.h
 * @brief  焦燥 — 【呪い】思考時間が半分になる代わりに 1ターン2手 打てる
 * @author Natsume Shidara
 * @date   2026/06/19
 *
 * ハイリスク・ハイリターンの呪いアビリティ。
 *   デメリット: 自分のターン制限時間が常に半減する (ITurnTimeProvider)
 *   メリット  : 1ターンに2回 連続で配置できる (ITurnHandler)
 * 二手打ちと同じ着手数フックに加え、時間プロバイダもチェインする。
 ****************************************/
#ifndef ABILITY_IMPATIENCE_H
#define ABILITY_IMPATIENCE_H

#include "ability/ability.h"
#include "ability/hooks.h"

#include <memory>

//======================================
// 焦燥アビリティ
//======================================
/**
 * @class  ImpatienceAbility
 * @brief  時間半減と引き換えに2手打ちを得る呪いアビリティ
 * @detail ITurnHandler(着手数) と ITurnTimeProvider(時間) の両方を実装し、
 *         RegisterTo で双方のチェインに割り込む。
 */
class ImpatienceAbility : public Ability,
                          public ITurnHandler,
                          public ITurnTimeProvider
{
public:
    //--------------------------------------
    // メンバ変数
    //--------------------------------------
    Piece  targetSide      = Piece::Player;   // 効果を適用する陣営
    int    placementCount  = 2;               // 付与する着手数
    double timeFactor      = 0.5;             // 時間の倍率 (0.5 = 半減)

    // 旧フックをチェインで保持し、加工前の値を委譲取得する
    std::shared_ptr<ITurnHandler>      chainedTurn;
    std::shared_ptr<ITurnTimeProvider> chainedTime;

    ImpatienceAbility();

    //======================================
    // ITurnHandler
    //======================================
    int  GetPlacementCount(Piece player) override;
    void OnTurnStart(GameState& state, Piece player) override;
    void OnTurnEnd  (GameState& state, Piece player) override;

    //======================================
    // ITurnTimeProvider
    //======================================
    double GetTurnTimeSeconds(Piece player) override;

    //======================================
    // Ability
    //======================================
    void RegisterTo(AbilityRegistry& registry) override;
};

#endif // ABILITY_IMPATIENCE_H
