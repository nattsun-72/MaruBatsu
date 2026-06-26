/****************************************
 * @file   curses.cpp
 * @brief  呪いアビリティ群の実装
 * @author Natsume Shidara
 * @date   2026/06/25
 ****************************************/
#include "curses.h"

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
// 1. 修羅の構え
//======================================
ShuraStanceCurse::ShuraStanceCurse()
{
    name        = "修羅の構え";
    description = "【呪い】1ターン3手\nだが思考時間が4割に";
    rarity      = Rarity::Curse;
    unique      = true;
}
int ShuraStanceCurse::GetPlacementCount(Piece player)
{
    if (player == Piece::Player) return placementCount;
    return chainedTurn ? chainedTurn->GetPlacementCount(player) : 1;
}
void ShuraStanceCurse::OnTurnStart(GameState& s, Piece p) { if (chainedTurn) chainedTurn->OnTurnStart(s, p); }
void ShuraStanceCurse::OnTurnEnd  (GameState& s, Piece p) { if (chainedTurn) chainedTurn->OnTurnEnd(s, p); }
double ShuraStanceCurse::GetTurnTimeSeconds(Piece player)
{
    const double base = chainedTime ? chainedTime->GetTurnTimeSeconds(player) : 180.0;
    return (player == Piece::Player) ? base * timeFactor : base;
}
void ShuraStanceCurse::RegisterTo(AbilityRegistry& registry)
{
    auto self = shared_from_this();
    chainedTurn = registry.SetTurnHandler(std::dynamic_pointer_cast<ITurnHandler>(self));
    chainedTime = registry.SetTurnTimeProvider(std::dynamic_pointer_cast<ITurnTimeProvider>(self));
}

//======================================
// 2. 四隅の渇望
//======================================
CornerCravingCurse::CornerCravingCurse()
{
    name        = "四隅の渇望";
    description = "【呪い】四隅支配でも\n勝てるが時間6割に";
    rarity      = Rarity::Curse;
    unique      = true;
}
bool CornerCravingCurse::Check(const Board& board, Piece player)
{
    if (chainedWin && chainedWin->Check(board, player)) return true;
    if (player != targetSide) return false;
    return OwnsAllCorners(board, player);   // 四隅すべてを自駒で支配
}
double CornerCravingCurse::GetTurnTimeSeconds(Piece player)
{
    const double base = chainedTime ? chainedTime->GetTurnTimeSeconds(player) : 180.0;
    return (player == targetSide) ? base * timeFactor : base;
}
void CornerCravingCurse::RegisterTo(AbilityRegistry& registry)
{
    auto self = shared_from_this();
    chainedWin  = registry.SetWinCondition(std::dynamic_pointer_cast<IWinCondition>(self));
    chainedTime = registry.SetTurnTimeProvider(std::dynamic_pointer_cast<ITurnTimeProvider>(self));
}

//======================================
// 3. 鏡返しの呪
//======================================
MirrorBackfireCurse::MirrorBackfireCurse()
{
    name        = "鏡返しの呪";
    description = "【呪い】2手 打てるが\n設置毎に敵が点対称湧き";
    rarity      = Rarity::Curse;
    unique      = true;
}
void MirrorBackfireCurse::OnPlace(GameState& state, Vec2 pos, Piece placedBy)
{
    if (placedBy != Piece::Player) return;   // 自駒の設置にのみ反応する

    // 盤面中心に対する点対称マスへ、敵駒を1つ出現させる
    const int mx = (state.board.width  - 1) - pos.x;
    const int my = (state.board.height - 1) - pos.y;
    if (mx == pos.x && my == pos.y) return;          // 中央マスは点対称が自身
    if (!state.board.IsEmpty(mx, my)) return;        // 既に埋まっていれば何もしない

    state.board.Set(mx, my, Piece::Enemy);
    state.animPhases.push_back(
        { { pos.x, pos.y, mx, my, Piece::Enemy, true } });   // 出現アニメ
}
int MirrorBackfireCurse::GetPlacementCount(Piece player)
{
    if (player == Piece::Player) return placementCount;
    return chainedTurn ? chainedTurn->GetPlacementCount(player) : 1;
}
void MirrorBackfireCurse::OnTurnStart(GameState& s, Piece p) { if (chainedTurn) chainedTurn->OnTurnStart(s, p); }
void MirrorBackfireCurse::OnTurnEnd  (GameState& s, Piece p) { if (chainedTurn) chainedTurn->OnTurnEnd(s, p); }
void MirrorBackfireCurse::RegisterTo(AbilityRegistry& registry)
{
    auto self = shared_from_this();
    registry.AddPlacementHandler(std::dynamic_pointer_cast<IPlacementHandler>(self));
    chainedTurn = registry.SetTurnHandler(std::dynamic_pointer_cast<ITurnHandler>(self));
}

