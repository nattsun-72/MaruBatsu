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
    description = "過去ボスの力を\n3つ同時に振るう";
    rarity      = Rarity::Legendary;
    pulseSteps  = Config::GetInt("bosses.engraved.pulseSteps", 1);

    // ラン開始時に「設置系」を氷盤/連鎖から確定 (構造系=回転, 駒強化系=重力は固定)
    placeMode  = (RollInt(2) == 0) ? EngravedPlaceMode::Ice : EngravedPlaceMode::Chain;
    currentDir = static_cast<Direction>(RollInt(4));
}

//======================================
// IPlacementHandler
//======================================
void BossEngravedAbility::OnPlace(GameState& state, Vec2 pos, Piece placedBy)
{
    // 設置のたび、3カテゴリのギミックを順に全て発動する(弱化なし)。
    // 順序: 設置系 → 構造系(回転) → 駒強化系(重力で全駒落下)。

    /*--- 設置系: 連鎖(ボス設置のみ) または 氷盤スライド ---*/
    if (placeMode == EngravedPlaceMode::Chain)
    {
        if (placedBy == Piece::Enemy)
        {
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
        }
    }
    else // Ice
    {
        BoardOps::MoveList moves;
        BoardOps::SlideAll(state.board, currentDir, &moves, state.slideAnchorSide);
        if (!moves.empty()) state.animPhases.push_back(std::move(moves));
    }

    /*--- 構造系: 盤面を90度回転 ---*/
    {
        BoardOps::MoveList moves;
        BoardOps::Rotate90(state.board, clockwise, &moves);
        if (!moves.empty()) state.animPhases.push_back(std::move(moves));
    }

    /*--- 駒強化系(重力): 全駒を下端へ落下させる(重駒=固定駒は除外) ---*/
    {
        BoardOps::MoveList moves;
        BoardOps::SlideAll(state.board, Direction::Down, &moves, state.slideAnchorSide);
        if (!moves.empty()) state.animPhases.push_back(std::move(moves));
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

    // 氷盤の滑走方向だけはターンごとに抽選し直す(3つの力の構成は固定)。
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

const char* BossEngravedAbility::PowersLabel() const
{
    // 構造系(螺旋)・駒強化系(重力)は固定。設置系のみ氷盤/連鎖で変わる。
    return (placeMode == EngravedPlaceMode::Chain)
        ? "連鎖＋螺旋＋重力"
        : "氷盤＋螺旋＋重力";
}
