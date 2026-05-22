/****************************************
 * @file hooks.h
 * @brief フックインターフェイス群
 * @author Natsume Shidara
 * @date 2026/05/15
 *
 * 本プロジェクトの根幹設計。ゲームルール自体を差し替え可能に
 * するための抽象。各アビリティは複数のフックを実装する。
 *
 * 一覧:
 *   IWinCondition    勝利条件の判定
 *   IPlacementHandler 駒設置時/設置後イベント
 *   ITurnHandler     ターン処理 (1ターンあたりの着手数、ターン開始/終了)
 *   ITurnTimeProvider 思考時間の決定
 *   IDefeatHandler   敗北処理 (return true で敗北回避)
 ****************************************/
#ifndef ABILITY_HOOKS_H
#define ABILITY_HOOKS_H

#include "piece.h"

class Board;
struct GameState;

/**
 * @brief 勝利条件
 *
 * 着手後に評価され、true を返したプレイヤーが勝利する。
 */
class IWinCondition
{
public:
    virtual ~IWinCondition() = default;
    virtual bool Check(const Board& board, Piece player) = 0;
};

/**
 * @brief 駒設置時のイベント
 *
 * 設置直後に呼ばれる。盤面を書き換えたり追加駒を置いたりできる
 * (例: 氷駒で1マス滑らせる、連鎖駒で隣接マスに自動配置)。
 */
class IPlacementHandler
{
public:
    virtual ~IPlacementHandler() = default;
    virtual void OnPlace(GameState& state, Vec2 pos, Piece placedBy) = 0;
};

/**
 * @brief ターン処理
 *
 * GetPlacementCount: 1ターンに置ける駒数 (例: 二手打ちで2)
 * OnTurnStart/End: ターン境界での状態更新フック
 */
class ITurnHandler
{
public:
    virtual ~ITurnHandler() = default;
    virtual int  GetPlacementCount(Piece player) = 0;
    virtual void OnTurnStart(GameState& state, Piece player) = 0;
    virtual void OnTurnEnd  (GameState& state, Piece player) = 0;
};

/**
 * @brief 思考時間プロバイダ
 *
 * 各プレイヤーのターン開始時に呼び出される。
 * 例: 「熟考」は +30s、「焦燥」は -90s などをここで反映する。
 */
class ITurnTimeProvider
{
public:
    virtual ~ITurnTimeProvider() = default;
    virtual double GetTurnTimeSeconds(Piece player) = 0;
};

/**
 * @brief 敗北処理
 *
 * 敗北発生時 (時間切れ・勝利条件達成等) に呼ばれる。
 * true を返すと敗北を打ち消す (例: 「不死」「復活の意志」)。
 */
class IDefeatHandler
{
public:
    virtual ~IDefeatHandler() = default;
    virtual bool OnDefeat(GameState& state, Piece loser) = 0;
};

#endif // ABILITY_HOOKS_H
