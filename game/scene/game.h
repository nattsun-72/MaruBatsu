/****************************************
 * @file   game.h
 * @brief  ゲームシーン
 * @author Natsume Shidara
 * @date   2025/07/01
 * @update 2026/05/15 - 〇×ローグライト用スタブへ書き換え
 *
 * 盤面プレイ本編。ボス戦の進行・勝敗判定・描画を担う。
 ****************************************/
#ifndef GAME_H
#define GAME_H

//======================================
// ゲームシーン API
//======================================
/** @brief ゲームシーンの初期化 */
void Game_Initialize();

/** @brief ゲームシーンの終了処理 */
void Game_Finalize();

/**
 * @brief ゲームシーンの更新
 * @param elapsed_time 前フレームからの経過秒数
 */
void Game_Update(double elapsed_time);

/** @brief ゲームシーンの描画 */
void Game_Draw();

#endif // GAME_H
