/****************************************
 * @file   engraved_one_boss.cpp
 * @brief  ボス5(ラスボス): 刻まれし者 実装
 * @author Natsume Shidara
 * @date   2026/06/05
 ****************************************/
#include "engraved_one_boss.h"
#include "ability/abilities/boss_engraved.h"
#include "ability/abilities/contemplation.h"

//======================================
// 構築
//======================================
EngravedOneBoss::EngravedOneBoss()
{
    name        = "刻まれし者";
    description = "過去の支配者たちの力を3つ同時に振るう";
    difficulty  = 5;
    // テーマ色: 複合の深い紫紺
    bgR = 0.10f; bgG = 0.05f; bgB = 0.12f;
}

//======================================
// Boss インターフェース実装
//======================================
std::vector<std::shared_ptr<Ability>> EngravedOneBoss::GetBossAbilities()
{
    // ボス側ギミック: 設置のたび 設置系(氷盤/連鎖)+構造系(回転)+駒強化系(重力)
    // の3カテゴリを同時に全発動する複合体
    return { std::make_shared<BossEngravedAbility>() };
}

std::shared_ptr<Ability> EngravedOneBoss::GetRewardAbility()
{
    // ラスボス撃破はランクリアとなり報酬画面を挟まないため、通常は未使用。
    return std::make_shared<ContemplationAbility>();
}
