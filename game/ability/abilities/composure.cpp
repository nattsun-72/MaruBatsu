/****************************************
 * @file   composure.cpp
 * @brief  平静の実装 (熟考の軽量版)
 * @author Natsume Shidara
 * @date   2026/06/19
 ****************************************/
#include "composure.h"
#include "config.h"

#include <cstdio>

//======================================
// 構築
//======================================
ComposureAbility::ComposureAbility()
{
    // 基底(熟考)から、名前・説明・加算量を上書きする
    name         = "平静";
    rarity       = Rarity::Common;
    bonusSeconds = Config::GetDouble("abilities.composure.bonusSeconds", 15.0);

    char buf[64];
    std::snprintf(buf, sizeof(buf), "毎ターン思考時間\n+%d秒 (恒久)",
                  static_cast<int>(bonusSeconds));
    description = buf;
}
