/****************************************
 * @file   hooks.h
 * @brief  フックインターフェイス群 (ゲームルールの差し替え点)
 * @author Natsume Shidara
 * @date   2026/05/15
 *
 * 本プロジェクトの根幹設計。ゲームルール自体を「フック」として
 * 抽象化し、アビリティで差し替え可能にする。各アビリティは
 * これらのインターフェイスを実装してルールを書き換える。
 *
 * 一覧:
 *   IWinCondition     勝利条件の判定
 *   IPlacementHandler 駒設置時/設置後イベント
 *   ITurnHandler      ターン処理 (1ターンの着手数、ターン開始/終了)
 *   ITurnTimeProvider 思考時間の決定
 *   IDefeatHandler    敗北処理 (return true で敗北回避)
 *   IBoardModifier    盤面サイズの決定 (盤面拡張等)
 ****************************************/
#ifndef ABILITY_HOOKS_H
#define ABILITY_HOOKS_H

#include "piece.h"

class Board;       // 前方宣言
struct GameState;  // 前方宣言

//======================================
// 勝利条件フック
//======================================
/**
 * @class  IWinCondition
 * @brief  勝利条件の判定インターフェイス
 * @detail 着手後に評価され、Check が true を返した陣営が勝利する。
 */
class IWinCondition
{
public:
    virtual ~IWinCondition() = default;
    /**
     * @brief  player が勝利条件を満たしているか判定
     * @param  board  対象の盤面
     * @param  player 判定する陣営
     * @return 勝利していれば true
     */
    virtual bool Check(const Board& board, Piece player) = 0;
};

//======================================
// 駒設置イベントフック
//======================================
/**
 * @class  IPlacementHandler
 * @brief  駒設置時のイベントインターフェイス
 * @detail 設置直後に呼ばれる。盤面を書き換えたり追加駒を置いたりできる
 *         (例: 氷駒で1マス滑らせる、連鎖駒で隣接マスに自動配置)。
 */
class IPlacementHandler
{
public:
    virtual ~IPlacementHandler() = default;
    /**
     * @brief  駒が設置された直後に呼ばれる
     * @param  state    試合状態
     * @param  pos      設置されたマス座標
     * @param  placedBy 設置した陣営
     */
    virtual void OnPlace(GameState& state, Vec2 pos, Piece placedBy) = 0;
};

//======================================
// ターン処理フック
//======================================
/**
 * @class  ITurnHandler
 * @brief  ターン処理のインターフェイス
 * @detail 1ターンの着手数や、ターン境界での状態更新を担う。
 */
class ITurnHandler
{
public:
    virtual ~ITurnHandler() = default;
    /** @brief 1ターンに置ける駒数を返す (例: 二手打ちで2) */
    virtual int  GetPlacementCount(Piece player) = 0;
    /** @brief ターン開始時に呼ばれる */
    virtual void OnTurnStart(GameState& state, Piece player) = 0;
    /** @brief ターン終了時に呼ばれる */
    virtual void OnTurnEnd  (GameState& state, Piece player) = 0;
};

//======================================
// 思考時間プロバイダフック
//======================================
/**
 * @class  ITurnTimeProvider
 * @brief  思考時間を決定するインターフェイス
 * @detail 各陣営のターン開始時に呼ばれる。
 *         例: 「熟考」は +30s、「焦燥」は -90s などを反映する。
 */
class ITurnTimeProvider
{
public:
    virtual ~ITurnTimeProvider() = default;
    /**
     * @brief  指定陣営のターン制限時間を返す
     * @param  player 対象の陣営
     * @return 制限時間(秒)
     */
    virtual double GetTurnTimeSeconds(Piece player) = 0;
};

//======================================
// 敗北処理フック
//======================================
/**
 * @class  IDefeatHandler
 * @brief  敗北処理のインターフェイス
 * @detail 敗北発生時(時間切れ・勝利条件達成等)に呼ばれる。
 *         true を返すと敗北を打ち消す (例: 「不死」「復活の意志」)。
 */
class IDefeatHandler
{
public:
    virtual ~IDefeatHandler() = default;
    /**
     * @brief  敗北発生時に呼ばれる
     * @param  state 試合状態
     * @param  loser 敗北しようとしている陣営
     * @return true を返すと敗北を回避する
     */
    virtual bool OnDefeat(GameState& state, Piece loser) = 0;
};

//======================================
// 盤面サイズフック
//======================================
/**
 * @class  IBoardModifier
 * @brief  盤面サイズを決定するインターフェイス
 * @detail 試合開始時に呼ばれ、盤面の縦横マス数を決める。
 *         例: 「盤面拡張」は 3×3 を 4×4 に広げる。
 */
class IBoardModifier
{
public:
    virtual ~IBoardModifier() = default;
    /**
     * @brief  盤面サイズを問い合わせる
     * @param  width  [in/out] 盤面の横マス数
     * @param  height [in/out] 盤面の縦マス数
     */
    virtual void GetBoardSize(int& width, int& height) = 0;
};

#endif // ABILITY_HOOKS_H
