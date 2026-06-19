/****************************************
 * @file   boss_time_eater.h
 * @brief  時喰らい — ボス「時喰らい」のボス側アビリティ
 * @author Natsume Shidara
 * @date   2026/06/19
 *
 * 仕様: プレイヤーの思考時間を一定割合に削る(既定は半減)。
 *       盤面は動かさず、純粋な時間圧で攻めるギミック。
 *       「時の砂」「時間停止」「熟考」等で対抗できる。
 ****************************************/
#ifndef ABILITY_BOSS_TIME_EATER_H
#define ABILITY_BOSS_TIME_EATER_H

#include "ability/ability.h"
#include "ability/hooks.h"

#include <memory>

//======================================
// 時喰らいアビリティ (ボス側)
//======================================
/**
 * @class  BossTimeEaterAbility
 * @brief  プレイヤーの思考時間を割合で削るボス側アビリティ
 */
class BossTimeEaterAbility : public Ability, public ITurnTimeProvider
{
public:
    //--------------------------------------
    // メンバ変数
    //--------------------------------------
    double factor   = 0.5;     // プレイヤー思考時間にかける係数
    double minFloor = 30.0;    // 削っても下回らない秒数(理不尽防止)

    // 旧 ITurnTimeProvider をチェインで保持し、敵側は委譲する
    std::shared_ptr<ITurnTimeProvider> chained;

    BossTimeEaterAbility();

    /** @brief 思考時間 (プレイヤーは factor 倍、敵は委譲) */
    double GetTurnTimeSeconds(Piece player) override;
    /** @brief 思考時間プロバイダとして registry に登録 */
    void   RegisterTo(AbilityRegistry& registry) override;
};

#endif // ABILITY_BOSS_TIME_EATER_H
