/****************************************
 * @file   ability_pool.cpp
 * @brief  アビリティプールの実装
 * @author Natsume Shidara
 * @date   2026/05/15
 * @update 2026/06/11 - レアリティ重み付き抽選・新アビリティ追加
 ****************************************/
#include "ability_pool.h"

#include "ability/abilities/absolute_victory.h"
#include "ability/abilities/board_expand.h"
#include "ability/abilities/chain_shard.h"
#include "ability/abilities/contemplation.h"
#include "ability/abilities/corner_dominion.h"
#include "ability/abilities/diagonal_void.h"
#include "ability/abilities/focus.h"
#include "ability/abilities/heavy_piece.h"
#include "ability/abilities/ice_tile.h"
#include "ability/abilities/immortal.h"
#include "ability/abilities/l_win.h"
#include "ability/abilities/margin.h"
#include "ability/abilities/reflection.h"
#include "ability/abilities/sands_of_time.h"
#include "ability/abilities/spiral_shard.h"
#include "ability/abilities/time_stop.h"
#include "ability/abilities/two_hands.h"

#include "config.h"

#include <algorithm>
#include <random>

//--------------------------------------
// 内部ヘルパ
//--------------------------------------
namespace
{
    /** @brief 乱数エンジン (初回呼び出し時に生成し共有する) */
    std::mt19937& Rng()
    {
        static std::mt19937 rng(std::random_device{}());
        return rng;
    }

    /**
     * @brief  レアリティの抽選重みを設定ファイルから取得する
     * @param  r レアリティ
     * @return 重み (大きいほど出やすい)。0以下なら抽選対象外。
     */
    int RarityWeight(Rarity r)
    {
        switch (r)
        {
        case Rarity::Common:    return Config::GetInt("rarityWeights.common",    50);
        case Rarity::Rare:      return Config::GetInt("rarityWeights.rare",      30);
        case Rarity::Epic:      return Config::GetInt("rarityWeights.epic",      16);
        case Rarity::Legendary: return Config::GetInt("rarityWeights.legendary",  4);
        case Rarity::Curse:     return Config::GetInt("rarityWeights.curse",      0);
        }
        return 0;
    }
}

namespace AbilityPool
{
    //======================================
    // 候補生成・抽選
    //======================================
    std::vector<std::shared_ptr<Ability>> CreateAll()
    {
        // 報酬候補となる全アビリティを、毎回新規インスタンスで生成する。
        // ※ セーブ復元(CreateByName)もここを参照するため、新アビリティは
        //    必ずこの一覧に追加すること。
        return {
            std::make_shared<ContemplationAbility>(),  // 熟考 (C)
            std::make_shared<MarginAbility>(),         // 余裕 (R)
            std::make_shared<FocusAbility>(),          // 集中 (C/任意発動)
            std::make_shared<IceTileAbility>(),        // 氷駒 (R)
            std::make_shared<BoardExpandAbility>(),    // 盤面拡張 (E/一度限り)
            std::make_shared<TwoHandsAbility>(),       // 二手打ち (E/一度限り)
            std::make_shared<SpiralShardAbility>(),    // 螺旋の欠片 (R/任意発動)
            std::make_shared<ChainShardAbility>(),     // 連鎖の欠片 (R/任意発動)
            std::make_shared<HeavyPieceAbility>(),     // 重駒 (E/一度限り)
            std::make_shared<LWinAbility>(),           // L字勝利 (E/一度限り)
            std::make_shared<DiagonalVoidAbility>(),   // 対角線無効 (R/一度限り)
            std::make_shared<ImmortalAbility>(),       // 不死 (L/一度限り)
            std::make_shared<SandsOfTimeAbility>(),    // 時の砂 (R/任意発動)
            std::make_shared<ReflectionAbility>(),     // 鏡映 (E/一度限り)
            std::make_shared<CornerDominionAbility>(), // 隅取り (E/一度限り)
            std::make_shared<TimeStopAbility>(),       // 時間停止 (L/一度限り)
            std::make_shared<AbsoluteVictoryAbility>(),// 絶対勝利 (L/一度限り)
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

        /*--- レアリティ重み付きで count 個を非復元抽選する ---*/
        std::vector<std::shared_ptr<Ability>> picked;
        picked.reserve(static_cast<size_t>(count));
        while (static_cast<int>(picked.size()) < count && !all.empty())
        {
            // 残候補の重み合計を求める (重み0以下は対象外として除く)
            long long total = 0;
            for (const auto& a : all)
            {
                const int w = RarityWeight(a->rarity);
                if (w > 0) total += w;
            }
            if (total <= 0) break;   // 抽選可能な候補が無い

            // 重みに比例したルーレット選択
            std::uniform_int_distribution<long long> dist(0, total - 1);
            long long roll = dist(Rng());
            size_t chosen = all.size();
            for (size_t i = 0; i < all.size(); ++i)
            {
                const int w = RarityWeight(all[i]->rarity);
                if (w <= 0) continue;
                if (roll < w) { chosen = i; break; }
                roll -= w;
            }
            if (chosen >= all.size()) break;   // 念のためのガード

            picked.push_back(all[chosen]);
            all.erase(all.begin() + chosen);   // 非復元 (同名重複を防ぐ)
        }
        return picked;
    }
}
