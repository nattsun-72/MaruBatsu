/****************************************
 * @file fever.h
 * @brief フィーバーモード管理
 * @detail コンボ数に応じた段階的パワーアップシステム
 *   Tier 1 (combo >= 50):  FOV拡大、移動速度UP、3段ジャンプ、エアダッシュ2回
 *   Tier 2 (combo >= 100): + 金色ビネット、HP自動回復
 *   Tier 3 (combo >= 200): + 攻撃範囲拡大
 * @author Natsume Shidara
 * @date 2026/02/22
 ****************************************/

#ifndef FEVER_H
#define FEVER_H

// フィーバーティア
enum class FeverTier
{
    None = 0,
    Tier1,      // combo >= 50
    Tier2,      // combo >= 100
    Tier3,      // combo >= 200
};

// 初期化・終了
/** @brief フィーバーシステム初期化 */
void Fever_Initialize();
/** @brief フィーバーシステム終了処理 */
void Fever_Finalize();

// 更新・描画
/** @brief フィーバー状態更新 */
void Fever_Update(float dt);
/** @brief フィーバー演出描画（金色ビネット等） */
void Fever_Draw();

// ゲッター
/** @brief 現在のフィーバーティアを取得 */
FeverTier Fever_GetTier();
/** @brief フィーバーが有効か */
bool Fever_IsActive();

/** @brief 移動速度倍率を取得（Tier1+） */
float Fever_GetSpeedMultiplier();
/** @brief 目標FOVを取得（Tier1+） */
float Fever_GetFOVTarget();
/** @brief 最大ジャンプ回数を取得（Tier1+） */
int   Fever_GetMaxJumpCount();
/** @brief 最大エアダッシュ回数を取得（Tier1+） */
int   Fever_GetMaxAirDashCount();

/** @brief 攻撃範囲倍率を取得（Tier3） */
float Fever_GetRangeMultiplier();

#endif // FEVER_H
