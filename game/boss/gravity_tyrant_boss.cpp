/****************************************
 * @file   gravity_tyrant_boss.cpp
 * @brief  ボス4: 重力の専制者 実装 (W1スタブ)
 * @author Natsume Shidara
 * @date   2026/06/05
 ****************************************/
#include "gravity_tyrant_boss.h"
#include "ability/abilities/boss_gravity.h"
#include "ability/abilities/heavy_piece.h"

//======================================
// 構築
//======================================
GravityTyrantBoss::GravityTyrantBoss()
{
    name        = "重力の専制者";
    description = "置かれた駒は、列の底へ落下する";
    difficulty  = 4;
    // テーマ色: 鋼鉄の灰青 (金属)
    bgR = 0.09f; bgG = 0.10f; bgB = 0.12f;
}

//======================================
// Boss インターフェース実装
//======================================
std::vector<std::shared_ptr<Ability>> GravityTyrantBoss::GetBossAbilities()
{
    // ボス側ギミック: 設置された駒を列の底まで落下させる(四目並べ風)
    return { std::make_shared<BossGravityAbility>() };
}

std::shared_ptr<Ability> GravityTyrantBoss::GetRewardAbility()
{
    // 撃破報酬: 自駒が滑り・落下を受けなくなる「重駒」
    return std::make_shared<HeavyPieceAbility>();
}
