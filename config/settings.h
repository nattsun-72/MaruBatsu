/****************************************
 * @file settings.h
 * @brief 設定画面の実装
 * @author Natsume Shidara
 * @date 2026/01/13
 ****************************************/

#ifndef SETTINGS_H
#define SETTINGS_H

/** @brief 設定画面の初期化 */
void Settings_Initialize();

/** @brief 設定画面の終了処理 */
void Settings_Finalize();

/** @brief 設定画面の更新処理 */
void Settings_Update(double elapsed_time);

/** @brief 設定画面の描画 */
void Settings_Draw();

/** @brief 設定画面のシャドウ描画 */
void Settings_DrawShadow();

#endif // SETTINGS_H