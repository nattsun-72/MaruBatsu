/****************************************
 * @file   engraved_one_boss.cpp
 * @brief  ボス5(ラスボス): 刻まれし者 実装 (W1スタブ)
 * @author Natsume Shidara
 * @date   2026/06/05
 ****************************************/
#include "engraved_one_boss.h"
#include "ability/abilities/contemplation.h"

//======================================
// 構築
//======================================
EngravedOneBoss::EngravedOneBoss()
{
    name        = "刻まれし者";
    description = "過去のボスたちの力が宿る（実装予定）";
    difficulty  = 5;
}

//======================================
// Boss インターフェース実装
//======================================
std::vector<std::shared_ptr<Ability>> EngravedOneBoss::GetBossAbilities()
{
    // W1第1増分: 複合ギミック未実装。空を返すと通常の〇×として成立する。
    // 撃破でランクリア(完走)に到達するフローをまず通すのが本スタブの目的。
    return {};
}

std::shared_ptr<Ability> EngravedOneBoss::GetRewardAbility()
{
    // ラスボス撃破はランクリアとなり報酬画面を挟まないため、通常は未使用。
    return std::make_shared<ContemplationAbility>();
}
