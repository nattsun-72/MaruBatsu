/****************************************
 * @file   time_stop.cpp
 * @brief  時間停止の実装
 * @author Natsume Shidara
 * @date   2026/06/19
 ****************************************/
#include "time_stop.h"
#include "ability/ability_registry.h"
#include "config.h"

//======================================
// 構築
//======================================
TimeStopAbility::TimeStopAbility()
{
    name        = "時間停止";
    description = "思考時間が\n実質無制限になる";
    rarity      = Rarity::Legendary;
    unique      = true;   // 無制限化は1つで完結するため一度限り
    seconds     = Config::GetDouble("abilities.timeStop.seconds", 9999.0);
}

//======================================
// ITurnTimeProvider
//======================================
double TimeStopAbility::GetTurnTimeSeconds(Piece player)
{
    if (player == targetSide) return seconds;   // 対象陣営は無制限値で上書き
    return chained ? chained->GetTurnTimeSeconds(player) : 180.0;
}

//======================================
// Ability
//======================================
void TimeStopAbility::RegisterTo(AbilityRegistry& registry)
{
    auto self = std::dynamic_pointer_cast<ITurnTimeProvider>(shared_from_this());
    chained = registry.SetTurnTimeProvider(self);  // 旧実装をチェインに保存
}
