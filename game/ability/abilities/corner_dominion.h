/****************************************
 * @file   corner_dominion.h
 * @brief  隅取り — 盤面の隅を一定数支配しても勝利になる (エピック)
 * @author Natsume Shidara
 * @date   2026/06/19
 * @update 2026/06/19 - 4隅固定→しきい値式(既定3)へ。3×3で死にアビリティ化を解消
 *
 * 勝利条件をチェインし、OR条件として「隅を cornerThreshold 個以上 自駒で支配」
 * を追加する。4隅固定だと3×3盤で3並びが先行成立して発火せず死にアビリティに
 * なるため、達成可能なしきい値(既定3)とし、JSONで調整可能にした。
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
 * @brief  隅の一定数支配による追加勝利条件を与えるアビリティ
 */
class CornerDominionAbility : public Ability, public IWinCondition
{
public:
    //--------------------------------------
    // メンバ変数
    //--------------------------------------
    int   cornerThreshold = 3;          // 勝利に必要な支配コーナー数 (JSON調整可)
    Piece targetSide      = Piece::Player;   // 効果を適用する陣営

    // 既存の勝利条件をチェインで保持し、OR判定する
    std::shared_ptr<IWinCondition> chained;

    CornerDominionAbility();

    /** @brief 勝利条件判定 (既存条件 OR 4隅支配) */
    bool Check(const Board& board, Piece player) override;
    /** @brief 勝利条件として registry に登録 */
    void RegisterTo(AbilityRegistry& registry) override;
};

#endif // ABILITY_CORNER_DOMINION_H
