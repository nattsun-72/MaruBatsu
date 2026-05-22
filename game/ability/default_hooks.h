/****************************************
 * @file   default_hooks.h
 * @brief  標準フック実装 (3並び勝利 / 1ターン1着手 / 3分タイマー / 3×3盤面)
 * @author Natsume Shidara
 * @date   2026/05/15
 *
 * アビリティ未取得時の既定ルール。AbilityRegistry の初期状態を成す。
 ****************************************/
#ifndef DEFAULT_HOOKS_H
#define DEFAULT_HOOKS_H

#include "hooks.h"

//======================================
// 標準勝利条件
//======================================
/**
 * @class  DefaultWinCondition
 * @brief  標準の勝利条件 (縦/横/斜めの required 並び)
 */
class DefaultWinCondition : public IWinCondition
{
public:
    int required = 3;  // 勝利に必要な連続数
    bool Check(const Board& board, Piece player) override;
};

//======================================
// 標準ターン処理
//======================================
/**
 * @class  DefaultTurnHandler
 * @brief  標準のターン処理 (1ターン1着手、開始/終了処理なし)
 */
class DefaultTurnHandler : public ITurnHandler
{
public:
    int placementCount = 1;  // 1ターンに置ける駒数
    int  GetPlacementCount(Piece /*player*/) override { return placementCount; }
    void OnTurnStart(GameState& /*state*/, Piece /*player*/) override {}
    void OnTurnEnd  (GameState& /*state*/, Piece /*player*/) override {}
};

//======================================
// 標準ターン時間
//======================================
/**
 * @class  DefaultTurnTimeProvider
 * @brief  標準のターン制限時間 (3分)
 */
class DefaultTurnTimeProvider : public ITurnTimeProvider
{
public:
    double seconds = 180.0;  // ターン制限時間(秒)
    double GetTurnTimeSeconds(Piece /*player*/) override { return seconds; }
};

//======================================
// 標準盤面サイズ
//======================================
/**
 * @class  DefaultBoardModifier
 * @brief  標準の盤面サイズ (3×3)
 */
class DefaultBoardModifier : public IBoardModifier
{
public:
    int width  = 3;  // 既定の横マス数
    int height = 3;  // 既定の縦マス数
    void GetBoardSize(int& w, int& h) override { w = width; h = height; }
};

#endif // DEFAULT_HOOKS_H
