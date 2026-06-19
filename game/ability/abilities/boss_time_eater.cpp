/****************************************
 * @file   boss_time_eater.cpp
 * @brief  時喰らい (ボス側アビリティ) の実装
 * @author Natsume Shidara
 * @date   2026/06/19
 ****************************************/
#include "boss_time_eater.h"
#include "ability/ability_registry.h"
#include "config.h"

//======================================
// 構築
//======================================
BossTimeEaterAbility::BossTimeEaterAbility()
{
    name        = "時喰らい";
    description = "あなたの思考時間を\n削り取る";
    rarity      = Rarity::Rare;
    factor      = Config::GetDouble("bosses.timeEater.factor",   0.5);
    minFloor    = Config::GetDouble("bosses.timeEater.minFloor", 30.0);
}

//======================================
// ITurnTimeProvider
//======================================
double BossTimeEaterAbility::GetTurnTimeSeconds(Piece player)
{
    // まずチェイン先(熟考/時間停止/デフォルト等)の値を取得する
    const double base = chained ? chained->GetTurnTimeSeconds(player) : 180.0;
    if (player != Piece::Player) return base;   // 敵側は削らない

    // プレイヤー時間を factor 倍に削る (ただし minFloor は下回らない)
    const double cut = base * factor;
    return (cut < minFloor) ? ((base < minFloor) ? base : minFloor) : cut;
}

//======================================
// Ability
//======================================
void BossTimeEaterAbility::RegisterTo(AbilityRegistry& registry)
{
    auto self = std::dynamic_pointer_cast<ITurnTimeProvider>(shared_from_this());
    chained = registry.SetTurnTimeProvider(self);  // 旧実装をチェインに保存
}
