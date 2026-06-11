/****************************************
 * @file   history.h
 * @brief  戦績履歴シーン (過去ランの一覧)
 * @author Natsume Shidara
 * @date   2026/06/05
 *
 * タイトルから開く。過去ランの戦績(日時・結果・撃破数・時間・ターン)を
 * 新しい順に一覧表示する。マウスホイールでスクロール可能。
 ****************************************/
#ifndef HISTORY_H
#define HISTORY_H

//======================================
// 戦績履歴シーン API
//======================================
/** @brief 履歴シーンの初期化 (履歴ファイルを読み込む) */
void History_Initialize();

/** @brief 履歴シーンの終了処理 */
void History_Finalize();

/**
 * @brief 履歴シーンの更新 (スクロール・戻る操作)
 * @param elapsed_time 前フレームからの経過秒数
 */
void History_Update(double elapsed_time);

/** @brief 履歴シーンの描画 */
void History_Draw();

#endif // HISTORY_H
