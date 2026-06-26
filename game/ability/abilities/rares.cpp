/****************************************
 * @file   rares.cpp
 * @brief  レアアビリティ群の実装
 * @author Natsume Shidara
 * @date   2026/06/25
 ****************************************/
#include "rares.h"

#include "ability/ability_registry.h"
#include "board.h"
#include "board_ops.h"
#include "direction.h"
#include "game_state.h"
#include "win_check.h"

namespace
{
    /** @brief どこかに player の 2×2 ブロックがあるか */
    bool HasSquare(const Board& b, Piece p)
    {
        for (int y = 0; y + 1 < b.height; ++y)
            for (int x = 0; x + 1 < b.width; ++x)
                if (b.Get(x, y)     == p && b.Get(x + 1, y)     == p
                 && b.Get(x, y + 1) == p && b.Get(x + 1, y + 1) == p)
                    return true;
        return false;
    }

    /** @brief 中央が定まる奇数正方盤か (十字・四辺中点の判定に使う) */
    bool OddSquare(const Board& b)
    {
        return b.width == b.height && b.width >= 3 && (b.width % 2) == 1;
    }

    /** @brief 四辺の中点をすべて player が支配しているか */
    bool HasEdgeMidpoints(const Board& b, Piece p)
    {
        if (!OddSquare(b)) return false;
        const int cx = b.width / 2, cy = b.height / 2;
        return b.Get(cx, 0) == p && b.Get(cx, b.height - 1) == p
            && b.Get(0, cy) == p && b.Get(b.width - 1, cy)  == p;
    }
}

//======================================
// 1. 四角の覇
//======================================
SquareSovereignRare::SquareSovereignRare()
{
    name        = "四角の覇";
    description = "2×2のブロックを\n支配しても勝利";
    rarity      = Rarity::Rare;
    unique      = true;
}
bool SquareSovereignRare::Check(const Board& board, Piece player)
{
    if (chainedWin && chainedWin->Check(board, player)) return true;
    if (player != targetSide) return false;
    return HasSquare(board, player);
}
void SquareSovereignRare::RegisterTo(AbilityRegistry& registry)
{
    auto self = std::dynamic_pointer_cast<IWinCondition>(shared_from_this());
    chainedWin = registry.SetWinCondition(self);
}

//======================================
// 2. 奔流 (全駒を右へ寄せる)
//======================================
TorrentRare::TorrentRare()
{
    name        = "奔流";
    description = "クリックで全ての駒を\n右へ寄せる";
    rarity      = Rarity::Rare;
    activatable = true;
    flashText   = "奔流！";
}
void TorrentRare::Activate(GameState& state)
{
    if (charges <= 0) return;
    --charges;
    BoardOps::MoveList moves;
    BoardOps::SlideAll(state.board, Direction::Right, &moves, state.slideAnchorSide);
    if (!moves.empty()) state.animPhases.push_back(std::move(moves));
}
bool TorrentRare::CanActivate(const GameState& /*state*/) const { return charges > 0; }

//======================================
// 3. 布陣の妙
//======================================
EdgeFormationRare::EdgeFormationRare()
{
    name        = "布陣の妙";
    description = "四辺の中点を\nすべて支配で勝利";
    rarity      = Rarity::Rare;
    unique      = true;
}
bool EdgeFormationRare::Check(const Board& board, Piece player)
{
    if (chainedWin && chainedWin->Check(board, player)) return true;
    if (player != targetSide) return false;
    return HasEdgeMidpoints(board, player);
}
void EdgeFormationRare::RegisterTo(AbilityRegistry& registry)
{
    auto self = std::dynamic_pointer_cast<IWinCondition>(shared_from_this());
    chainedWin = registry.SetWinCondition(self);
}

//======================================
// 4. 雪崩
//======================================
AvalancheRare::AvalancheRare()
{
    name        = "雪崩";
    description = "クリックで全ての駒を\n下へ落とす";
    rarity      = Rarity::Rare;
    activatable = true;
    flashText   = "落下！";
}
void AvalancheRare::Activate(GameState& state)
{
    if (charges <= 0) return;
    --charges;
    BoardOps::MoveList moves;
    BoardOps::SlideAll(state.board, Direction::Down, &moves, state.slideAnchorSide);
    if (!moves.empty()) state.animPhases.push_back(std::move(moves));
}
bool AvalancheRare::CanActivate(const GameState& /*state*/) const { return charges > 0; }

//======================================
// 5. 旋風
//======================================
WhirlwindRare::WhirlwindRare()
{
    name        = "旋風";
    description = "クリックで盤面を\n反時計回りに回転";
    rarity      = Rarity::Rare;
    activatable = true;
    flashText   = "回転！";
}
void WhirlwindRare::Activate(GameState& state)
{
    if (charges <= 0) return;
    --charges;
    BoardOps::MoveList moves;
    BoardOps::Rotate90(state.board, false, &moves);   // 反時計回り
    if (!moves.empty()) state.animPhases.push_back(std::move(moves));
}
bool WhirlwindRare::CanActivate(const GameState& /*state*/) const { return charges > 0; }

