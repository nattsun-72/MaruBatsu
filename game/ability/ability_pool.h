/****************************************
 * @file   ability_pool.h
 * @brief  報酬抽選用アビリティプール
 * @author Natsume Shidara
 * @date   2026/05/15
 *
 * 報酬画面の3択生成に使う候補リストを提供する。
 * α/β でレアリティ重み付き抽選への拡張を予定。
 ****************************************/
#ifndef ABILITY_POOL_H
#define ABILITY_POOL_H

//--------------------------------------
// インクルード
//--------------------------------------
#include "ability/ability.h"
#include <memory>
#include <string>
#include <vector>

//======================================
// アビリティプール 名前空間
//======================================
namespace AbilityPool
{
    /** @brief 全候補アビリティを、新規インスタンスで生成して返す */
    std::vector<std::shared_ptr<Ability>> CreateAll();

    /**
     * @brief  プールから指定個数をランダム抽選する (重複なし)
     * @param  count        抽選する個数
     * @param  excludeNames 抽選候補から除外するアビリティ名。取得済みの
     *                      一度限りアビリティ等を渡す。既定は除外なし。
     * @return 抽選されたアビリティ一覧
     */
    std::vector<std::shared_ptr<Ability>>
    PickRandom(int count, const std::vector<std::string>& excludeNames = {});
}

#endif // ABILITY_POOL_H
