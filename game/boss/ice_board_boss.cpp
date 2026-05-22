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
    name        = "Ice Tyrant";
    description = "Each placement slides every piece to the edge.";
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
