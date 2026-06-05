/****************************************
 * @file   result.h
 * @brief  リザルトシーン (ラン終了時の戦績表示)
 * @author Natsume Shidara
 * @date   2026/06/05
 *
 * ラン終了(クリア/敗北)時に、結果・撃破数・プレイ時間・ターン数・
 * 取得アビリティ・(敗北時)倒されたボス名と日時を表示する。
 ****************************************/
#ifndef RESULT_H
#define RESULT_H

//======================================
// リザルトシーン API
//======================================
/** @brief リザルトシーンの初期化 */
void Result_Initialize();

/** @brief リザルトシーンの終了処理 */
void Result_Finalize();

/**
 * @brief リザルトシーンの更新 (ボタン操作)
 * @param elapsed_time 前フレームからの経過秒数
 */
void Result_Update(double elapsed_time);

/** @brief リザルトシーンの描画 */
void Result_Draw();

#endif // RESULT_H
