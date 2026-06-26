/****************************************
 * @file   curses.h
 * @brief  呪いアビリティ群 (ハイリスク・ハイリターン10種)
 * @author Natsume Shidara
 * @date   2026/06/25
 *
 * 強力な恩恵と引き換えに明確な代償を負う「呪い」レアリティのアビリティ。
 * 報酬3択の中から「敢えて選ぶ」ギャンブル枠。既存フック(時間/着手数/
 * 勝利条件/盤面/敗北/設置)の組合せのみで構成し、エンジン拡張はしない。
 *
 *   1. 修羅の構え   3手/時間×0.4              [Turn + Time]
 *   2. 四隅の渇望   四隅勝利/時間×0.6          [Win + Time]
 *   3. 鏡返しの呪   2手/設置毎に敵が点対称湧き   [Placement + Turn]
 *   4. 時の負債     +150秒/敵が2手             [Time + Turn(enemy)]
 *   5. 逆位置の呪   2手/自分の斜め勝利が無効      [Win + Turn]
 *   6. 諸刃の祭壇   敗北1度無効/時間×0.5         [Defeat + Time]
 *   7. 狂宴         自分も敵も2手               [Turn(both)]
 *   8. 痩せ我慢     3手/思考時間45秒固定         [Time(fixed) + Turn]
 *   9. 混沌の盤     盤面4×4+自他2手             [Board + Turn(both)]
 *  10. 業火の刻     4手/時間×0.25              [Turn + Time]
 ****************************************/
#ifndef ABILITY_CURSES_H
#define ABILITY_CURSES_H

#include "ability/ability.h"
#include "ability/hooks.h"

#include <memory>

//======================================
// 1. 修羅の構え — 3手 打てるが思考時間が4割に
//======================================
class ShuraStanceCurse : public Ability, public ITurnHandler, public ITurnTimeProvider
{
public:
    int    placementCount = 3;     // 付与する着手数
    double timeFactor     = 0.4;   // 思考時間の倍率
    std::shared_ptr<ITurnHandler>      chainedTurn;
    std::shared_ptr<ITurnTimeProvider> chainedTime;

    ShuraStanceCurse();
    int    GetPlacementCount(Piece player) override;
    void   OnTurnStart(GameState& state, Piece player) override;
    void   OnTurnEnd  (GameState& state, Piece player) override;
    double GetTurnTimeSeconds(Piece player) override;
    void   RegisterTo(AbilityRegistry& registry) override;
};

//======================================
// 2. 四隅の渇望 — 四隅支配でも勝てるが思考時間6割に
//======================================
class CornerCravingCurse : public Ability, public IWinCondition, public ITurnTimeProvider
{
public:
    double timeFactor = 0.6;            // 思考時間の倍率
    Piece  targetSide = Piece::Player;
    std::shared_ptr<IWinCondition>     chainedWin;
    std::shared_ptr<ITurnTimeProvider> chainedTime;

    CornerCravingCurse();
    bool   Check(const Board& board, Piece player) override;
    double GetTurnTimeSeconds(Piece player) override;
    void   RegisterTo(AbilityRegistry& registry) override;
};

//======================================
// 3. 鏡返しの呪 — 2手 打てるが設置毎に敵が点対称マスへ湧く
//======================================
class MirrorBackfireCurse : public Ability, public IPlacementHandler, public ITurnHandler
{
public:
    int placementCount = 2;
    std::shared_ptr<ITurnHandler> chainedTurn;

    MirrorBackfireCurse();
    void OnPlace(GameState& state, Vec2 pos, Piece placedBy) override;
    int  GetPlacementCount(Piece player) override;
    void OnTurnStart(GameState& state, Piece player) override;
    void OnTurnEnd  (GameState& state, Piece player) override;
    void RegisterTo(AbilityRegistry& registry) override;
};

//======================================
// 4. 時の負債 — 思考時間+150秒 だが敵が2手 打つ
//======================================
class TimeDebtCurse : public Ability, public ITurnTimeProvider, public ITurnHandler
{
public:
    double bonusSeconds   = 150.0;   // 自分の加算秒数
    int    enemyPlacement = 2;       // 敵の着手数
    std::shared_ptr<ITurnTimeProvider> chainedTime;
    std::shared_ptr<ITurnHandler>      chainedTurn;

    TimeDebtCurse();
    double GetTurnTimeSeconds(Piece player) override;
    int    GetPlacementCount(Piece player) override;
    void   OnTurnStart(GameState& state, Piece player) override;
    void   OnTurnEnd  (GameState& state, Piece player) override;
    void   RegisterTo(AbilityRegistry& registry) override;
};

