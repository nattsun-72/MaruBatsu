/****************************************
 * @file   rares.h
 * @brief  レアアビリティ群 (ビルドの方向性を決める10種)
 * @author Natsume Shidara
 * @date   2026/06/25
 *
 * 別の勝ち筋・任意発動の盤面操作・設置効果など、立ち回りを変える中堅枠。
 * 既存フックの組合せのみで構成する。
 *
 *   1. 四角の覇   2×2ブロック支配でも勝利       [Win]
 *   2. 奔流       任意発動 全駒を右へ寄せる(×2)   [Activatable]
 *   3. 布陣の妙   四辺の中点をすべて支配で勝利     [Win]
 *   4. 雪崩       任意発動 全駒を下へ落とす(×2)   [Activatable]
 *   5. 旋風       任意発動 盤面を反時計回り回転(×2) [Activatable]
 *   6. 求心       設置した駒が中央へ1マス寄る      [Placement]
 *   7. 不屈の魂   敗北を1度 打ち消す             [Defeat]
 *   8. 快進撃     1ターン2手 + 思考時間+30秒      [Turn + Time]
 *   9. 先見の明   任意発動 残時間を150秒へ補充(×2) [Focus派生]
 *  10. 増援       任意発動 このターンの着手を+1(×2) [Activatable]
 ****************************************/
#ifndef ABILITY_RARES_H
#define ABILITY_RARES_H

#include "ability/ability.h"
#include "ability/hooks.h"
#include "ability/abilities/focus.h"

#include <memory>

//======================================
// 1. 四角の覇 — 2×2ブロック支配でも勝利
//======================================
class SquareSovereignRare : public Ability, public IWinCondition
{
public:
    Piece targetSide = Piece::Player;
    std::shared_ptr<IWinCondition> chainedWin;

    SquareSovereignRare();
    bool Check(const Board& board, Piece player) override;
    void RegisterTo(AbilityRegistry& registry) override;
};

//======================================
// 2. 奔流 — 任意発動: 全駒を右へ寄せる (雪崩の横方向版)
//======================================
class TorrentRare : public Ability
{
public:
    int charges = 2;

    TorrentRare();
    void Activate(GameState& state) override;
    bool CanActivate(const GameState& state) const override;
    int  ChargesLeft() const override { return charges; }
    void RegisterTo(AbilityRegistry& /*registry*/) override {}
};

//======================================
// 3. 布陣の妙 — 四辺の中点をすべて支配で勝利
//======================================
class EdgeFormationRare : public Ability, public IWinCondition
{
public:
    Piece targetSide = Piece::Player;
    std::shared_ptr<IWinCondition> chainedWin;

    EdgeFormationRare();
    bool Check(const Board& board, Piece player) override;
    void RegisterTo(AbilityRegistry& registry) override;
};

//======================================
// 4. 雪崩 — 任意発動: 全駒を下へ落とす
//======================================
class AvalancheRare : public Ability
{
public:
    int charges = 2;

    AvalancheRare();
    void Activate(GameState& state) override;
    bool CanActivate(const GameState& state) const override;
    int  ChargesLeft() const override { return charges; }
    void RegisterTo(AbilityRegistry& /*registry*/) override {}
};

//======================================
// 5. 旋風 — 任意発動: 盤面を反時計回りに90度回転
//======================================
class WhirlwindRare : public Ability
{
public:
    int charges = 2;

    WhirlwindRare();
    void Activate(GameState& state) override;
    bool CanActivate(const GameState& state) const override;
    int  ChargesLeft() const override { return charges; }
    void RegisterTo(AbilityRegistry& /*registry*/) override {}
};

//======================================
// 6. 求心 — 設置した自駒が中央へ1マス寄る
//======================================
class CentripetalRare : public Ability, public IPlacementHandler
{
public:
    CentripetalRare();
    void OnPlace(GameState& state, Vec2 pos, Piece placedBy) override;
    void RegisterTo(AbilityRegistry& registry) override;
};

//======================================
// 7. 不屈の魂 — 敗北を1度 打ち消す
//======================================
class UnyieldingRare : public Ability, public IDefeatHandler
{
public:
    int charges = 1;

    UnyieldingRare();
    bool OnDefeat(GameState& state, Piece loser) override;
    int  ChargesLeft() const override { return charges; }
    void RegisterTo(AbilityRegistry& registry) override;
};

//======================================
// 8. 快進撃 — 1ターン2手 + 思考時間+30秒
//======================================
class OnslaughtRare : public Ability, public ITurnHandler, public ITurnTimeProvider
{
public:
    int    placementCount = 2;
    double bonusSeconds   = 30.0;
    std::shared_ptr<ITurnHandler>      chainedTurn;
    std::shared_ptr<ITurnTimeProvider> chainedTime;

    OnslaughtRare();
    int    GetPlacementCount(Piece player) override;
    void   OnTurnStart(GameState& state, Piece player) override;
    void   OnTurnEnd  (GameState& state, Piece player) override;
    double GetTurnTimeSeconds(Piece player) override;
    void   RegisterTo(AbilityRegistry& registry) override;
};

//======================================
// 9. 先見の明 — 任意発動: 残り時間を floor まで補充 (集中派生)
//======================================
class ForesightRare : public FocusAbility
{
public:
    double floorSeconds = 150.0;
    ForesightRare();
    void Activate(GameState& state) override;
    bool CanActivate(const GameState& state) const override;
};

//======================================
// 10. 増援 — 任意発動: このターンの着手を+1する
//======================================
class ReinforcementRare : public Ability
{
public:
    int charges = 2;

    ReinforcementRare();
    void Activate(GameState& state) override;
    bool CanActivate(const GameState& state) const override;
    int  ChargesLeft() const override { return charges; }
    void RegisterTo(AbilityRegistry& /*registry*/) override {}
};

#endif // ABILITY_RARES_H
