/****************************************
 * @file game.cpp
 * @brief ゲーム管理クラス（実装・統合用）
 * @detail
 * - Stageによるアリーナ（広さ・方向・壁）
 * - ランダム敵スポーンシステム
 * - シャドウマップ生成と描画
 * - Blade統合（一人称視点）
 * - ポストプロセス（ブラー効果）
 * @author Natsume Shidara
 * @update 2026/01/13 - ステージ・ランダムスポーン追加
 * @update 2026/01/13 - サウンド対応・デバッグ機能分離
 * @update 2026/02/04 - パーティクルシステム追加
 * @update 2026/02/06 - リファクタリング（実装取得の一元化・内部関数分割・前提整理）
 ****************************************/

#include "game.h"

 //--------------------------------------
 // システム・共通ヘッダ
 //--------------------------------------
#include "direct3d.h"
#include "camera.h"
#include "player_camera.h"
#include "light.h"
#include "texture.h"
#include "key_logger.h"
#include "pad_logger.h"
#include "input_manager.h"
#include "ray.h"
#include "collision.h"

#include "shader_shadow_map.h"
#include "shader3d.h"
#include "light_camera.h"

//--------------------------------------
// オブジェクトマネージャーヘッダ
//--------------------------------------
#include "stage.h"
#include "prop_manager.h"
#include "player.h"
#include "enemy.h"
#include "enemy_bullet.h"
#include "bullet.h"
#include "billboard.h"
#include "bullet_hit_effect.h"
#include "blood_effect.h"
#include "trail.h"
#include "skybox.h"
#include "sprite_anim.h"
#include "blade.h"
#include "post_process.h"
#include "damage_effect.h"
#include "hp_gauge.h"
#include "score.h"
#include "game_timer.h"
#include "combo.h"
#include "hitstop.h"
#include "fever.h"
#include "reticle.h"
#include "enemy_generator.h"
#include "scene.h"
#include "fade.h"

#include "particle_test.h"

#include "sound_manager.h"

#ifdef _DEBUG
#include "debug_renderer.h"
#endif

#include <cstdlib>
#include <ctime>
#include <algorithm>

using namespace DirectX;

//--------------------------------------
// 定数
//--------------------------------------
namespace
{
    constexpr float GAME_TIME_LIMIT = 120.0f;

    constexpr float SCORE_MARGIN_X = 20.0f;
    constexpr float SCORE_MARGIN_Y = 16.0f;
    constexpr int SCORE_DIGITS = 7;
    constexpr float SCORE_DISPLAY_SCALE = 1.8f;  // スコア表示倍率

    // フォント設定（スコア文字幅計算用）
    constexpr float FONT_SRC_WIDTH = 89.0f;      // フォントソース幅（ピクセル）
    constexpr float FONT_DRAW_SCALE = 0.25f;     // フォント描画スケール（1/4サイズ）

    // フェード演出時間
    constexpr double FADE_DURATION_GAME_START = 1.0;  // ゲーム開始時フェード（秒）
    constexpr double FADE_DURATION_TIME_UP = 1.5;     // タイムアップ時フェード（秒）
    constexpr double FADE_DURATION_DEATH = 2.0;       // 死亡時フェード（秒）

    // パーティクルエミッター設定
    constexpr float EMITTER_POSITION_Y = 2.0f;        // エミッター初期Y座標
    constexpr float EMITTER_POSITION_Z = 5.0f;        // エミッター初期Z座標
    constexpr double EMITTER_LIFETIME = 1000.0;       // エミッター寿命

    // ライトカメラ設定
    constexpr float LIGHT_CAMERA_WIDTH = 50.0f;       // ライトカメラ幅
    constexpr float LIGHT_CAMERA_HEIGHT = 50.0f;      // ライトカメラ高さ
    constexpr float LIGHT_CAMERA_FAR = 200.0f;        // ライトカメラ遠方クリップ面

