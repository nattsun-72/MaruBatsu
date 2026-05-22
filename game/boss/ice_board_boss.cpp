/****************************************
 * @file ice_board_boss.cpp
 * @brief ボス1: 氷盤の支配者 実装
 * @author Natsume Shidara
 * @date 2026/05/15
 ****************************************/
#include "ice_board_boss.h"
#include "ability/abilities/boss_ice_slide.h"
#include "ability/abilities/ice_tile.h"

IceBoardBoss::IceBoardBoss()
{
    name        = "氷盤の支配者";
    description = "駒を置くたび全ての駒が滑る";
    difficulty  = 1;
}

std::vector<std::shared_ptr<Ability>> IceBoardBoss::GetBossAbilities()
{
    return { std::make_shared<BossIceSlideAbility>() };
}

std::shared_ptr<Ability> IceBoardBoss::GetRewardAbility()
{
    return std::make_shared<IceTileAbility>();
}
