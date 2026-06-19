/****************************************
 * @file   rapid_thought.h
 * @brief  倍速思考 — 任意発動でこのターンの思考時間を実質無制限にする
 * @author Natsume Shidara
 * @date   2026/06/19
 *
 * クリックで、現在ターンの残り思考時間を一気に積み増す (実質無制限)。
 * チャージ制(既定1回)。「考える時間が足りない」局面を1度だけ救済する
 * コモン枠の保険アビリティ。
 ****************************************/
#ifndef ABILITY_RAPID_THOUGHT_H
#define ABILITY_RAPID_THOUGHT_H

#include "ability/ability.h"

//======================================
// 倍速思考アビリティ
//======================================
/**
 * @class  RapidThoughtAbility
 * @brief  クリック発動でこのターンの思考時間を実質無制限にするアビリティ
 */
class RapidThoughtAbility : public Ability
{
public:
    //--------------------------------------
    // メンバ変数
    //--------------------------------------
    int    charges     = 1;        // 残りチャージ数
    double grantSeconds = 999.0;   // 発動時に確保する残り時間(秒)

    RapidThoughtAbility();

    //======================================
    // 任意発動アビリティのインターフェイス
    //======================================
    /** @brief 発動: チャージを1消費し、残り時間を実質無制限まで引き上げる */
    void Activate(GameState& state) override;
    /** @brief 発動可能か (チャージ残あり かつ まだ無制限化していない) */
    bool CanActivate(const GameState& state) const override;
    /** @brief 残りチャージ数を返す */
    int  ChargesLeft() const override { return charges; }

    //======================================
    // Ability
    //======================================
    /** @brief パッシブフックは持たないため、登録処理は空 */
    void RegisterTo(AbilityRegistry& /*registry*/) override {}
};

#endif // ABILITY_RAPID_THOUGHT_H