//======================================
// 4. 時の負債
//======================================
TimeDebtCurse::TimeDebtCurse()
{
    name        = "時の負債";
    description = "【呪い】思考+150秒\nだが敵が2手 打つ";
    rarity      = Rarity::Curse;
    unique      = true;
}
double TimeDebtCurse::GetTurnTimeSeconds(Piece player)
{
    const double base = chainedTime ? chainedTime->GetTurnTimeSeconds(player) : 180.0;
    return (player == Piece::Player) ? base + bonusSeconds : base;
}
int TimeDebtCurse::GetPlacementCount(Piece player)
{
    if (player == Piece::Enemy) return enemyPlacement;     // 敵だけ2手
    return chainedTurn ? chainedTurn->GetPlacementCount(player) : 1;
}
void TimeDebtCurse::OnTurnStart(GameState& s, Piece p) { if (chainedTurn) chainedTurn->OnTurnStart(s, p); }
void TimeDebtCurse::OnTurnEnd  (GameState& s, Piece p) { if (chainedTurn) chainedTurn->OnTurnEnd(s, p); }
void TimeDebtCurse::RegisterTo(AbilityRegistry& registry)
{
    auto self = shared_from_this();
    chainedTime = registry.SetTurnTimeProvider(std::dynamic_pointer_cast<ITurnTimeProvider>(self));
    chainedTurn = registry.SetTurnHandler(std::dynamic_pointer_cast<ITurnHandler>(self));
}

//======================================
// 5. 逆位置の呪
//======================================
ReversedCurse::ReversedCurse()
{
    name        = "逆位置の呪";
    description = "【呪い】2手 打てるが\n自分の斜めが無効";
    rarity      = Rarity::Curse;
    unique      = true;
}
bool ReversedCurse::Check(const Board& board, Piece player)
{
    if (player == targetSide)
    {
        // 自分側: 斜めを除いた縦横のみの並びでしか勝てない
        return WinCheck::HasLine(board, player, 3, false);
    }
    return chainedWin ? chainedWin->Check(board, player) : false;
}
int ReversedCurse::GetPlacementCount(Piece player)
{
    if (player == Piece::Player) return placementCount;
    return chainedTurn ? chainedTurn->GetPlacementCount(player) : 1;
}
void ReversedCurse::OnTurnStart(GameState& s, Piece p) { if (chainedTurn) chainedTurn->OnTurnStart(s, p); }
void ReversedCurse::OnTurnEnd  (GameState& s, Piece p) { if (chainedTurn) chainedTurn->OnTurnEnd(s, p); }
void ReversedCurse::RegisterTo(AbilityRegistry& registry)
{
    auto self = shared_from_this();
    chainedWin  = registry.SetWinCondition(std::dynamic_pointer_cast<IWinCondition>(self));
    chainedTurn = registry.SetTurnHandler(std::dynamic_pointer_cast<ITurnHandler>(self));
}

//======================================
// 6. 諸刃の祭壇
//======================================
DoubleEdgedAltarCurse::DoubleEdgedAltarCurse()
{
    name        = "諸刃の祭壇";
    description = "【呪い】敗北を1度防ぐ\nが思考時間は半分";
    rarity      = Rarity::Curse;
    unique      = true;
}
bool DoubleEdgedAltarCurse::OnDefeat(GameState& /*state*/, Piece loser)
{
    if (loser != Piece::Player) return false;
    if (charges <= 0)           return false;
    --charges;
    return true;
}
double DoubleEdgedAltarCurse::GetTurnTimeSeconds(Piece player)
{
    const double base = chainedTime ? chainedTime->GetTurnTimeSeconds(player) : 180.0;
    return (player == Piece::Player) ? base * timeFactor : base;
}
void DoubleEdgedAltarCurse::RegisterTo(AbilityRegistry& registry)
{
    auto self = shared_from_this();
    registry.AddDefeatHandler(std::dynamic_pointer_cast<IDefeatHandler>(self));
    chainedTime = registry.SetTurnTimeProvider(std::dynamic_pointer_cast<ITurnTimeProvider>(self));
}

//======================================
// 7. 狂宴
//======================================
FrenzyCurse::FrenzyCurse()
{
    name        = "狂宴";
    description = "【呪い】自分も敵も\n1ターン2手 になる";
    rarity      = Rarity::Curse;
    unique      = true;
}
int FrenzyCurse::GetPlacementCount(Piece player)
{
    if (player == Piece::Player || player == Piece::Enemy) return placementCount;
    return chainedTurn ? chainedTurn->GetPlacementCount(player) : 1;
}
void FrenzyCurse::OnTurnStart(GameState& s, Piece p) { if (chainedTurn) chainedTurn->OnTurnStart(s, p); }
void FrenzyCurse::OnTurnEnd  (GameState& s, Piece p) { if (chainedTurn) chainedTurn->OnTurnEnd(s, p); }
void FrenzyCurse::RegisterTo(AbilityRegistry& registry)
{
    auto self = std::dynamic_pointer_cast<ITurnHandler>(shared_from_this());
    chainedTurn = registry.SetTurnHandler(self);
}

