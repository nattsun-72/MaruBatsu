/****************************************
 * @file   l_win.h
 * @brief  L字勝利 — 2×2ブロックの3マス支配(L字)でも勝利になる
 * @author Natsume Shidara
 * @date   2026/06/11
 *
 * 勝利条件をチェインし、OR条件として「いずれかの2×2ブロックのうち
 * 3マス以上を自駒で支配(=L字配置)」を追加する。
 ****************************************/
#ifndef ABILITY_L_WIN_H
#define ABILITY_L_WIN_H

#include "ability/ability.h"
#include "ability/hooks.h"

#include <memory>

//======================================
// L字勝利アビリティ
//======================================
/**
 * @class  LWinAbility
 * @brief  L字配置による追加勝利条件を与えるアビリティ
 * @detail 既存の勝利条件をチェインし、OR条件としてL字判定を足す。
 */
class LWinAbility : public Ability, public IWinCondition
{
public:
    //--------------------------------------
    // メンバ変数
    //--------------------------------------
    Piece targetSide = Piece::Player;   // 効果を適用する陣営

    // 既存の勝利条件をチェインで保持し、OR判定する
    std::shared_ptr<IWinCondition> chained;

    LWinAbility();

    /** @brief 勝利条件判定 (既存条件 OR L字配置) */
    bool Check(const Board& board, Piece player) override;
    /** @brief 勝利条件として registry に登録 */
    void RegisterTo(AbilityRegistry& registry) override;
};

#endif // ABILITY_L_WIN_H
