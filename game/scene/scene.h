/****************************************
 * @file scene.h
 * @brief シーン管理
 * @author Natsume Shidara
 * @date 2025/07/10
 * @update 2026/01/13 - Settings画面追加
 ****************************************/

#ifndef SCENE_H
#define SCENE_H

/** @brief シーン列挙型 */
enum class Scene
{
    TITLE,
    GAME,
    RESULT,
    SETTINGS,  // 設定画面
};

/** @brief シーン初期化 */
void Scene_Initialize();

/** @brief シーン終了処理 */
void Scene_Finalize();

/** @brief シーン更新 */
void Scene_Update(double elapsed_time);

/** @brief シーン描画（シャドウマップ・メイン・UI） */
void Scene_Draw();

/** @brief シーン遷移の確定処理（毎フレーム呼ぶ） */
void Scene_Refresh();

/** @brief シーン遷移を予約 */
void Scene_Change(Scene scene);

#endif // SCENE_H