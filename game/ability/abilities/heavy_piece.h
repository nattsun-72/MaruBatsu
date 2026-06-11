/****************************************
 * @file   heavy_piece.h
 * @brief  重駒 — 自駒が滑り・落下の影響を受けなくなる (ボス4撃破報酬)
 * @author Natsume Shidara
 * @date   2026/06/11
 *
 * 重力の専制者のギミックの弱化版。自分の駒が「重く」なり、
 * 氷盤の全駒スライド・氷駒の滑走・重力落下の対象外になる。
 * (盤面全体が回る回転の影響だけは受ける)
 ****************************************/
#ifndef ABILITY_HEAVY_PIECE_H
#define ABILITY_HEAVY_PIECE_H

#include "ability/ability.h"
#include "ability/hooks.h"

#include <memory>

//======================================
// 重駒アビリティ
//======================================
/**
 * @class  HeavyPieceAbility
 * @brief  自駒をスライド系効果の対象外にするアビリティ
 * @detail ITurnHandler をチェインし、毎ターン開始時に
 *         GameState::slideAnchorSide を Player に設定する。
 *         スライド処理(BoardOps::SlideAll等)がこの値を参照して
 *         対象駒を固定する。
 */
class HeavyPieceAbility : public Ability, public ITurnHandler
{
public:
    //--------------------------------------
    // メンバ変数
    //--------------------------------------
    // 旧 ITurnHandler をチェインで保持し、委譲する
    std::shared_ptr<ITurnHandler> chained;

    HeavyPieceAbility();

    //======================================
    // ITurnHandler
    //======================================
    /** @brief 1ターンの着手数 (チェイン先へ委譲) */
    int  GetPlacementCount(Piece player) override;
    /** @brief ターン開始時、自駒の固定フラグを立てる */
    void OnTurnStart(GameState& state, Piece player) override;
    /** @brief ターン終了時 (チェイン先へ委譲) */
    void OnTurnEnd  (GameState& state, Piece player) override;

    //======================================
    // Ability
    //======================================
    /** @brief ターン処理として registry に登録 */
    void RegisterTo(AbilityRegistry& registry) override;
};

#endif // ABILITY_HEAVY_PIECE_H
