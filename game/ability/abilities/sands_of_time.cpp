/****************************************
 * @file   sands_of_time.cpp
 * @brief  時の砂の実装
 * @author Natsume Shidara
 * @date   2026/06/19
 ****************************************/
#include "sands_of_time.h"
#include "game_state.h"
#include "config.h"

//======================================
// 構築
//======================================
SandsOfTimeAbility::SandsOfTimeAbility()
{
    name          = "時の砂";
    description   = "クリックで\n思考時間を全回復";
    rarity        = Rarity::Rare;
    activatable   = true;
    flashText     = "時間回復！";
    charges       = Config::GetInt("abilities.sandsOfTime.charges", 2);
    refillSeconds = Config::GetDouble("abilities.sandsOfTime.refillSeconds", 180.0);
}

//======================================
// 任意発動アビリティのインターフェイス
//======================================
void SandsOfTimeAbility::Activate(GameState& state)
{
    if (charges <= 0) return;
    --charges;
    // 現在より少ない場合のみ回復 (増やしすぎを防ぐ)
    if (state.remainingTime < refillSeconds) state.remainingTime = refillSeconds;
}

bool SandsOfTimeAbility::CanActivate(const GameState& state) const
{
    return charges > 0 && state.remainingTime < refillSeconds;
}
