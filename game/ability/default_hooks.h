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
#include "config.h"

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
    // ターン制限時間(秒)。設定ファイルから読み込む。
    double seconds = Config::GetDouble("rules.turnTimeSeconds", 180.0);
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
    // 既定の盤面サイズ。設定ファイルから読み込む。
    int width  = Config::GetInt("rules.boardCols", 3);
    int height = Config::GetInt("rules.boardRows", 3);
    void GetBoardSize(int& w, int& h) override { w = width; h = height; }
};

#endif // DEFAULT_HOOKS_H
