/****************************************
 * @file   ability_registry.h
 * @brief  各フック実装を保持し、ゲームロジックへ提供するレジストリ
 * @author Natsume Shidara
 * @date   2026/05/15
 *
 * Game シーンは AbilityRegistry を介してフックを呼び出す。
 * 生成時にデフォルト実装がセットされ、アビリティ獲得時に
 * Set/Add で上書き・追加される。
 ****************************************/
#ifndef ABILITY_REGISTRY_H
#define ABILITY_REGISTRY_H

#include "hooks.h"
#include <memory>
#include <vector>

//======================================
// アビリティレジストリ
//======================================
/**
 * @class  AbilityRegistry
 * @brief  フック実装の集約・呼び出し窓口
 * @detail 単一フック(勝利条件・ターン処理・思考時間)は差し替え式、
 *         複数フック(設置イベント・敗北処理)は追加式で保持する。
 */
class AbilityRegistry
{
public:
    //======================================
    // 構築・リセット
    //======================================
    /** @brief 構築 (デフォルトフックを初期セット) */
    AbilityRegistry();

    /** @brief 全フックをデフォルト実装へ戻す (試合の初期化/リセット時) */
    void ResetToDefaults();

    //======================================
    // 単一フックの差し替え
    //======================================
    // いずれも旧実装を返す。アビリティ側が chained に保存してチェイン呼び出しできる。
    /** @brief 勝利条件を差し替え @return 旧実装 */
    std::shared_ptr<IWinCondition>     SetWinCondition   (std::shared_ptr<IWinCondition>   w);
    /** @brief ターン処理を差し替え @return 旧実装 */
    std::shared_ptr<ITurnHandler>      SetTurnHandler    (std::shared_ptr<ITurnHandler>    t);
    /** @brief 思考時間プロバイダを差し替え @return 旧実装 */
    std::shared_ptr<ITurnTimeProvider> SetTurnTimeProvider(std::shared_ptr<ITurnTimeProvider> p);
    /** @brief 盤面サイズフックを差し替え @return 旧実装 */
    std::shared_ptr<IBoardModifier>    SetBoardModifier  (std::shared_ptr<IBoardModifier>   m);

    //======================================
    // 複数登録可能なフック
    //======================================
    /** @brief 設置イベントハンドラを追加 */
    void AddPlacementHandler(std::shared_ptr<IPlacementHandler> h);
    /** @brief 敗北ハンドラを追加 */
    void AddDefeatHandler  (std::shared_ptr<IDefeatHandler>   h);

    //======================================
    // フック呼び出しヘルパ
    //======================================
    /** @brief 勝利条件を評価 @return player が勝利していれば true */
    bool   CheckWin     (const Board& board, Piece player);
    /** @brief 全設置ハンドラの OnPlace を順に呼ぶ */
    void   OnPlace      (GameState& state, Vec2 pos, Piece placedBy);
    /** @brief 1ターンの着手数を取得 */
    int    GetPlacementCount(Piece player);
    /** @brief ターン開始フックを呼ぶ */
    void   OnTurnStart  (GameState& state, Piece player);
    /** @brief ターン終了フックを呼ぶ */
    void   OnTurnEnd    (GameState& state, Piece player);
    /** @brief 指定陣営のターン制限時間を取得 */
    double GetTurnTime  (Piece player);
    /** @brief 敗北ハンドラを順に呼ぶ @return いずれかが敗北回避したら true */
    bool   HandleDefeat (GameState& state, Piece loser);
    /**
     * @brief  盤面サイズを取得する (盤面拡張アビリティ等を反映)
     * @param  width  [in/out] 盤面の横マス数
     * @param  height [in/out] 盤面の縦マス数
     */
    void   GetBoardSize (int& width, int& height);

    //======================================
    // アクセサ
    //======================================
    /** @brief 現在の勝利条件を取得 (AIが直接参照する用途) */
    IWinCondition* GetWinCondition() { return m_Win.get(); }

private:
    //--------------------------------------
    // 保持するフック実装
    //--------------------------------------
    std::shared_ptr<IWinCondition>     m_Win;          // 勝利条件(単一)
    std::shared_ptr<ITurnHandler>      m_Turn;         // ターン処理(単一)
    std::shared_ptr<ITurnTimeProvider> m_TimeProvider; // 思考時間(単一)
    std::shared_ptr<IBoardModifier>    m_BoardMod;     // 盤面サイズ(単一)
    std::vector<std::shared_ptr<IPlacementHandler>> m_OnPlace;  // 設置イベント(複数)
    std::vector<std::shared_ptr<IDefeatHandler>>    m_OnDefeat; // 敗北処理(複数)
};

#endif // ABILITY_REGISTRY_H
