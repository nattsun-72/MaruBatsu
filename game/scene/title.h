/****************************************
 * @file   title.h
 * @brief  タイトルシーン
 * @author Natsume Shidara
 * @date   2026/05/15
 *
 * タイトルロゴを表示し、入力でゲーム本編へ遷移する。
 ****************************************/

#ifndef TITLE_H
#define TITLE_H

//======================================
// タイトルシーン API
//======================================
/** @brief タイトルシーンの初期化 */
void Title_Initialize();

/** @brief タイトルシーンの終了処理 */
void Title_Finalize();

/**
 * @brief タイトルシーンの更新
 * @param elapsed_time 前フレームからの経過秒数
 */
void Title_Update(double elapsed_time);

/** @brief タイトルシーンの描画 */
void Title_Draw();

#endif // TITLE_H