//======================================
// 6. 求心
//======================================
CentripetalRare::CentripetalRare()
{
    name        = "求心";
    description = "置いた自駒が\n中央へ1マス寄る";
    rarity      = Rarity::Rare;
    unique      = true;
}
void CentripetalRare::OnPlace(GameState& state, Vec2 pos, Piece placedBy)
{
    if (placedBy != Piece::Player) return;
    if (state.slideAnchorSide == Piece::Player) return;        // 重駒は滑らない
    if (state.board.Get(pos.x, pos.y) != Piece::Player) return; // 既に他効果で移動済み

    // 盤面中心(連続座標)へ向かう支配的な軸へ1マス寄せる
    const float cx  = (state.board.width  - 1) * 0.5f;
    const float cy  = (state.board.height - 1) * 0.5f;
    const float ddx = cx - pos.x;
    const float ddy = cy - pos.y;
    if (ddx > -0.5f && ddx < 0.5f && ddy > -0.5f && ddy < 0.5f) return;  // 既に中央

    const float adx = (ddx < 0.0f) ? -ddx : ddx;
    const float ady = (ddy < 0.0f) ? -ddy : ddy;
    Direction dir;
    if (adx >= ady) dir = (ddx > 0.0f) ? Direction::Right : Direction::Left;
    else            dir = (ddy > 0.0f) ? Direction::Down  : Direction::Up;

    BoardOps::MoveList moves;
    if (BoardOps::SlideOne(state.board, pos.x, pos.y, dir, &moves))
        state.animPhases.push_back(std::move(moves));
}
void CentripetalRare::RegisterTo(AbilityRegistry& registry)
{
    auto self = std::dynamic_pointer_cast<IPlacementHandler>(shared_from_this());
    registry.AddPlacementHandler(self);
}

//======================================
// 7. 不屈の魂
//======================================
UnyieldingRare::UnyieldingRare()
{
    name        = "不屈の魂";
    description = "敗北を1度だけ\n打ち消す";
    rarity      = Rarity::Rare;
    unique      = true;
}
bool UnyieldingRare::OnDefeat(GameState& /*state*/, Piece loser)
{
    if (loser != Piece::Player) return false;
    if (charges <= 0)           return false;
    --charges;
    return true;
}
void UnyieldingRare::RegisterTo(AbilityRegistry& registry)
{
    auto self = std::dynamic_pointer_cast<IDefeatHandler>(shared_from_this());
    registry.AddDefeatHandler(self);
}

//======================================
// 8. 快進撃
//======================================
OnslaughtRare::OnslaughtRare()
{
    name        = "快進撃";
    description = "2手 打てて毎ターン\n+30秒 になる";
    rarity      = Rarity::Rare;
    unique      = true;
}
int OnslaughtRare::GetPlacementCount(Piece player)
{
    if (player == Piece::Player) return placementCount;
    return chainedTurn ? chainedTurn->GetPlacementCount(player) : 1;
}
void OnslaughtRare::OnTurnStart(GameState& s, Piece p) { if (chainedTurn) chainedTurn->OnTurnStart(s, p); }
void OnslaughtRare::OnTurnEnd  (GameState& s, Piece p) { if (chainedTurn) chainedTurn->OnTurnEnd(s, p); }
double OnslaughtRare::GetTurnTimeSeconds(Piece player)
{
    const double base = chainedTime ? chainedTime->GetTurnTimeSeconds(player) : 180.0;
    return (player == Piece::Player) ? base + bonusSeconds : base;
}
void OnslaughtRare::RegisterTo(AbilityRegistry& registry)
{
    auto self = shared_from_this();
    chainedTurn = registry.SetTurnHandler(std::dynamic_pointer_cast<ITurnHandler>(self));
    chainedTime = registry.SetTurnTimeProvider(std::dynamic_pointer_cast<ITurnTimeProvider>(self));
}

//======================================
// 9. 先見の明 (集中派生・残時間を floor まで補充)
//======================================
ForesightRare::ForesightRare()
{
    name        = "先見の明";
    rarity      = Rarity::Rare;
    charges     = 2;
    floorSeconds = 150.0;
    description = "クリックで残り時間を\n150秒まで補充";
    flashText   = "時間補充";
}
void ForesightRare::Activate(GameState& state)
{
    if (charges <= 0) return;
    if (state.remainingTime < floorSeconds) state.remainingTime = floorSeconds;
    --charges;
}
bool ForesightRare::CanActivate(const GameState& state) const
{
    return charges > 0 && state.remainingTime < floorSeconds;
}

//======================================
// 10. 増援 (このターンの着手を+1)
//======================================
ReinforcementRare::ReinforcementRare()
{
    name        = "増援";
    description = "クリックでこのターン\nの着手を+1する";
    rarity      = Rarity::Rare;
    activatable = true;
    flashText   = "+1着手";
}
void ReinforcementRare::Activate(GameState& state)
{
    if (charges <= 0) return;
    --charges;
    state.placementsRemaining += 1;   // このターンの残り着手を1増やす
}
bool ReinforcementRare::CanActivate(const GameState& /*state*/) const { return charges > 0; }
