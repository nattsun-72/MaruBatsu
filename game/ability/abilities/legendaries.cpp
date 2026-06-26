/****************************************
 * @file   legendaries.cpp
 * @brief  レジェンダリーアビリティ群の実装
 * @author Natsume Shidara
 * @date   2026/06/25
 ****************************************/
#include "legendaries.h"

#include "ability/ability_registry.h"
#include "board.h"
#include "game_state.h"
#include "win_check.h"

namespace
{
    /** @brief 四隅をすべて player が支配しているか */
    bool OwnsAllCorners(const Board& board, Piece player)
    {
        const int x1 = board.width  - 1;
        const int y1 = board.height - 1;
        return board.Get(0, 0)  == player && board.Get(x1, 0)  == player
            && board.Get(0, y1) == player && board.Get(x1, y1) == player;
    }
}

//======================================
// 1. 神速
//======================================
GodspeedLegend::GodspeedLegend()
{
    name        = "神速";
    description = "1ターンに3手\n置けるようになる";
    rarity      = Rarity::Legendary;
    unique      = true;
}
int GodspeedLegend::GetPlacementCount(Piece player)
{
    if (player == Piece::Player) return placementCount;
    return chainedTurn ? chainedTurn->GetPlacementCount(player) : 1;
}
void GodspeedLegend::OnTurnStart(GameState& s, Piece p) { if (chainedTurn) chainedTurn->OnTurnStart(s, p); }
void GodspeedLegend::OnTurnEnd  (GameState& s, Piece p) { if (chainedTurn) chainedTurn->OnTurnEnd(s, p); }
void GodspeedLegend::RegisterTo(AbilityRegistry& registry)
{
    auto self = std::dynamic_pointer_cast<ITurnHandler>(shared_from_this());
    chainedTurn = registry.SetTurnHandler(self);
}

//======================================
// 2. 大局観
//======================================
GrandVisionLegend::GrandVisionLegend()
{
    name        = "大局観";
    description = "毎ターン思考時間\n+300秒 (恒久)";
    rarity      = Rarity::Legendary;
    unique      = true;
}
double GrandVisionLegend::GetTurnTimeSeconds(Piece player)
{
    const double base = chainedTime ? chainedTime->GetTurnTimeSeconds(player) : 180.0;
    return (player == targetSide) ? base + bonusSeconds : base;
}
void GrandVisionLegend::RegisterTo(AbilityRegistry& registry)
{
    auto self = std::dynamic_pointer_cast<ITurnTimeProvider>(shared_from_this());
    chainedTime = registry.SetTurnTimeProvider(self);
}

//======================================
// 3. 絶対防壁
//======================================
AbsoluteBarrierLegend::AbsoluteBarrierLegend()
{
    name        = "絶対防壁";
    description = "敵は4並びでないと\n勝てなくなる";
    rarity      = Rarity::Legendary;
    unique      = true;
}
bool AbsoluteBarrierLegend::Check(const Board& board, Piece player)
{
    if (player != protectedSide)
    {
        // 相手側: 勝利に必要な連続数を引き上げる(3×3では事実上ライン勝利不能)
        return WinCheck::HasLine(board, player, enemyRequired, true);
    }
    return chainedWin ? chainedWin->Check(board, player) : false;
}
void AbsoluteBarrierLegend::RegisterTo(AbilityRegistry& registry)
{
    auto self = std::dynamic_pointer_cast<IWinCondition>(shared_from_this());
    chainedWin = registry.SetWinCondition(self);
}

//======================================
// 4. 永劫回帰
//======================================
EternalReturnLegend::EternalReturnLegend()
{
    name        = "永劫回帰";
    description = "敗北を3度まで\n打ち消す";
    rarity      = Rarity::Legendary;
    unique      = true;
}
bool EternalReturnLegend::OnDefeat(GameState& /*state*/, Piece loser)
{
    if (loser != Piece::Player) return false;
    if (charges <= 0)           return false;
    --charges;
    return true;
}
void EternalReturnLegend::RegisterTo(AbilityRegistry& registry)
{
    auto self = std::dynamic_pointer_cast<IDefeatHandler>(shared_from_this());
    registry.AddDefeatHandler(self);
}

//======================================
// 5. 二重思考
//======================================
DoubleThinkLegend::DoubleThinkLegend()
{
    name        = "二重思考";
    description = "2手 打てて毎ターン\n+120秒 になる";
    rarity      = Rarity::Legendary;
    unique      = true;
}
int DoubleThinkLegend::GetPlacementCount(Piece player)
{
    if (player == Piece::Player) return placementCount;
    return chainedTurn ? chainedTurn->GetPlacementCount(player) : 1;
}
void DoubleThinkLegend::OnTurnStart(GameState& s, Piece p) { if (chainedTurn) chainedTurn->OnTurnStart(s, p); }
void DoubleThinkLegend::OnTurnEnd  (GameState& s, Piece p) { if (chainedTurn) chainedTurn->OnTurnEnd(s, p); }
double DoubleThinkLegend::GetTurnTimeSeconds(Piece player)
{
    const double base = chainedTime ? chainedTime->GetTurnTimeSeconds(player) : 180.0;
    return (player == Piece::Player) ? base + bonusSeconds : base;
}
void DoubleThinkLegend::RegisterTo(AbilityRegistry& registry)
{
    auto self = shared_from_this();
    chainedTurn = registry.SetTurnHandler(std::dynamic_pointer_cast<ITurnHandler>(self));
    chainedTime = registry.SetTurnTimeProvider(std::dynamic_pointer_cast<ITurnTimeProvider>(self));
}

