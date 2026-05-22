/****************************************
 * @file   board_expand.h
 * @brief  盤面拡張 — 盤面を 3×3 から 4×4 へ広げる
 * @author Natsume Shidara
 * @date   2026/05/15
 * @update 2026/05/22 - 隅支配勝利(旧ライン本数追加)から盤面拡張へ仕様変更
 *
 * IBoardModifier をチェインし、試合開始時の盤面サイズを拡大する。
 * 勝利条件(3並び)は据え置きのため、勝利機会が大幅に増える。
 ****************************************/
#ifndef ABILITY_BOARD_EXPAND_H
#define ABILITY_BOARD_EXPAND_H

#include "ability/ability.h"
#include "ability/hooks.h"

#include <memory>

//======================================
// 盤面拡張アビリティ
//======================================
/**
 * @class  BoardExpandAbility
 * @brief  盤面サイズを拡大するアビリティ
 * @detail IBoardModifier をチェインし、盤面を 4×4 (既定) へ広げる。
 *         盤面構造カテゴリのエピックアビリティ。
 */
class BoardExpandAbility : public Ability, public IBoardModifier
{
public:
    //--------------------------------------
    // メンバ変数
    //--------------------------------------
    int targetWidth  = 4;  // 拡張後の横マス数
    int targetHeight = 4;  // 拡張後の縦マス数

    // 既存の盤面サイズフックをチェインで保持する
    std::shared_ptr<IBoardModifier> chained;

    BoardExpandAbility();

    //======================================
    // IBoardModifier
    //======================================
    /** @brief 盤面サイズを問い合わせ、拡張後サイズへ広げる */
    void GetBoardSize(int& width, int& height) override;

    //======================================
    // Ability
    //======================================
    /** @brief 盤面サイズフックとして registry に登録 */
    void RegisterTo(AbilityRegistry& registry) override;
};

#endif // ABILITY_BOARD_EXPAND_H
