/****************************************
 * @file   boss_chain_pulse.cpp
 * @brief  連鎖鼓動 (ボス側アビリティ) の実装
 * @author Natsume Shidara
 * @date   2026/06/05
 ****************************************/
#include "boss_chain_pulse.h"

#include "ability/ability_registry.h"
#include "board.h"
#include "board_ops.h"
#include "game_state.h"

#include <random>
#include <vector>

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
     * @brief  指定マスの上下左右で空いている隣接マスから1つをランダムに返す
     * @param  board 対象の盤面
     * @param  from  起点のマス座標
     * @return 選ばれた空き隣接マス。無ければ {-1,-1}。
     */
    Vec2 PickEmptyNeighbor(const Board& board, Vec2 from)
    {
        static const int dx[4] = { 1, -1, 0, 0 };
        static const int dy[4] = { 0, 0, 1, -1 };

        std::vector<Vec2> candidates;
        candidates.reserve(4);
        for (int i = 0; i < 4; ++i)
        {
            const int nx = from.x + dx[i];
            const int ny = from.y + dy[i];
            if (board.IsEmpty(nx, ny)) candidates.push_back({ nx, ny });
        }
        if (candidates.empty()) return { -1, -1 };

        std::uniform_int_distribution<size_t> dist(0, candidates.size() - 1);
        return candidates[dist(Rng())];
    }
}

//======================================
// 構築
//======================================
BossChainPulseAbility::BossChainPulseAbility()
{
    name        = "連鎖鼓動";
    description = "ボスが駒を置くたび\n隣接マスへ拡張する";
    rarity      = Rarity::Rare;
}

//======================================
// IPlacementHandler
//======================================
void BossChainPulseAbility::OnPlace(GameState& state, Vec2 pos, Piece placedBy)
{
    if (placedBy != targetSide) return;   // 対象陣営の設置のみ拡張する

    /*--- 起点から連鎖的に隣接空きマスへ同色駒を出現させる ---*/
    BoardOps::MoveList spawns;
    Vec2 from = pos;
    for (int step = 0; step < pulseSteps; ++step)
    {
        const Vec2 cell = PickEmptyNeighbor(state.board, from);
        if (cell.x < 0) break;            // 空き隣接が無ければ連鎖終了

        state.board.Set(cell.x, cell.y, targetSide);
        // 出現(isSpawn): from=発生源(盤上に残る), to=出現先
        spawns.push_back({ from.x, from.y, cell.x, cell.y, targetSide, true });
        from = cell;                      // 出現した駒を起点に次の連鎖へ
    }

    // 各出現を個別フェーズにして順次(連鎖的に)アニメ再生する
    for (const auto& s : spawns)
    {
        state.animPhases.push_back({ s });
    }
}

//======================================
// Ability
//======================================
void BossChainPulseAbility::RegisterTo(AbilityRegistry& registry)
{
    // 設置ハンドラとして追加登録 (他の設置効果と共存可能)
    auto self = shared_from_this();
    registry.AddPlacementHandler(std::dynamic_pointer_cast<IPlacementHandler>(self));
}
