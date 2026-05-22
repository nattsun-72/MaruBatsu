/****************************************
 * @file   focus.h
 * @brief  集中 — 任意発動アビリティ (このターン思考時間 +2分)
 * @author Natsume Shidara
 * @date   2026/05/22
 *
 * 企画書コモン「集中」。クリックで発動する初の任意発動アビリティ。
 * パッシブなフックは持たず、クリック時の Activate でのみ作用する。
 ****************************************/
#ifndef ABILITY_FOCUS_H
#define ABILITY_FOCUS_H

#include "ability/ability.h"

//======================================
// 集中アビリティ
//======================================
/**
 * @class  FocusAbility
 * @brief  クリック発動でこのターンの思考時間を加算するアビリティ
 * @detail 任意発動(activatable)型。チャージを消費して発動する。
 */
class FocusAbility : public Ability
{
public:
    //--------------------------------------
    // メンバ変数
    //--------------------------------------
    int    charges      = 3;        // 残りチャージ数
    double bonusSeconds = 120.0;    // 1回あたりの加算秒数

    FocusAbility();

    //======================================
    // 任意発動アビリティのインターフェイス
    //======================================
    /** @brief 発動: チャージを1消費し、残り時間を加算する */
    void Activate(GameState& state) override;
    /** @brief 発動可能か (チャージが残っているか) */
    bool CanActivate(const GameState& state) const override;
    /** @brief 残りチャージ数を返す */
    int  ChargesLeft() const override { return charges; }

    //======================================
    // Ability
    //======================================
    /** @brief パッシブフックは持たないため、登録処理は空 */
    void RegisterTo(AbilityRegistry& /*registry*/) override {}
};

#endif // ABILITY_FOCUS_H
