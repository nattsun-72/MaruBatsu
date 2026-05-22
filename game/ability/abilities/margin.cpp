/****************************************
 * @file   margin.cpp
 * @brief  余裕の実装 (熟考の強化版)
 * @author Natsume Shidara
 * @date   2026/05/15
 ****************************************/
#include "margin.h"

//======================================
// 構築
//======================================
MarginAbility::MarginAbility()
{
    // 基底(熟考)から、名前・説明・加算量を上書きする
    name         = "余裕";
    description  = "毎ターン思考時間\n+60秒 (恒久)";
    rarity       = Rarity::Common;
    bonusSeconds = 60.0;
}
