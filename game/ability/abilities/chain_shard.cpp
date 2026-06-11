/****************************************
 * @file   chain_shard.cpp
 * @brief  連鎖の欠片の実装
 * @author Natsume Shidara
 * @date   2026/06/11
 ****************************************/
#include "chain_shard.h"

#include "ability/ability_registry.h"
#include "board.h"
#include "board_ops.h"
#include "config.h"
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
ChainShardAbility::ChainShardAbility()
{
    name        = "連鎖の欠片";
    description = "クリックで装填。次の\n自駒設置で隣へ拡張";
    rarity      = Rarity::Rare;
    activatable = true;
    flashText   = "連鎖装填！";
    charges     = Config::GetInt("abilities.chainShard.charges", 3);
}

//======================================
// 任意発動アビリティのインターフェイス
//======================================
void ChainShardAbility::Activate(GameState& /*state*/)
{
    if (charges <= 0 || armed) return;
    --charges;
    armed = true;   // 次の自駒設置で OnPlace が拡張を実行する
}

bool ChainShardAbility::CanActivate(const GameState& /*state*/) const
{
    return charges > 0 && !armed;
}

//======================================
// IPlacementHandler
//======================================
void ChainShardAbility::OnPlace(GameState& state, Vec2 pos, Piece placedBy)
{
    if (!armed) return;                    // 装填していなければ何もしない
    if (placedBy != Piece::Player) return; // 自駒の設置のみ対象

    armed = false;   // 装填を消費 (拡張先が無くても消費される)

    // 設置マスの隣接空きマスへ自駒を1個出現させる
    const Vec2 cell = PickEmptyNeighbor(state.board, pos);
    if (cell.x < 0) return;

    state.board.Set(cell.x, cell.y, Piece::Player);
    // 出現(isSpawn): from=発生源(盤上に残る), to=出現先
    state.animPhases.push_back({ { pos.x, pos.y, cell.x, cell.y, Piece::Player, true } });
}

//======================================
// Ability
//======================================
void ChainShardAbility::RegisterTo(AbilityRegistry& registry)
{
    // 設置ハンドラとして追加登録 (他の設置効果と共存可能)
    auto self = shared_from_this();
    registry.AddPlacementHandler(std::dynamic_pointer_cast<IPlacementHandler>(self));
}