    constexpr XMFLOAT3 PLAYER_CAMERA_OFFSET = { 0.0f, 1.6f, 0.0f };
    constexpr XMFLOAT3 PLAYER_START_POSITION = { 0.0f, 1.0f, 0.0f };
    constexpr XMFLOAT3 PLAYER_START_FRONT = { 0.0f, 0.0f, 1.0f };

    constexpr XMFLOAT3 LIGHT_OFFSET = { 25.0f, 50.0f, -25.0f };
    constexpr XMFLOAT4 LIGHT_DIRECTION = { -0.3f, -0.75f, 0.3f, 0.0f };
    constexpr XMFLOAT4 LIGHT_COLOR = { 1.1f, 1.08f, 1.0f, 1.0f };
    constexpr XMFLOAT3 AMBIENT_COLOR = { 0.5f, 0.5f, 0.55f };

#ifdef _DEBUG
    constexpr XMFLOAT3 DEBUG_CAMERA_POSITION = { 0.0f, 20.0f, -40.0f };
    constexpr XMFLOAT3 DEBUG_CAMERA_FRONT = { 0.0f, -0.3f, 1.0f };
    constexpr XMFLOAT3 DEBUG_CAMERA_UP = { 0.0f, 1.0f, 0.0f };
#endif
}

//--------------------------------------
// グローバル変数
//--------------------------------------
static float g_GameElapsedTime = 0.0f;

static bool g_IsGameOver = false;
static bool g_IsTransitioning = false;

static float g_ScoreDrawX = 0.0f;
static float g_ScoreDrawY = 0.0f;

static NormalEmitter* g_pTestEmitter = nullptr;

#ifdef _DEBUG
static bool g_IsDebugCamera = false;
static bool g_IsDebugDraw = false;
static bool g_IsTimeFrozen = false;  // デバッグ用タイムフリーズ
#endif

//--------------------------------------
// 内部関数プロトタイプ
//--------------------------------------
static void UpdateSystems(float dt);
static void UpdateObjects(float dt);
static void UpdateLightCamera();
static void UpdateCollisions();
static void CheckGameEnd();

static void DrawScene();

#ifdef _DEBUG
static void ProcessDebugInput();
static void DrawDebugOverlay(const XMFLOAT4X4& mtxView, const XMFLOAT4X4& mtxProj);
#endif

//--------------------------------------
// ビュー・プロジェクション行列の一元取得
// → デバッグカメラ / プレイヤーカメラの分割を1箇所に集約
//--------------------------------------
static void GetActiveMatrices(XMFLOAT4X4& outView, XMFLOAT4X4& outProj)
{
#ifdef _DEBUG
    if (g_IsDebugCamera)
    {
        outView = Camera_GetViewMatrix();
        outProj = Camera_GetPerspectiveMatrix();
        return;
    }
#endif
    outView = PLCamera_GetViewMatrix();
    outProj = PLCamera_GetPerspectiveMatrix();
}

