/****************************************
 * @file   diagonal_void.h
 * @brief  対角線無効 — 相手の斜めラインを勝利として認めない
 * @author Natsume Shidara
 * @date   2026/06/11
 *
 * 勝利条件をチェインし、相手(ボス)側の判定だけを
 * 「縦横のみの並び判定」へ差し替える防御系アビリティ。
 ****************************************/
#ifndef ABILITY_DIAGONAL_VOID_H
#define ABILITY_DIAGONAL_VOID_H

#include "ability/ability.h"
#include "ability/hooks.h"

#include <memory>

//======================================
// 対角線無効アビリティ
//======================================
/**
 * @class  DiagonalVoidAbility
 * @brief  相手の斜めライン勝利を打ち消すアビリティ
 * @detail 自分側の判定はチェイン先へ委譲し、相手側のみ
 *         斜めを除いた並び判定で評価する。
 */
class DiagonalVoidAbility : public Ability, public IWinCondition
{
public:
    //--------------------------------------
    // メンバ変数
    //--------------------------------------
    Piece protectedSide = Piece::Player;   // 守られる側 (相手の斜めを無効化)

    // 既存の勝利条件をチェインで保持する
    std::shared_ptr<IWinCondition> chained;

    DiagonalVoidAbility();

    /** @brief 勝利条件判定 (相手側のみ斜め無効の並び判定) */
    bool Check(const Board& board, Piece player) override;
    /** @brief 勝利条件として registry に登録 */
    void RegisterTo(AbilityRegistry& registry) override;
};

#endif // ABILITY_DIAGONAL_VOID_H
