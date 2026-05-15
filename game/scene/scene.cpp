/****************************************
 * @file scene.cpp
 * @brief シーン管理・描画パイプライン制御（ShadowMap対応版）
 * @author Natsume Shidara
 * @date 2025/07/10
 * @update 2026/01/12 - Resultシーン追加
 * @update 2026/01/13 - シーン切り替え時サウンドリセット対応
 * @update 2026/01/13 - Settingsシーン追加
 ****************************************/

#include "scene.h"

 // 各シーン
#include "game.h"
#include "title.h"
#include "result.h"
#include "settings.h"

// 入力
#include "input_manager.h"

// 描画・システム関連
#include "direct3d.h"
#include "render_target.h"
#include "sprite.h"
#include "fade.h"
#include "light_camera.h"
#include "shader_shadow_map.h"
#include "shader_shadow_map_skinned.h"
#include "shader3d.h"
#include "shader_skinned.h"
#include "post_process.h"
#include "model.h"

// サウンド
#include "sound_manager.h"

// 内部変数・定数
// シャドウマップ解像度
namespace
{
    constexpr int SHADOW_MAP_RESOLUTION = 8192;  // シャドウマップ解像度（ピクセル）
}

static auto g_Scene = Scene::TITLE;
static Scene g_SceneNext = g_Scene;

// オフスクリーンレンダリング用（シャドウマップ用）
static RenderTarget g_ShadowMap;

// 内部関数プロトタイプ宣言
static void DrawOffscreen();  // パス1: シャドウマップ生成
static void DrawMainScene();  // パス2: メイン描画
static void DrawUI();         // パス3: UI描画

// 公開関数実装

void Scene_Initialize()
{
    // モデルシステム初期化（フォールバックテクスチャ等）
    Model_Initialize();

    // シャドウマップ生成用（SHADOW_MAP_RESOLUTION x SHADOW_MAP_RESOLUTION, Shadowモード）
    g_ShadowMap.Initialize(SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION, RenderTargetType::Shadow);

    // シェーダー初期化
    ShaderShadowMap_Initialize(Direct3D_GetDevice(), Direct3D_GetContext());
    ShaderShadowMapSkinned_Initialize(Direct3D_GetDevice(), Direct3D_GetContext());

    switch (g_Scene)
    {
    case Scene::TITLE:    Title_Initialize();    break;
    case Scene::GAME:     Game_Initialize();     break;
    case Scene::RESULT:   Result_Initialize();   break;
    case Scene::SETTINGS: Settings_Initialize(); break;
    default: break;
    }
}

void Scene_Finalize()
{
    switch (g_Scene)
    {
    case Scene::TITLE:    Title_Finalize();    break;
    case Scene::GAME:     Game_Finalize();     break;
    case Scene::RESULT:   Result_Finalize();   break;
    case Scene::SETTINGS: Settings_Finalize(); break;
    default: break;
    }

    ShaderShadowMap_Finalize();
    ShaderShadowMapSkinned_Finalize();
    g_ShadowMap.Finalize();
    Model_Finalize();
}

void Scene_Update(double elapsed_time)
{
    InputManager_Update();

    switch (g_Scene)
    {
    case Scene::TITLE:    Title_Update(elapsed_time);    break;
    case Scene::GAME:     Game_Update(elapsed_time);     break;
    case Scene::RESULT:   Result_Update(elapsed_time);   break;
    case Scene::SETTINGS: Settings_Update(elapsed_time); break;
    default: break;
    }
}

void Scene_Draw()
{
    // 1. シャドウマップ生成パス
    DrawOffscreen();

    // 2. メイン描画パス
    DrawMainScene();

    // 3. UI描画パス
    DrawUI();
}