// ゲーム初期化
void Game_Initialize()
{
    srand(static_cast<unsigned int>(time(nullptr)));

    InputManager_Initialize();
    SpriteAnim_Initialize();

    Stage_Initialize();
    PropManager_Initialize();

    // カメラ初期化（定数バッファ作成のため、リリースでも必要）
    Camera_Initialize();

#ifdef _DEBUG
    Camera_Initialize(DEBUG_CAMERA_POSITION, DEBUG_CAMERA_FRONT, DEBUG_CAMERA_UP);
#endif

    PLCamera_Initialize(PLAYER_CAMERA_OFFSET);

    Player_Initialize(PLAYER_START_POSITION, PLAYER_START_FRONT);
    Enemy_Initialize();
    EnemyBullet_Initialize();
    Bullet_Initialize();
    Billboard_Initialize();
    BulletHItEffect_Initialize();
    BloodEffect_Initialize();
    Trail_Initialize();
    Skybox_Initialize();
    Blade_Initialize();

    PostProcess_Initialize();
    DamageEffect_Initialize();

    HpGauge_Initialize();
    Reticle_Initialize();

    const float screenWidth = static_cast<float>(Direct3D_GetBackBufferWidth());
    const float scoreCharW = FONT_SRC_WIDTH * FONT_DRAW_SCALE * SCORE_DISPLAY_SCALE;
    g_ScoreDrawX = screenWidth - SCORE_MARGIN_X - (scoreCharW * SCORE_DIGITS);
    g_ScoreDrawY = SCORE_MARGIN_Y;
    Score_Initialize(g_ScoreDrawX, g_ScoreDrawY, SCORE_DIGITS);

    GameTimer_Initialize(GAME_TIME_LIMIT);
    Combo_Initialize();
    HitStop_Initialize();
    Fever_Initialize();

#ifdef _DEBUG
    DebugRenderer::Initialize();
#endif

    g_GameElapsedTime = 0.0f;
    g_IsGameOver = false;
    g_IsTransitioning = false;

#ifdef _DEBUG
    g_IsDebugCamera = false;
    g_IsDebugDraw = false;
#endif

    EnemyGenerator_Initialize();

    SoundManager_PlayBGM(SOUND_BGM_GAME);
    Fade_Start(FADE_DURATION_GAME_START, false);

    // パーティクルエミッター初期化
    const XMVECTOR emitterPos = XMVectorSet(0.0f, EMITTER_POSITION_Y, EMITTER_POSITION_Z, 0.0f);
    g_pTestEmitter = new NormalEmitter(0, emitterPos, EMITTER_LIFETIME, true);
}

// ゲーム終了
void Game_Finalize()
{
    delete g_pTestEmitter;
    g_pTestEmitter = nullptr;

#ifdef _DEBUG
    DebugRenderer::Finalize();
#endif

    Fever_Finalize();
    HitStop_Finalize();
    Combo_Finalize();
    GameTimer_Finalize();
    Score_Finalize();
    Reticle_Finalize();
    HpGauge_Finalize();
    DamageEffect_Finalize();
    PostProcess_Finalize();
    Blade_Finalize();
    PropManager_Finalize();
    Stage_Finalize();

    Skybox_Finalize();
    Trail_Finalize();
    BloodEffect_Finalize();
    BulletHItEffect_Finalize();
    Billboard_Finalize();
    Bullet_Finalize();
    EnemyGenerator_Finalize();
    EnemyBullet_Finalize();
    Enemy_Finalize();
    Player_Finalize();

    Camera_Finalize();
    PLCamera_Finalize();

    InputManager_Finalize();
    SpriteAnim_Finalize();
}

// ゲーム更新
void Game_Update(double elapsed_time)
{
    const float raw_dt = static_cast<float>(elapsed_time);

    CheckGameEnd();

    if (g_IsTransitioning)
    {
        if (Fade_GetState() == FADE_STATE::FINISHED_OUT)
        {
            Scene_Change(Scene::RESULT);
        }
        return;
    }

    if (g_IsGameOver)
    {
        HpGauge_Update(raw_dt);
        Score_Update(raw_dt);
        return;
    }

    // ヒットストップ更新（実時間ベース）
    HitStop_Update(raw_dt);

#ifdef _DEBUG
    // デバッグ入力はフリーズ中も常に処理（解除操作のため）
    ProcessDebugInput();

    // タイムフリーズ中: デバッグカメラだけ更新して他は全スキップ
    if (g_IsTimeFrozen)
    {
        if (g_IsDebugCamera)
        {
            Camera_Update(raw_dt);
        }
        return;
    }
#endif

    // タイムスケール適用（ヒットストップ中はほぼ0になる）
    const float dt = raw_dt * HitStop_GetTimeScale();

    g_GameElapsedTime += dt;

    UpdateSystems(dt);
    UpdateObjects(dt);
    EnemyGenerator_Update(dt, g_GameElapsedTime);
    UpdateLightCamera();
    UpdateCollisions();

    PostProcess_Update(dt);
    DamageEffect_Update(dt);

    HpGauge_Update(dt);
    Score_Update(dt);

    // GameTimerは実時間で更新（ヒットストップの影響を受けない）
    GameTimer_Update(raw_dt);

    Combo_Update(dt);
    Fever_Update(dt);
    Reticle_Update(dt);
    SoundManager_Update(dt);
}

