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
#include <vector>

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
}

#endif // RUN_STATE_H
