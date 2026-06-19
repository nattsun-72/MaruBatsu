/****************************************
 * @file   impatience.cpp
 * @brief  焦燥の実装
 * @author Natsume Shidara
 * @date   2026/06/19
 ****************************************/
#include "impatience.h"
#include "ability/ability_registry.h"
#include "config.h"

#include <cstdio>

//======================================
// 構築
//======================================
ImpatienceAbility::ImpatienceAbility()
{
    name           = "焦燥";
    rarity         = Rarity::Curse;
    unique         = true;   // 着手数は固定・時間倍率の多重掛けは破綻するため一度限り
    placementCount = Config::GetInt   ("abilities.impatience.placementCount", 2);
    timeFactor     = Config::GetDouble("abilities.impatience.timeFactor",     0.5);

    char buf[96];
    std::snprintf(buf, sizeof(buf),
                  "【呪い】思考時間が半分\nになるが%d手 打てる",
                  placementCount);
    description = buf;
}

//======================================
// ITurnHandler
//======================================
int ImpatienceAbility::GetPlacementCount(Piece player)
{
    // 対象陣営は複数着手、それ以外はチェイン先へ委譲
    if (player == targetSide) return placementCount;
    return chainedTurn ? chainedTurn->GetPlacementCount(player) : 1;
}

void ImpatienceAbility::OnTurnStart(GameState& state, Piece player)
{
    if (chainedTurn) chainedTurn->OnTurnStart(state, player);
}

void ImpatienceAbility::OnTurnEnd(GameState& state, Piece player)
{
    if (chainedTurn) chainedTurn->OnTurnEnd(state, player);
}

//======================================
// ITurnTimeProvider
//======================================
double ImpatienceAbility::GetTurnTimeSeconds(Piece player)
{
    // チェイン先(熟考等)の結果を基準に、対象陣営のみ時間を倍率で削る
    const double base = chainedTime ? chainedTime->GetTurnTimeSeconds(player) : 180.0;
    if (player == targetSide) return base * timeFactor;
    return base;
}

//======================================
// Ability
//======================================
void ImpatienceAbility::RegisterTo(AbilityRegistry& registry)
{
    auto self = shared_from_this();
    // 着手数フックと時間プロバイダの双方に割り込み、旧実装をチェイン保持する
    chainedTurn = registry.SetTurnHandler(
        std::dynamic_pointer_cast<ITurnHandler>(self));
    chainedTime = registry.SetTurnTimeProvider(
        std::dynamic_pointer_cast<ITurnTimeProvider>(self));
}
