/****************************************
 * @file   margin.cpp
 * @brief  余裕の実装 (熟考の強化版)
 * @author Natsume Shidara
 * @date   2026/05/15
 ****************************************/
#include "margin.h"
#include "config.h"

#include <cstdio>

//======================================
// 構築
//======================================
MarginAbility::MarginAbility()
{
    // 基底(熟考)から、名前・説明・加算量・レアリティを上書きする
    name         = "余裕";
    rarity       = Rarity::Rare;   // 熟考の上位互換のためレアに引き上げ
    bonusSeconds = Config::GetDouble("abilities.margin.bonusSeconds", 60.0);

    char buf[64];
    std::snprintf(buf, sizeof(buf), "毎ターン思考時間\n+%d秒 (恒久)",
                  static_cast<int>(bonusSeconds));
    description = buf;
}
