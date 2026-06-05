/****************************************
 * @file   chain_pulse_boss.cpp
 * @brief  ボス3: 連鎖の鼓動 実装 (W1スタブ)
 * @author Natsume Shidara
 * @date   2026/06/05
 ****************************************/
#include "chain_pulse_boss.h"
#include "ability/abilities/contemplation.h"

//======================================
// 構築
//======================================
ChainPulseBoss::ChainPulseBoss()
{
    name        = "連鎖の鼓動";
    description = "やがて駒が自動で連鎖拡張する（実装予定）";
    difficulty  = 3;
    // テーマ色: 深い赤 (鼓動)
    bgR = 0.14f; bgG = 0.04f; bgB = 0.05f;
}

//======================================
// Boss インターフェース実装
//======================================
std::vector<std::shared_ptr<Ability>> ChainPulseBoss::GetBossAbilities()
{
    // W1第1増分: ギミック未実装。空を返すと通常の〇×として成立する。
    // 自動拡張ギミック(IPlacementHandler系)は後続増分でここに追加する。
    return {};
}

std::shared_ptr<Ability> ChainPulseBoss::GetRewardAbility()
{
    // 暫定報酬。実際の報酬画面はプール抽選のため現状この戻り値は未使用。
    return std::make_shared<ContemplationAbility>();
}
