/****************************************
 * @file   scene.cpp
 * @brief  シーン管理 (2D版)
 * @author Natsume Shidara
 * @date   2025/07/10
 * @update 2026/05/15 - 〇×ローグライト用2D版へ書き換え
 ****************************************/

#include "scene.h"
#include "game.h"
#include "title.h"
#include "reward.h"
#include "result.h"
#include "history.h"

#include "input_manager.h"
#include "direct3d.h"
#include "sprite.h"
#include "sound_manager.h"

//--------------------------------------
// 内部状態
//--------------------------------------
static auto  g_Scene     = Scene::TITLE;  // 現在のシーン
static Scene g_SceneNext = g_Scene;       // 次フレームで切り替える予約先シーン

//======================================
// シーン初期化／終了
//======================================
void Scene_Initialize()
{
    // 現在シーンに応じた初期化関数へ振り分ける
    switch (g_Scene)
    {
    case Scene::TITLE:  Title_Initialize();  break;
    case Scene::GAME:   Game_Initialize();   break;
    case Scene::REWARD: Reward_Initialize(); break;
    case Scene::RESULT: Result_Initialize(); break;
    case Scene::HISTORY: History_Initialize(); break;
    default: break;
    }
}

void Scene_Finalize()
{
    // 現在シーンに応じた終了関数へ振り分ける
    switch (g_Scene)
    {
    case Scene::TITLE:  Title_Finalize();  break;
    case Scene::GAME:   Game_Finalize();   break;
    case Scene::REWARD: Reward_Finalize(); break;
    case Scene::RESULT: Result_Finalize(); break;
    case Scene::HISTORY: History_Finalize(); break;
    default: break;
    }
}

//======================================
// シーン更新
//======================================
void Scene_Update(double elapsed_time)
{
    // 入力状態を先に更新してから各シーンへ渡す
    InputManager_Update();

    switch (g_Scene)
    {
    case Scene::TITLE:  Title_Update(elapsed_time);  break;
    case Scene::GAME:   Game_Update(elapsed_time);   break;
    case Scene::REWARD: Reward_Update(elapsed_time); break;
    case Scene::RESULT: Result_Update(elapsed_time); break;
    case Scene::HISTORY: History_Update(elapsed_time); break;
    default: break;
    }
}

//======================================
// シーン描画
//======================================
void Scene_Draw()
{
    /*--- 2D描画用のレンダーステートを設定 ---*/
    Direct3D_SetBackBufferRenderTarget();
    Direct3D_Clear();
    Direct3D_DepthStencilStateDepthIsEnable(false);   // 2Dなので深度テスト無効
    Direct3D_SetBlendState(BlendMode::Alpha);
    Sprite_Begin();

    /*--- 現在シーンの描画関数へ振り分け ---*/
    switch (g_Scene)
    {
    case Scene::TITLE:  Title_Draw();  break;
    case Scene::GAME:   Game_Draw();   break;
    case Scene::REWARD: Reward_Draw(); break;
    case Scene::RESULT: Result_Draw(); break;
    case Scene::HISTORY: History_Draw(); break;
    default: break;
    }
}

//======================================
// シーン遷移
//======================================
void Scene_Refresh()
{
    // 予約先が現在シーンと異なる場合のみ、終了→切替→初期化を行う
    if (g_Scene != g_SceneNext)
    {
        SoundManager_StopAll();   // 旧シーンの再生音を停止
        Scene_Finalize();
        g_Scene = g_SceneNext;
        Scene_Initialize();
    }
}

void Scene_Change(Scene scene)
{
    // 実際の切り替えは Scene_Refresh() まで遅延させる
    g_SceneNext = scene;
}