//--------------------------------------
// システム更新
//--------------------------------------
static void UpdateSystems(float dt)
{
    SpriteAnim_Update(dt);
    Blade_Update(dt);

#ifdef _DEBUG
    if (g_IsDebugCamera)
    {
        Camera_Update(dt);
    }
    else
#endif
    {
        PLCamera_Update(dt);
        Player_Update(dt);
    }
}

//--------------------------------------
// オブジェクト更新
//--------------------------------------
static void UpdateObjects(float dt)
{
    Stage_Update(dt);

    Enemy_ProcessSliceResults();

    PropManager_Update(dt);

    Enemy_Update(dt);
    EnemyBullet_Update(dt);
    Light_Update(dt);
    Bullet_Update(dt);
    BulletHItEffect_Update(dt);
    BloodEffect_Update(dt);
    Trail_Update(dt);

    if (g_pTestEmitter)
    {
        g_pTestEmitter->Update(dt);
    }
}

//--------------------------------------
// ライトカメラ更新
//--------------------------------------
static void UpdateLightCamera()
{
    const XMFLOAT3 playerPos = Player_GetPosition();

    LightCamera_SetPosition({
        playerPos.x + LIGHT_OFFSET.x,
        LIGHT_OFFSET.y,
        playerPos.z + LIGHT_OFFSET.z
                            });
    LightCamera_SetFront({ LIGHT_DIRECTION.x, LIGHT_DIRECTION.y, LIGHT_DIRECTION.z });
    LightCamera_SetRange(LIGHT_CAMERA_WIDTH, LIGHT_CAMERA_HEIGHT, 1.0f, LIGHT_CAMERA_FAR);
}

//--------------------------------------
// 衝突判定処理
//--------------------------------------
static void UpdateCollisions()
{
    for (int i = Bullet_GetBulletCount() - 1; i >= 0; i--)
    {
        const XMFLOAT3 bulletPos = Bullet_GetPosition(i);
        if (!Stage_IsInsidePlayArea(bulletPos))
        {
            BulletHItEffect_Create(bulletPos);
            Bullet_Destory(i);
        }
    }
}

//--------------------------------------
// ゲーム終了判定
//--------------------------------------
static void CheckGameEnd()
{
    if (g_IsTransitioning) return;

    bool shouldEnd = false;
    double fadeDuration = 0.0;

    if (GameTimer_IsTimeUp())
    {
        shouldEnd = true;
        fadeDuration = FADE_DURATION_TIME_UP;
    }
    else if (Player_GetHP() <= 0)
    {
        shouldEnd = true;
        fadeDuration = FADE_DURATION_DEATH;
    }

    if (shouldEnd)
    {
        g_IsGameOver = true;
        g_IsTransitioning = true;
        Score_SaveFinal();
        Fade_Start(fadeDuration, true);
    }
}

#ifdef _DEBUG
//--------------------------------------
// デバッグ入力処理
//--------------------------------------
static void ProcessDebugInput()
{
    if (KeyLogger_IsTrigger(KK_L))
    {
        g_IsDebugCamera = !g_IsDebugCamera;
    }

    if (KeyLogger_IsTrigger(KK_O))
    {
        g_IsDebugDraw = !g_IsDebugDraw;
    }

    if (KeyLogger_IsTrigger(KK_P))
    {
        EnemyGenerator_SpawnOne(ENEMY_TYPE_GROUND);
    }

    if (KeyLogger_IsTrigger(KK_U))
    {
        EnemyGenerator_SpawnOne(ENEMY_TYPE_FLYING);
    }

    // タイムフリーズ切り替え（Tキー）
    if (KeyLogger_IsTrigger(KK_T))
    {
        g_IsTimeFrozen = !g_IsTimeFrozen;
    }
}
#endif

