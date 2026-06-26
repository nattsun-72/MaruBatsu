/****************************************
 * @file   scene.h
 * @brief  シーン管理 (2D版)
 * @author Natsume Shidara
 * @date   2025/07/10
 * @update 2026/05/15 - 〇×ローグライト用2D版へ書き換え
 *
 * タイトル/ゲーム/報酬の3シーンを切り替える。遷移は予約制で、
 * Scene_Refresh() のタイミングでのみ実際の切り替えが確定する。
 ****************************************/

#ifndef SCENE_H
#define SCENE_H

//======================================
// シーン種別
//======================================
/**
 * @enum  Scene
 * @brief 画面の種類を表す列挙型
 */
enum class Scene
{
    TITLE,       // タイトル画面
    GAME,        // ゲーム本編 (盤面プレイ)
    BOSS_REWARD, // ボス撃破時の固有スキル獲得画面 (はい/いいえ)
    REWARD,      // 撃破報酬の選択画面 (通常抽選3択)
    RESULT,      // ラン終了時のリザルト画面
    HISTORY,     // 戦績履歴の閲覧画面
};

//======================================
// シーン管理 API
//======================================
/** @brief 現在シーンの初期化 */
void Scene_Initialize();

/** @brief 現在シーンの終了処理 */
void Scene_Finalize();

/**
 * @brief シーン更新
 * @param elapsed_time 前フレームからの経過秒数
 */
void Scene_Update(double elapsed_time);

/** @brief 現在シーンの描画 (2D) */
void Scene_Draw();

/** @brief 予約済みシーン遷移の確定処理 (毎フレーム呼ぶ) */
void Scene_Refresh();

/**
 * @brief 次フレーム以降に切り替えるシーンを予約する
 * @param scene 遷移先シーン
 */
void Scene_Change(Scene scene);

/** @brief 現在表示中のシーンを取得する (オーバーレイ等が文脈判断に使う) */
Scene Scene_Current();

#endif // SCENE_H
