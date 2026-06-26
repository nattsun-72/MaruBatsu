/****************************************
 * @file   legendaries.h
 * @brief  レジェンダリーアビリティ群 (ビルドを決定づける10種)
 * @author Natsume Shidara
 * @date   2026/06/25
 *
 * ラスボス初撃破で解放される最高レアリティ。強力でほぼ恩恵のみ。
 * 既存フックの組合せのみで構成する。
 *
 *   1. 神速         1ターン3手                  [Turn]
 *   2. 大局観       毎ターン+300秒               [Time]
 *   3. 絶対防壁     敵は4並びでないと勝てない      [Win(enemy)]
 *   4. 永劫回帰     敗北を3度 防ぐ               [Defeat]
 *   5. 二重思考     2手 + 毎ターン+120秒         [Turn + Time]
 *   6. 創世         盤面が5×5に広がる            [Board]
 *   7. 刻印の祝福   思考時間が2倍                [Time(×2)]
 *   8. 王の威光     四隅でも勝利 + 毎ターン+120秒 [Win + Time]
 *   9. 無双         3手 + 敗北を1度 防ぐ         [Turn + Defeat]
 *  10. 終焉の宣告   盤面が埋まれば引分でなく勝利    [Win(board-full)]
 ****************************************/
#ifndef ABILITY_LEGENDARIES_H
#define ABILITY_LEGENDARIES_H

#include "ability/ability.h"
#include "ability/hooks.h"

#include <memory>

//======================================
// 1. 神速 — 1ターン3手
//======================================
class GodspeedLegend : public Ability, public ITurnHandler
{
public:
    int placementCount = 3;
    std::shared_ptr<ITurnHandler> chainedTurn;

    GodspeedLegend();
    int  GetPlacementCount(Piece player) override;
    void OnTurnStart(GameState& state, Piece player) override;
    void OnTurnEnd  (GameState& state, Piece player) override;
    void RegisterTo(AbilityRegistry& registry) override;
};

//======================================
// 2. 大局観 — 毎ターン+300秒
//======================================
class GrandVisionLegend : public Ability, public ITurnTimeProvider
{
public:
    double bonusSeconds = 300.0;
    Piece  targetSide   = Piece::Player;
    std::shared_ptr<ITurnTimeProvider> chainedTime;

    GrandVisionLegend();
    double GetTurnTimeSeconds(Piece player) override;
    void   RegisterTo(AbilityRegistry& registry) override;
};

//======================================
// 3. 絶対防壁 — 敵は4並びでないと勝てない
//======================================
class AbsoluteBarrierLegend : public Ability, public IWinCondition
{
public:
    int   enemyRequired = 4;               // 敵の勝利に必要な連続数
    Piece protectedSide = Piece::Player;   // 守られる側
    std::shared_ptr<IWinCondition> chainedWin;

    AbsoluteBarrierLegend();
    bool Check(const Board& board, Piece player) override;
    void RegisterTo(AbilityRegistry& registry) override;
};

//======================================
// 4. 永劫回帰 — 敗北を3度 防ぐ
//======================================
class EternalReturnLegend : public Ability, public IDefeatHandler
{
public:
    int charges = 3;

    EternalReturnLegend();
    bool OnDefeat(GameState& state, Piece loser) override;
    int  ChargesLeft() const override { return charges; }
    void RegisterTo(AbilityRegistry& registry) override;
};

//======================================
// 5. 二重思考 — 2手 + 毎ターン+120秒
//======================================
class DoubleThinkLegend : public Ability, public ITurnHandler, public ITurnTimeProvider
{
public:
    int    placementCount = 2;
    double bonusSeconds   = 120.0;
    std::shared_ptr<ITurnHandler>      chainedTurn;
    std::shared_ptr<ITurnTimeProvider> chainedTime;

    DoubleThinkLegend();
    int    GetPlacementCount(Piece player) override;
    void   OnTurnStart(GameState& state, Piece player) override;
    void   OnTurnEnd  (GameState& state, Piece player) override;
    double GetTurnTimeSeconds(Piece player) override;
    void   RegisterTo(AbilityRegistry& registry) override;
};

//======================================
// 6. 創世 — 盤面が5×5に広がる
//======================================
class GenesisLegend : public Ability, public IBoardModifier
{
public:
    int targetWidth  = 5;
    int targetHeight = 5;
    std::shared_ptr<IBoardModifier> chainedBoard;

    GenesisLegend();
    void GetBoardSize(int& width, int& height) override;
    void RegisterTo(AbilityRegistry& registry) override;
};

//======================================
// 7. 刻印の祝福 — 思考時間が2倍
//======================================
class EngravedBlessingLegend : public Ability, public ITurnTimeProvider
{
public:
    double timeFactor = 2.0;
    Piece  targetSide = Piece::Player;
    std::shared_ptr<ITurnTimeProvider> chainedTime;

    EngravedBlessingLegend();
    double GetTurnTimeSeconds(Piece player) override;
    void   RegisterTo(AbilityRegistry& registry) override;
};

//======================================
// 8. 王の威光 — 四隅でも勝利 + 毎ターン+120秒
//======================================
class KingsMajestyLegend : public Ability, public IWinCondition, public ITurnTimeProvider
{
public:
    double bonusSeconds = 120.0;
    Piece  targetSide   = Piece::Player;
    std::shared_ptr<IWinCondition>     chainedWin;
    std::shared_ptr<ITurnTimeProvider> chainedTime;

    KingsMajestyLegend();
    bool   Check(const Board& board, Piece player) override;
    double GetTurnTimeSeconds(Piece player) override;
    void   RegisterTo(AbilityRegistry& registry) override;
};

//======================================
// 9. 無双 — 3手 + 敗北を1度 防ぐ
//======================================
class PeerlessLegend : public Ability, public ITurnHandler, public IDefeatHandler
{
public:
    int placementCount = 3;
    int charges        = 1;
    std::shared_ptr<ITurnHandler> chainedTurn;

    PeerlessLegend();
    int  GetPlacementCount(Piece player) override;
    void OnTurnStart(GameState& state, Piece player) override;
    void OnTurnEnd  (GameState& state, Piece player) override;
    bool OnDefeat(GameState& state, Piece loser) override;
    int  ChargesLeft() const override { return charges; }
    void RegisterTo(AbilityRegistry& registry) override;
};

//======================================
// 10. 終焉の宣告 — 盤面が埋まれば引分でなく勝利
//======================================
class FinalDecreeLegend : public Ability, public IWinCondition
{
public:
    Piece targetSide = Piece::Player;
    std::shared_ptr<IWinCondition> chainedWin;

    FinalDecreeLegend();
    bool Check(const Board& board, Piece player) override;
    void RegisterTo(AbilityRegistry& registry) override;
};

#endif // ABILITY_LEGENDARIES_H