//======================================
// 5. 逆位置の呪 — 2手 打てるが自分の斜めライン勝利が無効
//======================================
class ReversedCurse : public Ability, public IWinCondition, public ITurnHandler
{
public:
    int   placementCount = 2;
    Piece targetSide     = Piece::Player;   // 斜め無効を受ける側
    std::shared_ptr<IWinCondition> chainedWin;
    std::shared_ptr<ITurnHandler>  chainedTurn;

    ReversedCurse();
    bool Check(const Board& board, Piece player) override;
    int  GetPlacementCount(Piece player) override;
    void OnTurnStart(GameState& state, Piece player) override;
    void OnTurnEnd  (GameState& state, Piece player) override;
    void RegisterTo(AbilityRegistry& registry) override;
};

//======================================
// 6. 諸刃の祭壇 — 敗北を1度だけ防ぐが思考時間が半分
//======================================
class DoubleEdgedAltarCurse : public Ability, public IDefeatHandler, public ITurnTimeProvider
{
public:
    int    charges    = 1;     // 敗北回避の残り回数
    double timeFactor = 0.5;   // 思考時間の倍率
    std::shared_ptr<ITurnTimeProvider> chainedTime;

    DoubleEdgedAltarCurse();
    bool   OnDefeat(GameState& state, Piece loser) override;
    double GetTurnTimeSeconds(Piece player) override;
    int    ChargesLeft() const override { return charges; }
    void   RegisterTo(AbilityRegistry& registry) override;
};

//======================================
// 7. 狂宴 — 自分も敵も1ターン2手 になる
//======================================
class FrenzyCurse : public Ability, public ITurnHandler
{
public:
    int placementCount = 2;   // 両陣営に付与する着手数
    std::shared_ptr<ITurnHandler> chainedTurn;

    FrenzyCurse();
    int  GetPlacementCount(Piece player) override;
    void OnTurnStart(GameState& state, Piece player) override;
    void OnTurnEnd  (GameState& state, Piece player) override;
    void RegisterTo(AbilityRegistry& registry) override;
};

//======================================
// 8. 痩せ我慢 — 3手 打てるが思考時間は45秒固定
//======================================
class BluffCurse : public Ability, public ITurnTimeProvider, public ITurnHandler
{
public:
    double fixedSeconds   = 45.0;   // 固定する思考時間
    int    placementCount = 3;
    Piece  targetSide     = Piece::Player;
    std::shared_ptr<ITurnTimeProvider> chainedTime;
    std::shared_ptr<ITurnHandler>      chainedTurn;

    BluffCurse();
    double GetTurnTimeSeconds(Piece player) override;
    int    GetPlacementCount(Piece player) override;
    void   OnTurnStart(GameState& state, Piece player) override;
    void   OnTurnEnd  (GameState& state, Piece player) override;
    void   RegisterTo(AbilityRegistry& registry) override;
};

//======================================
// 9. 混沌の盤 — 盤面4×4+自他2手 (敵も2手 になる)
//======================================
class ChaosBoardCurse : public Ability, public IBoardModifier, public ITurnHandler
{
public:
    int targetWidth    = 4;
    int targetHeight   = 4;
    int placementCount = 2;   // 両陣営に付与する着手数
    std::shared_ptr<IBoardModifier> chainedBoard;
    std::shared_ptr<ITurnHandler>   chainedTurn;

    ChaosBoardCurse();
    void GetBoardSize(int& width, int& height) override;
    int  GetPlacementCount(Piece player) override;
    void OnTurnStart(GameState& state, Piece player) override;
    void OnTurnEnd  (GameState& state, Piece player) override;
    void RegisterTo(AbilityRegistry& registry) override;
};

//======================================
// 10. 業火の刻 — 4手 打てるが思考時間が×0.25
//======================================
class HellfireHourCurse : public Ability, public ITurnHandler, public ITurnTimeProvider
{
public:
    int    placementCount = 4;
    double timeFactor     = 0.25;
    std::shared_ptr<ITurnHandler>      chainedTurn;
    std::shared_ptr<ITurnTimeProvider> chainedTime;

    HellfireHourCurse();
    int    GetPlacementCount(Piece player) override;
    void   OnTurnStart(GameState& state, Piece player) override;
    void   OnTurnEnd  (GameState& state, Piece player) override;
    double GetTurnTimeSeconds(Piece player) override;
    void   RegisterTo(AbilityRegistry& registry) override;
};

#endif // ABILITY_CURSES_H
