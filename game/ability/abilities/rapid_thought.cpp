/****************************************
 * @file   rapid_thought.cpp
 * @brief  倍速思考の実装
 * @author Natsume Shidara
 * @date   2026/06/19
 ****************************************/
#include "rapid_thought.h"
#include "game_state.h"
#include "config.h"

//======================================
// 構築
//======================================
RapidThoughtAbility::RapidThoughtAbility()
{
    name         = "倍速思考";
    description  = "クリックでこの手番\n思考時間を無制限に";
    rarity       = Rarity::Common;
    activatable  = true;
    flashText    = "思考無制限！";
    charges      = Config::GetInt("abilities.rapidThought.charges", 1);
    grantSeconds = Config::GetDouble("abilities.rapidThought.grantSeconds", 999.0);
}

//======================================
// 任意発動アビリティのインターフェイス
//======================================
void RapidThoughtAbility::Activate(GameState& state)
{
    if (charges <= 0) return;
    --charges;
    // 現在より少ない場合のみ積み増す (このターンを実質無制限にする)
    if (state.remainingTime < grantSeconds) state.remainingTime = grantSeconds;
}

bool RapidThoughtAbility::CanActivate(const GameState& state) const
{
    return charges > 0 && state.remainingTime < grantSeconds;
}
