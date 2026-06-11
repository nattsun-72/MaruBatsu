/****************************************
 * @file   immortal.h
 * @brief  不死 — 敗北を打ち消すレジェンダリーアビリティ
 * @author Natsume Shidara
 * @date   2026/06/11
 *
 * 敗北(時間切れ・相手のライン完成)が発生したとき、チャージを消費して
 * 打ち消す。時間切れは残り時間が回復し、ライン敗北は盤面が再生する
 * (打ち消し方はゲームシーン側の敗北フローが担当)。
 * チャージはラン全体で共有される。
 ****************************************/
#ifndef ABILITY_IMMORTAL_H
#define ABILITY_IMMORTAL_H

#include "ability/ability.h"
#include "ability/hooks.h"

//======================================
// 不死アビリティ
//======================================
/**
 * @class  ImmortalAbility
 * @brief  敗北をラン中 charges 回まで無効化するアビリティ
 * @detail IDefeatHandler を実装。OnDefeat でチャージを消費して
 *         true(敗北回避)を返す。
 */
class ImmortalAbility : public Ability, public IDefeatHandler
{
public:
    //--------------------------------------
    // メンバ変数
    //--------------------------------------
    int charges = 2;   // 残り回避回数 (ラン全体で共有)

    ImmortalAbility();

    //======================================
    // IDefeatHandler
    //======================================
    /** @brief 敗北発生時、チャージがあれば消費して打ち消す */
    bool OnDefeat(GameState& state, Piece loser) override;

    //======================================
    // Ability
    //======================================
    /** @brief 残り回避回数を返す (HUDの回数表示用) */
    int  ChargesLeft() const override { return charges; }
    /** @brief 敗北ハンドラとして registry に登録 */
    void RegisterTo(AbilityRegistry& registry) override;
};

#endif // ABILITY_IMMORTAL_H
