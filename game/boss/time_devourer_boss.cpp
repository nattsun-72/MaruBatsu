/****************************************
 * @file   time_devourer_boss.cpp
 * @brief  ボス: 時喰らい 実装
 * @author Natsume Shidara
 * @date   2026/06/19
 ****************************************/
#include "time_devourer_boss.h"
#include "ability/abilities/boss_time_eater.h"
#include "ability/abilities/sands_of_time.h"

//======================================
// 構築
//======================================
TimeDevourerBoss::TimeDevourerBoss()
{
    name        = "時喰らい";
    description = "あなたの思考時間を削り取る";
    difficulty  = 3;
    // テーマ色: 沈んだ褐色 (砂時計)
    bgR = 0.12f; bgG = 0.08f; bgB = 0.04f;
}

//======================================
// Boss インターフェース実装
//======================================
std::vector<std::shared_ptr<Ability>> TimeDevourerBoss::GetBossAbilities()
{
    // ボス側ギミック: プレイヤーの思考時間を削る
    return { std::make_shared<BossTimeEaterAbility>() };
}

std::shared_ptr<Ability> TimeDevourerBoss::GetRewardAbility()
{
    // 撃破報酬: 思考時間を全回復できる「時の砂」
    return std::make_shared<SandsOfTimeAbility>();
}
