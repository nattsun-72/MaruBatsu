/****************************************
 * @file ability_pool.cpp
 * @brief アビリティプール実装
 * @author Natsume Shidara
 * @date 2026/05/15
 ****************************************/
#include "ability_pool.h"

#include "ability/abilities/contemplation.h"
#include "ability/abilities/ice_tile.h"
#include "ability/abilities/line_added.h"
#include "ability/abilities/margin.h"
#include "ability/abilities/two_hands.h"

#include <algorithm>
#include <random>

namespace AbilityPool
{
    std::vector<std::shared_ptr<Ability>> CreateAll()
    {
        return {
            std::make_shared<ContemplationAbility>(),
            std::make_shared<MarginAbility>(),
            std::make_shared<IceTileAbility>(),
            std::make_shared<LineAddedAbility>(),
            std::make_shared<TwoHandsAbility>(),
        };
    }

    std::vector<std::shared_ptr<Ability>> PickRandom(int count)
    {
        auto all = CreateAll();
        static std::mt19937 rng(std::random_device{}());
        std::shuffle(all.begin(), all.end(), rng);
        if (static_cast<int>(all.size()) > count) all.resize(count);
        return all;
    }
}
