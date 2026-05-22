/****************************************
 * @file ice_board_boss.h
 * @brief ボス1: 氷盤の支配者
 * @author Natsume Shidara
 * @date 2026/05/15
 *
 * 設置時/設置後フックを使う最初のボス。難易度★1。
 ****************************************/
#ifndef ICE_BOARD_BOSS_H
#define ICE_BOARD_BOSS_H

#include "boss/boss.h"

class IceBoardBoss : public Boss
{
public:
    IceBoardBoss();
    std::vector<std::shared_ptr<Ability>> GetBossAbilities() override;
    std::shared_ptr<Ability> GetRewardAbility() override;
};

#endif // ICE_BOARD_BOSS_H
