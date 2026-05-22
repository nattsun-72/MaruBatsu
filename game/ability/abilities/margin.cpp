/****************************************
 * @file margin.cpp
 * @brief 余裕の実装 (Contemplationの強化版)
 * @author Natsume Shidara
 * @date 2026/05/15
 ****************************************/
#include "margin.h"

MarginAbility::MarginAbility()
{
    name          = "Margin";
    description   = "Player turn time +60s (permanent, strong common).";
    rarity        = Rarity::Common;
    bonusSeconds  = 60.0;
}
