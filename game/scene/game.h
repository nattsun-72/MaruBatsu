/****************************************
 * @file game.h
 * @brief ゲームシーン (Day 1 スタブ)
 * @author Natsume Shidara
 * @date 2025/07/01
 * @update 2026/05/15 - 〇×ローグライト用スタブへ書き換え
 ****************************************/
#ifndef GAME_H
#define GAME_H

/** @brief ゲームシーン初期化 */
void Game_Initialize();

/** @brief ゲームシーン終了処理 */
void Game_Finalize();

/** @brief ゲームシーン更新 */
void Game_Update(double elapsed_time);

/** @brief ゲームシーン描画 */
void Game_Draw();

#endif // GAME_H
