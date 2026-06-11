/****************************************
 * @file   boss_engraved.cpp
 * @brief  刻まれし力 (ボス側アビリティ) の実装
 * @author Natsume Shidara
 * @date   2026/06/11
 ****************************************/
#include "boss_engraved.h"

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

    /** @brief 0..(n-1) の一様乱数 */
    int RollInt(int n)
    {
        std::uniform_int_distribution<int> dist(0, n - 1);
        return dist(Rng());
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
        return candidates[static_cast<size_t>(RollInt(static_cast<int>(candidates.size())))];
    }
}

//======================================
// 構築
//======================================
BossEngravedAbility::BossEngravedAbility()
{
    name        = "刻まれし力";
    description = "毎ターン過去ボスの\nギミックを振るう";
    rarity      = Rarity::Legendary;
    pulseSteps  = Config::GetInt("bosses.engraved.pulseSteps", 1);

    // 初期モードも抽選しておく (初回ターン開始前の表示用)
    mode       = static_cast<EngravedMode>(RollInt(4));
    currentDir = static_cast<Direction>(RollInt(4));
}

//======================================
// IPlacementHandler
//======================================
void BossEngravedAbility::OnPlace(GameState& state, Vec2 pos, Piece placedBy)
{
    switch (mode)
    {
    case EngravedMode::Ice:
    {
        /*--- 氷盤: 全駒を currentDir 方向へスライド ---*/
        BoardOps::MoveList moves;
        BoardOps::SlideAll(state.board, currentDir, &moves, state.slideAnchorSide);
        if (!moves.empty()) state.animPhases.push_back(std::move(moves));
        break;
    }
    case EngravedMode::Spiral:
    {
        /*--- 螺旋: 盤面を90度回転 ---*/
        BoardOps::MoveList moves;
        BoardOps::Rotate90(state.board, clockwise, &moves);
        if (!moves.empty()) state.animPhases.push_back(std::move(moves));
        break;
    }
    case EngravedMode::Chain:
    {
        /*--- 連鎖: ボスの設置時のみ、隣接空きマスへ拡張 ---*/
        if (placedBy != Piece::Enemy) break;
        Vec2 from = pos;
        for (int step = 0; step < pulseSteps; ++step)
        {
            const Vec2 cell = PickEmptyNeighbor(state.board, from);
            if (cell.x < 0) break;
            state.board.Set(cell.x, cell.y, Piece::Enemy);
            state.animPhases.push_back(
                { { from.x, from.y, cell.x, cell.y, Piece::Enemy, true } });
            from = cell;
        }
        break;
    }
    case EngravedMode::Gravity:
    {
        /*--- 重力: 置かれた駒を列の底まで落下 (重駒は対象外) ---*/
        if (state.slideAnchorSide == placedBy) break;

        // 先行フックで滑った後に備え、設置マスから下へ走査して現在位置を求める
        int cy = pos.y;
        while (state.board.IsValid(pos.x, cy)
            && state.board.Get(pos.x, cy) == Piece::Empty)
        {
            ++cy;
        }
        if (state.board.Get(pos.x, cy) != placedBy) break;

        BoardOps::MoveList moves;
        if (BoardOps::DropDown(state.board, pos.x, cy, &moves))
        {
            state.animPhases.push_back(std::move(moves));
        }
        break;
    }
    }
}

//======================================
// ITurnHandler
//======================================
int BossEngravedAbility::GetPlacementCount(Piece player)
{
    return chainedTurn ? chainedTurn->GetPlacementCount(player) : 1;
}

void BossEngravedAbility::OnTurnStart(GameState& state, Piece player)
{
    if (chainedTurn) chainedTurn->OnTurnStart(state, player);

    // 毎ターン、振るう力と方向を抽選し直す (HUDが事前に提示する)
    mode       = static_cast<EngravedMode>(RollInt(4));
    currentDir = static_cast<Direction>(RollInt(4));
}

void BossEngravedAbility::OnTurnEnd(GameState& state, Piece player)
{
    if (chainedTurn) chainedTurn->OnTurnEnd(state, player);
}

//======================================
// Ability
//======================================
void BossEngravedAbility::RegisterTo(AbilityRegistry& registry)
{
    auto self = shared_from_this();
    // 設置ハンドラとして追加登録
    registry.AddPlacementHandler(std::dynamic_pointer_cast<IPlacementHandler>(self));
    // ターン処理を差し替え、旧実装はチェインで保持する
    auto turnSelf = std::dynamic_pointer_cast<ITurnHandler>(self);
    chainedTurn = registry.SetTurnHandler(turnSelf);
}

const char* BossEngravedAbility::ModeName() const
{
    switch (mode)
    {
    case EngravedMode::Ice:     return "氷盤";
    case EngravedMode::Spiral:  return "螺旋";
    case EngravedMode::Chain:   return "連鎖";
    case EngravedMode::Gravity: return "重力";
    }
    return "?";
}
