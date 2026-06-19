/****************************************
 * @file   time_devourer_boss.h
 * @brief  ボス: 時喰らい
 * @author Natsume Shidara
 * @date   2026/06/19
 *
 * プレイヤーの思考時間を削るギミックを持つボス。難易度★3。
 ****************************************/
#ifndef TIME_DEVOURER_BOSS_H
#define TIME_DEVOURER_BOSS_H

#include "boss/boss.h"

//======================================
// 時喰らい
//======================================
/**
 * @class  TimeDevourerBoss
 * @brief  プレイヤーの思考時間を削るボス
 */
class TimeDevourerBoss : public Boss
{
public:
    TimeDevourerBoss();
    std::vector<std::shared_ptr<Ability>> GetBossAbilities() override;
    std::shared_ptr<Ability> GetRewardAbility() override;
};

#endif // TIME_DEVOURER_BOSS_H
