/****************************************
 * @file ability_pool.h
 * @brief 報酬抽選用アビリティプール
 * @author Natsume Shidara
 * @date 2026/05/15
 *
 * 報酬画面の3択生成に使う候補リスト。
 * α/β でレアリティ重み付き抽選に拡張予定。
 ****************************************/
#ifndef ABILITY_POOL_H
#define ABILITY_POOL_H

#include "ability/ability.h"
#include <memory>
#include <vector>

namespace AbilityPool
{
    /** @brief 全候補アビリティを新規インスタンスで生成して返す */
    std::vector<std::shared_ptr<Ability>> CreateAll();

    /** @brief プールから N 個をランダム抽選 (重複なし) */
    std::vector<std::shared_ptr<Ability>> PickRandom(int count);
}

#endif // ABILITY_POOL_H
