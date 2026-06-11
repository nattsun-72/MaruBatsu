/****************************************
 * @file   engraved_one_boss.cpp
 * @brief  ボス5(ラスボス): 刻まれし者 実装 (W1スタブ)
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
    description = "毎ターン、過去の支配者たちの力を振るう";
    difficulty  = 5;
    // テーマ色: 複合の深い紫紺
    bgR = 0.10f; bgG = 0.05f; bgB = 0.12f;
}

//======================================
// Boss インターフェース実装
//======================================
std::vector<std::shared_ptr<Ability>> EngravedOneBoss::GetBossAbilities()
{
    // ボス側ギミック: 毎ターン「氷盤/螺旋/連鎖/重力」を切り替えて振るう複合体
    return { std::make_shared<BossEngravedAbility>() };
}

std::shared_ptr<Ability> EngravedOneBoss::GetRewardAbility()
{
    // ラスボス撃破はランクリアとなり報酬画面を挟まないため、通常は未使用。
    return std::make_shared<ContemplationAbility>();
}
