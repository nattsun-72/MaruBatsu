/****************************************
 * @file   time_stop.h
 * @brief  時間停止 — 思考時間が実質無制限になる (レジェンダリー)
 * @author Natsume Shidara
 * @date   2026/06/19
 *
 * ITurnTimeProvider をチェインし、プレイヤーのターン時間を
 * 巨大値(実質無制限)に上書きする。時間半減系デメリットの打ち消し。
 ****************************************/
#ifndef ABILITY_TIME_STOP_H
#define ABILITY_TIME_STOP_H

#include "ability/ability.h"
#include "ability/hooks.h"

#include <memory>

//======================================
// 時間停止アビリティ
//======================================
/**
 * @class  TimeStopAbility
 * @brief  プレイヤーの思考時間を実質無制限にするアビリティ
 */
class TimeStopAbility : public Ability, public ITurnTimeProvider
{
public:
    //--------------------------------------
    // メンバ変数
    //--------------------------------------
    double seconds    = 9999.0;          // 実質無制限とみなす秒数
    Piece  targetSide = Piece::Player;   // 効果を適用する陣営

    // 旧 ITurnTimeProvider をチェインで保持し、対象外は委譲する
    std::shared_ptr<ITurnTimeProvider> chained;

    TimeStopAbility();

    /** @brief 指定陣営のターン時間 (targetSide は無制限値、他は委譲) */
    double GetTurnTimeSeconds(Piece player) override;
    /** @brief 思考時間プロバイダとして registry に登録 */
    void   RegisterTo(AbilityRegistry& registry) override;
};

#endif // ABILITY_TIME_STOP_H
