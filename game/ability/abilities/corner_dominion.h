/****************************************
 * @file   corner_dominion.h
 * @brief  四隅の覇 — 盤面の4隅すべてを支配しても勝利になる (エピック)
 * @author Natsume Shidara
 * @date   2026/06/19
 *
 * 勝利条件をチェインし、OR条件として「4隅すべてを自駒で支配」を追加する。
 ****************************************/
#ifndef ABILITY_CORNER_DOMINION_H
#define ABILITY_CORNER_DOMINION_H

#include "ability/ability.h"
#include "ability/hooks.h"

#include <memory>

//======================================
// 四隅の覇アビリティ
//======================================
/**
 * @class  CornerDominionAbility
 * @brief  4隅支配による追加勝利条件を与えるアビリティ
 */
class CornerDominionAbility : public Ability, public IWinCondition
{
public:
    //--------------------------------------
    // メンバ変数
    //--------------------------------------
    Piece targetSide = Piece::Player;   // 効果を適用する陣営

    // 既存の勝利条件をチェインで保持し、OR判定する
    std::shared_ptr<IWinCondition> chained;

    CornerDominionAbility();

    /** @brief 勝利条件判定 (既存条件 OR 4隅支配) */
    bool Check(const Board& board, Piece player) override;
    /** @brief 勝利条件として registry に登録 */
    void RegisterTo(AbilityRegistry& registry) override;
};

#endif // ABILITY_CORNER_DOMINION_H