//======================================
// 8. 痩せ我慢
//======================================
BluffCurse::BluffCurse()
{
    name        = "痩せ我慢";
    description = "【呪い】3手 打てるが\n思考時間は45秒固定";
    rarity      = Rarity::Curse;
    unique      = true;
}
double BluffCurse::GetTurnTimeSeconds(Piece player)
{
    if (player == targetSide) return fixedSeconds;   // 固定値で上書き(チェイン無視)
    return chainedTime ? chainedTime->GetTurnTimeSeconds(player) : 180.0;
}
int BluffCurse::GetPlacementCount(Piece player)
{
    if (player == Piece::Player) return placementCount;
    return chainedTurn ? chainedTurn->GetPlacementCount(player) : 1;
}
void BluffCurse::OnTurnStart(GameState& s, Piece p) { if (chainedTurn) chainedTurn->OnTurnStart(s, p); }
void BluffCurse::OnTurnEnd  (GameState& s, Piece p) { if (chainedTurn) chainedTurn->OnTurnEnd(s, p); }
void BluffCurse::RegisterTo(AbilityRegistry& registry)
{
    auto self = shared_from_this();
    chainedTime = registry.SetTurnTimeProvider(std::dynamic_pointer_cast<ITurnTimeProvider>(self));
    chainedTurn = registry.SetTurnHandler(std::dynamic_pointer_cast<ITurnHandler>(self));
}

//======================================
// 9. 混沌の盤
//======================================
ChaosBoardCurse::ChaosBoardCurse()
{
    name        = "混沌の盤";
    description = "【呪い】盤面4×4+2手\nだが敵も2手 になる";
    rarity      = Rarity::Curse;
    unique      = true;
}
void ChaosBoardCurse::GetBoardSize(int& width, int& height)
{
    if (chainedBoard) chainedBoard->GetBoardSize(width, height);
    if (targetWidth  > width)  width  = targetWidth;    // 縮小はしない
    if (targetHeight > height) height = targetHeight;
}
int ChaosBoardCurse::GetPlacementCount(Piece player)
{
    if (player == Piece::Player || player == Piece::Enemy) return placementCount;
    return chainedTurn ? chainedTurn->GetPlacementCount(player) : 1;
}
void ChaosBoardCurse::OnTurnStart(GameState& s, Piece p) { if (chainedTurn) chainedTurn->OnTurnStart(s, p); }
void ChaosBoardCurse::OnTurnEnd  (GameState& s, Piece p) { if (chainedTurn) chainedTurn->OnTurnEnd(s, p); }
void ChaosBoardCurse::RegisterTo(AbilityRegistry& registry)
{
    auto self = shared_from_this();
    chainedBoard = registry.SetBoardModifier(std::dynamic_pointer_cast<IBoardModifier>(self));
    chainedTurn  = registry.SetTurnHandler(std::dynamic_pointer_cast<ITurnHandler>(self));
}

//======================================
// 10. 業火の刻
//======================================
HellfireHourCurse::HellfireHourCurse()
{
    name        = "業火の刻";
    description = "【呪い】4手 打てるが\n思考時間が4分の1に";
    rarity      = Rarity::Curse;
    unique      = true;
}
int HellfireHourCurse::GetPlacementCount(Piece player)
{
    if (player == Piece::Player) return placementCount;
    return chainedTurn ? chainedTurn->GetPlacementCount(player) : 1;
}
void HellfireHourCurse::OnTurnStart(GameState& s, Piece p) { if (chainedTurn) chainedTurn->OnTurnStart(s, p); }
void HellfireHourCurse::OnTurnEnd  (GameState& s, Piece p) { if (chainedTurn) chainedTurn->OnTurnEnd(s, p); }
double HellfireHourCurse::GetTurnTimeSeconds(Piece player)
{
    const double base = chainedTime ? chainedTime->GetTurnTimeSeconds(player) : 180.0;
    return (player == Piece::Player) ? base * timeFactor : base;
}
void HellfireHourCurse::RegisterTo(AbilityRegistry& registry)
{
    auto self = shared_from_this();
    chainedTurn = registry.SetTurnHandler(std::dynamic_pointer_cast<ITurnHandler>(self));
    chainedTime = registry.SetTurnTimeProvider(std::dynamic_pointer_cast<ITurnTimeProvider>(self));
}
