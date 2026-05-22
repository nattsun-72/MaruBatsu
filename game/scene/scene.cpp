/****************************************
 * @file scene.cpp
 * @brief シーン管理 (2D版)
 * @author Natsume Shidara
 * @date 2025/07/10
 * @update 2026/05/15 - 〇×ローグライト用2D版へ書き換え
 ****************************************/

#include "scene.h"
#include "game.h"
#include "title.h"
#include "reward.h"

#include "input_manager.h"
#include "direct3d.h"
#include "sprite.h"
#include "sound_manager.h"

static auto g_Scene = Scene::TITLE;
static Scene g_SceneNext = g_Scene;

void Scene_Initialize()
{
    switch (g_Scene)
    {
    case Scene::TITLE:  Title_Initialize();  break;
    case Scene::GAME:   Game_Initialize();   break;
    case Scene::REWARD: Reward_Initialize(); break;
    default: break;
    }
}

void Scene_Finalize()
{
    switch (g_Scene)
    {
    case Scene::TITLE:  Title_Finalize();  break;
    case Scene::GAME:   Game_Finalize();   break;
    case Scene::REWARD: Reward_Finalize(); break;
    default: break;
    }
}

void Scene_Update(double elapsed_time)
{
    InputManager_Update();

    switch (g_Scene)
    {
    case Scene::TITLE:  Title_Update(elapsed_time);  break;
    case Scene::GAME:   Game_Update(elapsed_time);   break;
    case Scene::REWARD: Reward_Update(elapsed_time); break;
    default: break;
    }
}

void Scene_Draw()
{
    Direct3D_SetBackBufferRenderTarget();
    Direct3D_Clear();
    Direct3D_DepthStencilStateDepthIsEnable(false);
    Direct3D_SetBlendState(BlendMode::Alpha);
    Sprite_Begin();

    switch (g_Scene)
    {
    case Scene::TITLE:  Title_Draw();  break;
    case Scene::GAME:   Game_Draw();   break;
    case Scene::REWARD: Reward_Draw(); break;
    default: break;
    }
}

void Scene_Refresh()
{
    if (g_Scene != g_SceneNext)
    {
        SoundManager_StopAll();
        Scene_Finalize();
        g_Scene = g_SceneNext;
        Scene_Initialize();
    }
}

void Scene_Change(Scene scene)
{
    g_SceneNext = scene;
}
