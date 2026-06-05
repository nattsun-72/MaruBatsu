/****************************************
 * @file   ability_pool.cpp
 * @brief  アビリティプールの実装
 * @author Natsume Shidara
 * @date   2026/05/15
 ****************************************/
#include "ability_pool.h"

#include "ability/abilities/board_expand.h"
#include "ability/abilities/contemplation.h"
#include "ability/abilities/focus.h"
#include "ability/abilities/ice_tile.h"
#include "ability/abilities/margin.h"
#include "ability/abilities/two_hands.h"

#include <algorithm>
#include <random>

namespace AbilityPool
{
    //======================================
    // 候補生成・抽選
    //======================================
    std::vector<std::shared_ptr<Ability>> CreateAll()
    {
        // 報酬候補となる全アビリティを、毎回新規インスタンスで生成する
        return {
            std::make_shared<ContemplationAbility>(),
            std::make_shared<MarginAbility>(),
            std::make_shared<FocusAbility>(),
            std::make_shared<IceTileAbility>(),
            std::make_shared<BoardExpandAbility>(),
            std::make_shared<TwoHandsAbility>(),
        };
    }

    std::shared_ptr<Ability> CreateByName(const std::string& name)
    {
        // 全候補を生成し、名前が一致した新規インスタンスを返す
        for (auto& a : CreateAll())
        {
            if (a->name == name) return a;
        }
        return nullptr;  // 該当なし
    }

    std::vector<std::shared_ptr<Ability>>
    PickRandom(int count, const std::vector<std::string>& excludeNames)
    {
        auto all = CreateAll();

        // 除外名に該当する候補 (取得済みの一度限りアビリティ等) を取り除く
        all.erase(std::remove_if(all.begin(), all.end(),
            [&](const std::shared_ptr<Ability>& a)
            {
                return std::find(excludeNames.begin(), excludeNames.end(),
                                 a->name) != excludeNames.end();
            }), all.end());

        // 残った候補をシャッフルし、先頭 count 個を取り出す
        static std::mt19937 rng(std::random_device{}());
        std::shuffle(all.begin(), all.end(), rng);
        if (static_cast<int>(all.size()) > count) all.resize(count);
        return all;
    }
}
