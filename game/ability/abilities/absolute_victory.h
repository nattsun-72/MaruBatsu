/****************************************
 * @file   absolute_victory.h
 * @brief  絶対勝利 — ライン達成時に強制勝利するレジェンダリー
 * @author Natsume Shidara
 * @date   2026/06/19
 *
 * 勝利条件をチェインし、プレイヤーが標準の3並びを達成していれば、
 * 他の勝利条件改変の影響を受けずに必ず勝利と判定する。
 * 「勝利条件改変系デメリットの打ち消し」(企画書 2.4) の役割を担う。
 ****************************************/
#ifndef ABILITY_ABSOLUTE_VICTORY_H
#define ABILITY_ABSOLUTE_VICTORY_H

#include "ability/ability.h"
#include "ability/hooks.h"

#include <memory>

//======================================
// 絶対勝利アビリティ
//======================================
/**
 * @class  AbsoluteVictoryAbility
 * @brief  標準ラインの達成を無条件で勝利に確定させるアビリティ
 */
class AbsoluteVictoryAbility : public Ability, public IWinCondition
{
public:
    //--------------------------------------
    // メンバ変数
    //--------------------------------------
    Piece targetSide = Piece::Player;   // 効果を適用する陣営

    // 既存の勝利条件をチェインで保持する
    std::shared_ptr<IWinCondition> chained;

    AbsoluteVictoryAbility();

    /** @brief 勝利条件判定 (自陣の標準3並びを最優先で勝利に確定) */
    bool Check(const Board& board, Piece player) override;
    /** @brief 勝利条件として registry に登録 */
    void RegisterTo(AbilityRegistry& registry) override;
};

#endif // ABILITY_ABSOLUTE_VICTORY_H
