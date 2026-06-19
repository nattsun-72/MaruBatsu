/****************************************
 * @file   sands_of_time.h
 * @brief  時の砂 — 任意発動で思考時間を全回復する (ボス「時喰らい」報酬)
 * @author Natsume Shidara
 * @date   2026/06/19
 *
 * クリックで、現在ターンの残り思考時間を refillSeconds まで回復する。
 * チャージ制(ラン中の残回数)。時間削り系ボスへの対抗手段。
 ****************************************/
#ifndef ABILITY_SANDS_OF_TIME_H
#define ABILITY_SANDS_OF_TIME_H

#include "ability/ability.h"

//======================================
// 時の砂アビリティ
//======================================
/**
 * @class  SandsOfTimeAbility
 * @brief  クリック発動で思考時間を回復するアビリティ
 */
class SandsOfTimeAbility : public Ability
{
public:
    //--------------------------------------
    // メンバ変数
    //--------------------------------------
    int    charges       = 2;       // 残りチャージ数
    double refillSeconds = 180.0;   // 回復後の残り時間(秒)

    SandsOfTimeAbility();

    //======================================
    // 任意発動アビリティのインターフェイス
    //======================================
    /** @brief 発動: チャージを1消費し、残り時間を回復する */
    void Activate(GameState& state) override;
    /** @brief 発動可能か (チャージ残あり かつ 時間が回復値未満) */
    bool CanActivate(const GameState& state) const override;
    /** @brief 残りチャージ数を返す */
    int  ChargesLeft() const override { return charges; }

    //======================================
    // Ability
    //======================================
    /** @brief パッシブフックは持たないため、登録処理は空 */
    void RegisterTo(AbilityRegistry& /*registry*/) override {}
};

#endif // ABILITY_SANDS_OF_TIME_H
