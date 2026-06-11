/****************************************
 * @file   run_state.h
 * @brief  1ラン(5戦)の進行状態をシーン間で共有
 * @author Natsume Shidara
 * @date   2026/05/15
 *
 * Game/Reward シーンはローカルにラン状態を持たず、本モジュールを参照する。
 * α版で RunManager クラスへ統合する前提の、シンプルな名前空間API。
 ****************************************/
#ifndef RUN_STATE_H
#define RUN_STATE_H

//--------------------------------------
// インクルード
//--------------------------------------
#include "ability/ability.h"

#include <memory>
#include <string>
#include <vector>

//======================================
// ラン結果 (リザルト画面・履歴用の戦績スナップショット)
//======================================
/**
 * @struct RunResult
 * @brief  1ラン終了時の戦績
 * @detail リザルト画面の表示と、履歴ログへの記録に使う。
 */
struct RunResult
{
    bool        cleared        = false;  // クリア(全撃破)なら true、敗北なら false
    int         bossesDefeated = 0;      // 撃破したボス数
    int         bossTotal      = 0;      // ボス総数 (例 5)
    double      timeSeconds    = 0.0;    // ランの総プレイ時間(秒)
    int         turns          = 0;      // ランの総ターン数
    std::string defeatedByBoss;          // 敗北時に倒されたボス名 (クリア時は空)
    std::string dateTime;                // 終了日時 "YYYY/MM/DD HH:MM"
    std::vector<std::string> abilityNames; // 取得アビリティ名(取得順)
};

//======================================
// ラン進行状態 名前空間
//======================================
namespace RunState
{
    /** @brief プレイヤーの所持アビリティ一覧(ランを通じて累積)を取得 */
    std::vector<std::shared_ptr<Ability>>& PlayerAbilities();

    /** @brief 現在の報酬画面で提示している3択アビリティを取得 */
    std::vector<std::shared_ptr<Ability>>& RewardChoices();

    /** @brief 現在挑戦中のボスindex (0=ボス1, …, 4=ラスボス) を取得 */
    int  CurrentBossIndex();
    /** @brief ボスindexを1つ進める (ボス撃破時に呼ぶ) */
    void IncrementBoss();

    /** @brief 1ランのボス総数 (ロスター定義に追従) を取得 */
    int  BossCount();
    /**
     * @brief  ランを完走したか (全ボスを撃破したか) を判定
     * @return CurrentBossIndex が BossCount 以上なら true
     */
    bool IsRunComplete();

    /** @brief 直近のランをクリアした表示状態か (タイトルの制覇表示用) */
    bool IsRunCleared();
    /** @brief ランクリアを記録する (ラスボス撃破時に呼ぶ) */
    void MarkRunCleared();

    /** @brief ラン状態を完全リセット (タイトルから新ラン開始時に呼ぶ) */
    void ResetRun();

    /** @brief 報酬の3択をプールからランダム抽選して生成する */
    void GenerateRewardChoices();

    //======================================
    // ラン統計 (リザルト用)
    //======================================
    /** @brief ランの累積プレイ時間に dt(秒) を加算する (ゲーム進行中に呼ぶ) */
    void   AddRunTime(double dt);
    /** @brief ランの累積ターン数を1増やす (ターン開始時に呼ぶ) */
    void   AddRunTurn();
    /** @brief ランの累積プレイ時間(秒)を取得 */
    double RunTime();
    /** @brief ランの累積ターン数を取得 */
    int    RunTurns();

    /**
     * @brief  ラン終了時の戦績を確定して保持する
     * @param  cleared        クリア(全撃破)なら true
     * @param  defeatedByBoss 敗北時に倒されたボス名 (クリア時は空文字)
     */
    void             CaptureResult(bool cleared, const std::string& defeatedByBoss);
    /** @brief 直近に確定した戦績を取得 (リザルト画面が参照) */
    const RunResult& LastResult();

    /**
     * @brief  過去ランの戦績履歴を読み込む
     * @return 新しい順の戦績一覧 (最大100件)。履歴が無ければ空。
     */
    std::vector<RunResult> LoadHistory();

    //======================================
    // セーブ／ロード (中断再開用の最小実装)
    //======================================
    /** @brief セーブデータが存在するか */
    bool HasSave();
    /**
     * @brief  現在のラン状態(ボス番号+所持アビリティ名)をファイルへ保存する
     * @detail 各戦の開始時に呼ぶことで、アプリ終了後も同じ戦況から再開できる。
     *         盤面途中・残時間・チャージ等は保存しない(最小実装)。
     */
    void SaveRun();
    /**
     * @brief  セーブからラン状態を復元する
     * @return 復元に成功したら true (セーブ無し/破損時は false)
     */
    bool LoadRun();
    /** @brief セーブデータを削除する (ランクリア時・新規開始時に呼ぶ) */
    void ClearSave();
}

#endif // RUN_STATE_H
