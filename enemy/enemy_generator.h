/****************************************
 * @file enemy_generator.h
 * @brief 敵スポーン管理
 * @detail ゲーム中の敵生成ロジック
 *         （間隔制御、バッチ生成、位置決定、難易度進行）
 * @author Natsume Shidara
 * @date 2026/02/23
 ****************************************/

#ifndef ENEMY_GENERATOR_H
#define ENEMY_GENERATOR_H

#include "enemy.h"
#include <DirectXMath.h>

/**
 * @brief スポーン状態の初期化（初期敵の生成含む）
 */
void EnemyGenerator_Initialize();

/**
 * @brief スポーン状態のリセット
 */
void EnemyGenerator_Finalize();

/**
 * @brief スポーン更新（毎フレーム呼び出し）
 * @param dt デルタタイム（秒）
 * @param gameElapsedTime ゲーム経過時間（秒） - 難易度進行に使用
 */
void EnemyGenerator_Update(float dt, float gameElapsedTime);

/**
 * @brief 指定タイプの敵を1体スポーン（デバッグ用）
 * @param type 敵タイプ
 */
void EnemyGenerator_SpawnOne(ENEMY_TYPE type);

/**
 * @brief デバッグ描画（スポーン禁止エリア等）
 */
void EnemyGenerator_DrawDebug();

#endif // ENEMY_GENERATOR_H
