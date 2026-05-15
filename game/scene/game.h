/****************************************
 * @file game.h
 * @brief ゲームシーンの管理
 * @author Natsume Shidara
 * @date 2025/07/01
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

/** @brief ゲームシーンのシャドウマップ描画 */
void Game_DrawShadow();
#endif
