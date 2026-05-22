/****************************************
 * @file   contemplation.h
 * @brief  熟考 — 思考時間 +30秒 (恒久) のアビリティ
 * @author Natsume Shidara
 * @date   2026/05/15
 *
 * ITurnTimeProvider をチェインし、プレイヤーのターン時間に
 * bonusSeconds を恒久的に加算する。
 ****************************************/
#ifndef ABILITY_CONTEMPLATION_H
#define ABILITY_CONTEMPLATION_H

#include "ability/ability.h"
#include "ability/hooks.h"

#include <memory>

//======================================
// 熟考アビリティ
//======================================
/**
 * @class  ContemplationAbility
 * @brief  プレイヤーのターン時間を恒久加算するアビリティ
 * @detail 余裕(MarginAbility)など、加算量違いの派生クラスの基底も兼ねる。
 */
class ContemplationAbility : public Ability, public ITurnTimeProvider
{
public:
    //--------------------------------------
    // メンバ変数
    //--------------------------------------
    double bonusSeconds = 30.0;            // 加算する秒数
    double baseSeconds  = 180.0;           // チェイン先が無い場合の基準秒数
    Piece  targetSide   = Piece::Player;   // 効果を適用する陣営

    // 旧 ITurnTimeProvider をチェインで保持し、加算前の値を委譲取得する
    std::shared_ptr<ITurnTimeProvider> chained;

    ContemplationAbility();

    /** @brief 指定陣営のターン時間を返す (targetSide には bonus を加算) */
    double GetTurnTimeSeconds(Piece player) override;
    /** @brief 思考時間プロバイダとして registry に登録 */
    void   RegisterTo(AbilityRegistry& registry) override;
};

#endif // ABILITY_CONTEMPLATION_H
