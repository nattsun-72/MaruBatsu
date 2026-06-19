/****************************************
 * @file   mirror_warden_boss.h
 * @brief  ボス: 鏡写しの番人
 * @author Natsume Shidara
 * @date   2026/06/19
 *
 * プレイヤーの設置を点対称にミラー設置するギミックを持つボス。難易度★2。
 ****************************************/
#ifndef MIRROR_WARDEN_BOSS_H
#define MIRROR_WARDEN_BOSS_H

#include "boss/boss.h"

//======================================
// 鏡写しの番人
//======================================
/**
 * @class  MirrorWardenBoss
 * @brief  ミラー設置ギミックを持つボス
 */
class MirrorWardenBoss : public Boss
{
public:
    MirrorWardenBoss();
    std::vector<std::shared_ptr<Ability>> GetBossAbilities() override;
    std::shared_ptr<Ability> GetRewardAbility() override;
};

#endif // MIRROR_WARDEN_BOSS_H