// シャドウマップ描画
void Game_DrawShadow()
{
    Stage_DrawShadow();
    PropManager_DrawShadow();
    Player_DrawShadow();
    Enemy_DrawShadow();
    Blade_DrawShadow();
}

// ゲーム描画
void Game_Draw()
{
    PostProcess_BeginScene();
    DrawScene();
    PostProcess_EndScene();

    Direct3D_DepthStencilStateDepthIsEnable(false);
    Direct3D_SetBlendState(BlendMode::Alpha);

    // ダメージエフェクト（UIの下に描画）
    DamageEffect_Draw();

    // フィーバーモード画色ビネット（Tier2+）
    Fever_Draw();

    Reticle_Draw();
    HpGauge_Draw();
    Score_Draw(g_ScoreDrawX, g_ScoreDrawY, SCORE_DISPLAY_SCALE, SCORE_DISPLAY_SCALE);
    GameTimer_Draw();
    Combo_Draw();

    Direct3D_DepthStencilStateDepthIsEnable(true);

#ifdef _DEBUG
    XMFLOAT4X4 mtxView, mtxProj;
    GetActiveMatrices(mtxView, mtxProj);
    DrawDebugOverlay(mtxView, mtxProj);
#endif
}

//--------------------------------------
// シーン描画
//--------------------------------------
static void DrawScene()
{
    Direct3D_DepthStencilStateDepthIsEnable(true);

    XMFLOAT4X4 mtxView, mtxProj;
    GetActiveMatrices(mtxView, mtxProj);

    Camera_SetMatrix(XMLoadFloat4x4(&mtxView), XMLoadFloat4x4(&mtxProj));
    Billboard_SetViewMatrix(mtxView);

    Light_SetDirectional_World(LIGHT_DIRECTION, LIGHT_COLOR);
    Light_SetAmbient(AMBIENT_COLOR);

    // スカイボックス（深度テスト停止）
    Direct3D_DepthStencilStateDepthIsEnable(false);
    Skybox_SetPosition(Player_GetPosition());
    Skybox_Draw();

    // メインシェーダーに復帰（スカイボックスがUnlitシェーダーを使うため）
    Shader3D_Begin();

    // 不透明オブジェクト（Shader3Dパス）
    Stage_Draw();
    PropManager_Draw();

    // 敵（ShaderSkinnedは自前でBeginを呼ぶ）
    Enemy_Draw();

    // 敵弾丸（内部でビルボードに切替されるため、前後でShader3D_Beginが必要）
    Shader3D_Begin();
    EnemyBullet_Draw();

    // プレイヤー弾丸・刀（EnemyBulletのビルボード後なのでShader3Dに復帰）
    Shader3D_Begin();
    Bullet_Draw();
    Blade_Draw();

    // 半透明・エフェクト（ビルボードシェーダーは各自でBeginを呼ぶ）
    BulletHItEffect_Draw();
    BloodEffect_Draw();
    Trail_Draw();

    // パーティクル描画（加算合成・深度書き込み無効化・バッチ描画）
    if (g_pTestEmitter)
    {
        Direct3D_SetDepthWriteEnable(false);
        Direct3D_SetBlendState(BlendMode::Add);

        Billboard_BeginBatch();
        g_pTestEmitter->Draw();
        Billboard_EndBatch();

        Direct3D_SetDepthWriteEnable(true);
        Direct3D_SetBlendState(BlendMode::Alpha);
    }
}

#ifdef _DEBUG
//--------------------------------------
// デバッグオーバーレイ描画
//--------------------------------------
static void DrawDebugOverlay(const XMFLOAT4X4& mtxView, const XMFLOAT4X4& mtxProj)
{
    if (g_IsDebugCamera)
    {
        Camera_DebugDraw();
    }

    if (g_IsDebugDraw)
    {
        Stage_DrawDebug();
        EnemyGenerator_DrawDebug();
        PropManager_DrawDebug();
        Player_DrawDebug();
        Enemy_DrawDebug();
        EnemyBullet_DrawDebug();
        Blade_DebugDraw();

        DebugRenderer::Render(mtxView, mtxProj);
    }
}
#endif