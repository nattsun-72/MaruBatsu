/****************************************
 * @file   mirror_warden_boss.cpp
 * @brief  ボス: 鏡写しの番人 実装
 * @author Natsume Shidara
 * @date   2026/06/19
 ****************************************/
#include "mirror_warden_boss.h"
#include "ability/abilities/boss_mirror.h"
#include "ability/abilities/reflection.h"

//======================================
// 構築
//======================================
MirrorWardenBoss::MirrorWardenBoss()
{
    name        = "鏡写しの番人";
    description = "あなたの設置を、点対称にミラーする";
    difficulty  = 2;
    // テーマ色: 鏡面の青緑
    bgR = 0.04f; bgG = 0.10f; bgB = 0.11f;
}

//======================================
// Boss インターフェース実装
//======================================
std::vector<std::shared_ptr<Ability>> MirrorWardenBoss::GetBossAbilities()
{
    // ボス側ギミック: プレイヤー設置を点対称にミラー設置
    return { std::make_shared<BossMirrorAbility>() };
}

std::shared_ptr<Ability> MirrorWardenBoss::GetRewardAbility()
{
    // 撃破報酬: 自駒を点対称マスにも出す「鏡映」(味方版)
    return std::make_shared<ReflectionAbility>();
}