//======================================
// 6. 創世
//======================================
GenesisLegend::GenesisLegend()
{
    name        = "創世";
    description = "盤面が5×5に\n広がる";
    rarity      = Rarity::Legendary;
    unique      = true;
}
void GenesisLegend::GetBoardSize(int& width, int& height)
{
    if (chainedBoard) chainedBoard->GetBoardSize(width, height);
    if (targetWidth  > width)  width  = targetWidth;   // 縮小はしない
    if (targetHeight > height) height = targetHeight;
}
void GenesisLegend::RegisterTo(AbilityRegistry& registry)
{
    auto self = std::dynamic_pointer_cast<IBoardModifier>(shared_from_this());
    chainedBoard = registry.SetBoardModifier(self);
}

//======================================
// 7. 刻印の祝福
//======================================
EngravedBlessingLegend::EngravedBlessingLegend()
{
    name        = "刻印の祝福";
    description = "思考時間が\n常に2倍になる";
    rarity      = Rarity::Legendary;
    unique      = true;
}
double EngravedBlessingLegend::GetTurnTimeSeconds(Piece player)
{
    const double base = chainedTime ? chainedTime->GetTurnTimeSeconds(player) : 180.0;
    return (player == targetSide) ? base * timeFactor : base;
}
void EngravedBlessingLegend::RegisterTo(AbilityRegistry& registry)
{
    auto self = std::dynamic_pointer_cast<ITurnTimeProvider>(shared_from_this());
    chainedTime = registry.SetTurnTimeProvider(self);
}

//======================================
// 8. 王の威光
//======================================
KingsMajestyLegend::KingsMajestyLegend()
{
    name        = "王の威光";
    description = "四隅支配でも勝利\n+毎ターン+120秒";
    rarity      = Rarity::Legendary;
    unique      = true;
}
bool KingsMajestyLegend::Check(const Board& board, Piece player)
{
    if (chainedWin && chainedWin->Check(board, player)) return true;
    if (player != targetSide) return false;
    return OwnsAllCorners(board, player);
}
double KingsMajestyLegend::GetTurnTimeSeconds(Piece player)
{
    const double base = chainedTime ? chainedTime->GetTurnTimeSeconds(player) : 180.0;
    return (player == targetSide) ? base + bonusSeconds : base;
}
void KingsMajestyLegend::RegisterTo(AbilityRegistry& registry)
{
    auto self = shared_from_this();
    chainedWin  = registry.SetWinCondition(std::dynamic_pointer_cast<IWinCondition>(self));
    chainedTime = registry.SetTurnTimeProvider(std::dynamic_pointer_cast<ITurnTimeProvider>(self));
}

//======================================
// 9. 無双
//======================================
PeerlessLegend::PeerlessLegend()
{
    name        = "無双";
    description = "3手 打てて敗北を\n1度 打ち消す";
    rarity      = Rarity::Legendary;
    unique      = true;
}
int PeerlessLegend::GetPlacementCount(Piece player)
{
    if (player == Piece::Player) return placementCount;
    return chainedTurn ? chainedTurn->GetPlacementCount(player) : 1;
}
void PeerlessLegend::OnTurnStart(GameState& s, Piece p) { if (chainedTurn) chainedTurn->OnTurnStart(s, p); }
void PeerlessLegend::OnTurnEnd  (GameState& s, Piece p) { if (chainedTurn) chainedTurn->OnTurnEnd(s, p); }
bool PeerlessLegend::OnDefeat(GameState& /*state*/, Piece loser)
{
    if (loser != Piece::Player) return false;
    if (charges <= 0)           return false;
    --charges;
    return true;
}
void PeerlessLegend::RegisterTo(AbilityRegistry& registry)
{
    auto self = shared_from_this();
    chainedTurn = registry.SetTurnHandler(std::dynamic_pointer_cast<ITurnHandler>(self));
    registry.AddDefeatHandler(std::dynamic_pointer_cast<IDefeatHandler>(self));
}

//======================================
// 10. 終焉の宣告
//======================================
FinalDecreeLegend::FinalDecreeLegend()
{
    name        = "終焉の宣告";
    description = "盤面が埋まったら\n引分でなく勝利";
    rarity      = Rarity::Legendary;
    unique      = true;
}
bool FinalDecreeLegend::Check(const Board& board, Piece player)
{
    if (chainedWin && chainedWin->Check(board, player)) return true;
    // 自分側: 盤面が満杯になった時点で勝利(引き分けを勝ちに変える)
    if (player == targetSide && board.IsFull()) return true;
    return false;
}
void FinalDecreeLegend::RegisterTo(AbilityRegistry& registry)
{
    auto self = std::dynamic_pointer_cast<IWinCondition>(shared_from_this());
    chainedWin = registry.SetWinCondition(self);
}