void Scene_Refresh()
{
    if (g_Scene != g_SceneNext)
    {
        // 全サウンド停止（BGM・SE全リセット）
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

// 内部関数実装

/**
 * @brief パス1: シャドウマップ生成
 */
static void DrawOffscreen()
{
    // 1. レンダーターゲット切り替え & 深度クリア
    g_ShadowMap.Activate();

    // 2. シャドウ用ラスタライザ設定（シャドウアクネ防止）
    Direct3D_SetRasterizerState_Shadow();

    // 3. 深度テスト有効化
    Direct3D_DepthStencilStateDepthIsEnable(true);

    // 4. シャドウマップ用シェーダー開始
    ShaderShadowMap_Begin();

    // 5. ライトビュープロジェクション行列の計算と設定
    DirectX::XMMATRIX lightView = DirectX::XMLoadFloat4x4(&LightCamera_GetViewMatrix());
    DirectX::XMMATRIX lightProj = DirectX::XMLoadFloat4x4(&LightCamera_GetProjectionMatrix());
    DirectX::XMMATRIX lightVP = lightView * lightProj;

    DirectX::XMFLOAT4X4 matLightVP;
    DirectX::XMStoreFloat4x4(&matLightVP, lightVP);

    // シェーダー定数バッファにセット
    ShaderShadowMap_SetLightViewProjection(matLightVP);
    ShaderShadowMapSkinned_SetLightViewProjection(matLightVP);

    // 6. 全オブジェクトの描画（影を落とす物の全て）
    switch (g_Scene)
    {
    case Scene::TITLE:    Title_DrawShadow();    break;
    case Scene::GAME:     Game_DrawShadow();     break;
    case Scene::RESULT:   Result_DrawShadow();   break;
    case Scene::SETTINGS: Settings_DrawShadow(); break;
    default: break;
    }

    // 7. 設定をデフォルトに戻す
    Direct3D_SetRasterizerState_Default();
}

/**
 * @brief パス2: メイン描画
 */
static void DrawMainScene()
{
    Direct3D_SetBackBufferRenderTarget();
    Direct3D_Clear();
    Direct3D_DepthStencilStateDepthIsEnable(true);

    // メインシェーダー開始
    Shader3D_Begin();

    // メインシェーダーに影データを渡す
    // 1. シャドウマップテクスチャ (slot 1)
    Shader3D_SetShadowMap(g_ShadowMap.GetSRV());

    // 2. ライトビュープロジェクション行列
    DirectX::XMMATRIX lightView = DirectX::XMLoadFloat4x4(&LightCamera_GetViewMatrix());
    DirectX::XMMATRIX lightProj = DirectX::XMLoadFloat4x4(&LightCamera_GetProjectionMatrix());
    DirectX::XMFLOAT4X4 matLightVP;
    DirectX::XMStoreFloat4x4(&matLightVP, lightView * lightProj);

    Shader3D_SetLightViewProjection(matLightVP);
    ShaderSkinned_SetLightViewProjection(matLightVP);
    ShaderSkinned_SetShadowMap(g_ShadowMap.GetSRV());

    switch (g_Scene)
    {
    case Scene::TITLE:    Title_Draw();    break;
    case Scene::GAME:     Game_Draw();     break;
    case Scene::RESULT:   Result_Draw();   break;
    case Scene::SETTINGS: Settings_Draw(); break;
    default: break;
    }

    // バインド解除（読み書き同時バインド防止）
    ID3D11ShaderResourceView* nullSRV = nullptr;
    Shader3D_SetShadowMap(nullSRV);
    ShaderSkinned_SetShadowMap(nullSRV);
}

/**
 * @brief パス3: UI描画
 */
static void DrawUI()
{
    Direct3D_DepthStencilStateDepthIsEnable(false);
    Direct3D_SetBlendState(BlendMode::Alpha);
    Sprite_Begin();

    // デバッグ表示: シャドウマップの出力確認
    // 画面に表示して、影がちゃんとレンダリングされているかチェック
    ID3D11ShaderResourceView* pShadowSRV = g_ShadowMap.GetSRV();
    if (pShadowSRV)
    {
        // 深度バッファ(R32_FLOAT)は赤一色に値が入っているので、
        // そのまま表示すると赤色の濃淡になりますが、重要です。
        //Sprite_Draw(pShadowSRV, Direct3D_GetBackBufferWidth() - 200.0f, 0.0f, 200.0f, 200.0f);
    }

    Fade_Draw();
    Direct3D_DepthStencilStateDepthIsEnable(true);
}