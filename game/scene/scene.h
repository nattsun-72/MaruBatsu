/****************************************
 * @file scene.h
 * @brief シーン管理 (2D版)
 * @author Natsume Shidara
 * @date 2025/07/10
 * @update 2026/05/15 - 〇×ローグライト用2D版へ書き換え
 ****************************************/

#ifndef SCENE_H
#define SCENE_H

/** @brief シーン列挙型 */
enum class Scene
{
    TITLE,
    GAME,
    REWARD,
};

/** @brief シーン初期化 */
void Scene_Initialize();

/** @brief シーン終了処理 */
void Scene_Finalize();

/** @brief シーン更新 */
void Scene_Update(double elapsed_time);

/** @brief シーン描画（2D） */
void Scene_Draw();

/** @brief シーン遷移の確定処理（毎フレーム呼ぶ） */
void Scene_Refresh();

/** @brief シーン遷移を予約 */
void Scene_Change(Scene scene);

#endif // SCENE_H
