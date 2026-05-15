/****************************************
 * @file enemy_generator.cpp
 * @brief 敵スポーン管理
 * @detail ゲーム中の敵生成ロジック
 *         （間隔制御、バッチ生成、位置決定、難易度進行）
 * @author Natsume Shidara
 * @date 2026/02/23
 ****************************************/

#include "enemy_generator.h"
#include "enemy.h"
#include "player.h"
#include "stage.h"
#include "meshfield.h"
#include "debug_renderer.h"

#include <cstdlib>
#include <cmath>
#include <algorithm>

using namespace DirectX;

// 定数
namespace
{
    //--- タイミング ---
    constexpr float SPAWN_INTERVAL_BASE = 1.2f;     // スポーン間隔（初期）
    constexpr float SPAWN_INTERVAL_MIN  = 0.4f;     // 最短間隔（ほぼ常時攻め）
    constexpr float SPAWN_RATE_INCREASE = 0.03f;     // 加速度（秒あたり）

    //--- 数量 ---
    constexpr int   MAX_ENEMIES         = 100;       // 同時最大数
    constexpr int   INITIAL_ENEMY_COUNT = 8;         // 初期生成数
    constexpr int   SPAWN_BATCH_MIN     = 3;         // 1回のスポーンで最少体数
    constexpr int   SPAWN_BATCH_MAX     = 5;         // 1回のスポーンで最大体数

    //--- タイプ ---
    constexpr float FLYING_SPAWN_CHANCE = 0.25f;     // 飛行敵出現確率（25%）

    //--- 位置決定 ---
    constexpr float SPAWN_HEIGHT_GROUND = 5.0f;      // 地上型のスポーン高度（地形より上）
    constexpr float SPAWN_HEIGHT_FLYING = 10.0f;     // 飛行型のスポーン高度
    constexpr float SPAWN_MARGIN        = 5.0f;      // 壁からのマージン
    constexpr float PLAYER_SAFE_RADIUS  = 10.0f;     // プレイヤー周辺のスポーン禁止半径
    constexpr int   MAX_PLACEMENT_ATTEMPTS = 20;     // 位置決定の最大試行回数

    // 円周率×2（角度のランダム生成用）
    constexpr float TWO_PI = 6.28318f;

    // スポーン強制配置時のオフセット距離
    constexpr float SPAWN_FORCED_OFFSET = 5.0f;

    // 飛行型スポーン高度のランダム範囲
    constexpr float FLYING_SPAWN_HEIGHT_RANGE = 5.0f;

    // デバッグ円描画のセグメント数
    constexpr int DEBUG_CIRCLE_SEGMENTS = 16;

    // デバッグ円描画の高さ
    constexpr float DEBUG_CIRCLE_HEIGHT = 0.5f;
}

// 内部変数
static float g_SpawnTimer = 0.0f;

// ヘルパー関数

static float RandomFloat(float min, float max)
{
    float r = static_cast<float>(rand()) / RAND_MAX;
    return min + r * (max - min);
}

// ゲーム経過時間に応じた現在のスポーン間隔を取得
static float GetCurrentSpawnInterval(float gameElapsedTime)
{
    return (std::max)(SPAWN_INTERVAL_BASE - gameElapsedTime * SPAWN_RATE_INCREASE,
                      SPAWN_INTERVAL_MIN);
}

// ランダムな敵タイプを取得
static ENEMY_TYPE GetRandomEnemyType()
{
    const float r = static_cast<float>(rand()) / RAND_MAX;
    return (r < FLYING_SPAWN_CHANCE) ? ENEMY_TYPE_FLYING : ENEMY_TYPE_GROUND;
}

//--------------------------------------
// スポーン位置決定
//--------------------------------------

// 地上型敵のスポーン位置を取得（ランダム）
static XMFLOAT3 GetGroundSpawnPosition(const XMFLOAT3& playerPos)
{
    using namespace StageConfig;

    XMFLOAT3 spawnPos;

    for (int i = 0; i < MAX_PLACEMENT_ATTEMPTS; i++)
    {
        spawnPos.x = RandomFloat(PLAY_AREA_MIN_X + SPAWN_MARGIN, PLAY_AREA_MAX_X - SPAWN_MARGIN);
        spawnPos.z = RandomFloat(PLAY_AREA_MIN_Z + SPAWN_MARGIN, PLAY_AREA_MAX_Z - SPAWN_MARGIN);

        float terrainHeight = MeshField_GetHeight(spawnPos.x, spawnPos.z);
        spawnPos.y = terrainHeight + SPAWN_HEIGHT_GROUND;

        float dx = spawnPos.x - playerPos.x;
        float dz = spawnPos.z - playerPos.z;
        float distSq = dx * dx + dz * dz;

        if (distSq > PLAYER_SAFE_RADIUS * PLAYER_SAFE_RADIUS)
        {
            return spawnPos;
        }
    }

    // 最大試行回数を超えたら、プレイヤーから離れた位置を強制的に選択
    float angle = RandomFloat(0.0f, TWO_PI);
    spawnPos.x = playerPos.x + cosf(angle) * (PLAYER_SAFE_RADIUS + SPAWN_FORCED_OFFSET);
    spawnPos.z = playerPos.z + sinf(angle) * (PLAYER_SAFE_RADIUS + SPAWN_FORCED_OFFSET);

    spawnPos = Stage_ClampToPlayArea(spawnPos);
    spawnPos.y = MeshField_GetHeight(spawnPos.x, spawnPos.z) + SPAWN_HEIGHT_GROUND;

    return spawnPos;
}

// 飛行型敵のスポーン位置を取得（ランダム）
static XMFLOAT3 GetFlyingSpawnPosition(const XMFLOAT3& playerPos)
{
    using namespace StageConfig;

    XMFLOAT3 spawnPos;

    for (int i = 0; i < MAX_PLACEMENT_ATTEMPTS; i++)
    {
        spawnPos.x = RandomFloat(PLAY_AREA_MIN_X + SPAWN_MARGIN, PLAY_AREA_MAX_X - SPAWN_MARGIN);
        spawnPos.y = RandomFloat(SPAWN_HEIGHT_FLYING, SPAWN_HEIGHT_FLYING + FLYING_SPAWN_HEIGHT_RANGE);
        spawnPos.z = RandomFloat(PLAY_AREA_MIN_Z + SPAWN_MARGIN, PLAY_AREA_MAX_Z - SPAWN_MARGIN);

        float dx = spawnPos.x - playerPos.x;
        float dz = spawnPos.z - playerPos.z;
        float distSq = dx * dx + dz * dz;

        if (distSq > PLAYER_SAFE_RADIUS * PLAYER_SAFE_RADIUS)
        {
            return spawnPos;
        }
    }

    // 最大試行回数を超えたら強制配置
    float angle = RandomFloat(0.0f, TWO_PI);
    spawnPos.x = playerPos.x + cosf(angle) * (PLAYER_SAFE_RADIUS + SPAWN_FORCED_OFFSET);
    spawnPos.y = SPAWN_HEIGHT_FLYING;
    spawnPos.z = playerPos.z + sinf(angle) * (PLAYER_SAFE_RADIUS + SPAWN_FORCED_OFFSET);

    spawnPos = Stage_ClampToPlayArea(spawnPos);
    spawnPos.y = SPAWN_HEIGHT_FLYING;  // 高度は維持

    return spawnPos;
}

// タイプに応じたスポーン位置を取得
static XMFLOAT3 GetSpawnPosition(ENEMY_TYPE type, const XMFLOAT3& playerPos)
{
    return (type == ENEMY_TYPE_FLYING)
        ? GetFlyingSpawnPosition(playerPos)
        : GetGroundSpawnPosition(playerPos);
}

// 初期化
void EnemyGenerator_Initialize()
{
    g_SpawnTimer = 0.0f;

    // 初期敵スポーン
    const XMFLOAT3 playerPos = Player_GetPosition();
    for (int i = 0; i < INITIAL_ENEMY_COUNT; i++)
    {
        Enemy_Create(ENEMY_TYPE_GROUND, GetGroundSpawnPosition(playerPos));
    }
}

// 終了処理
void EnemyGenerator_Finalize()
{
    g_SpawnTimer = 0.0f;
}

// 更新（毎フレーム呼び出し）
void EnemyGenerator_Update(float dt, float gameElapsedTime)
{
    g_SpawnTimer += dt;

    if (g_SpawnTimer < GetCurrentSpawnInterval(gameElapsedTime)) return;

    g_SpawnTimer = 0.0f;

    if (Enemy_GetAliveCount() >= MAX_ENEMIES) return;

    // 1回のスポーンで複数体出現
    const int batchCount = SPAWN_BATCH_MIN + rand() % (SPAWN_BATCH_MAX - SPAWN_BATCH_MIN + 1);
    const XMFLOAT3 playerPos = Player_GetPosition();

    for (int i = 0; i < batchCount; i++)
    {
        if (Enemy_GetAliveCount() >= MAX_ENEMIES) break;

        const ENEMY_TYPE enemyType = GetRandomEnemyType();
        Enemy_Create(enemyType, GetSpawnPosition(enemyType, playerPos));
    }
}

// 1体スポーン（デバッグ用）
void EnemyGenerator_SpawnOne(ENEMY_TYPE type)
{
    const XMFLOAT3 playerPos = Player_GetPosition();
    Enemy_Create(type, GetSpawnPosition(type, playerPos));
}

// デバッグ描画
void EnemyGenerator_DrawDebug()
{
#ifdef _DEBUG
    // スポーン禁止エリア表示
    XMFLOAT4 safeColor = { 1.0f, 0.5f, 0.0f, 1.0f };
    for (int i = 0; i < DEBUG_CIRCLE_SEGMENTS; i++)
    {
        float angle1 = (i / static_cast<float>(DEBUG_CIRCLE_SEGMENTS)) * TWO_PI;
        float angle2 = ((i + 1) / static_cast<float>(DEBUG_CIRCLE_SEGMENTS)) * TWO_PI;

        XMFLOAT3 p1 = { cosf(angle1) * PLAYER_SAFE_RADIUS, DEBUG_CIRCLE_HEIGHT, sinf(angle1) * PLAYER_SAFE_RADIUS };
        XMFLOAT3 p2 = { cosf(angle2) * PLAYER_SAFE_RADIUS, DEBUG_CIRCLE_HEIGHT, sinf(angle2) * PLAYER_SAFE_RADIUS };

        DebugRenderer::DrawLine(p1, p2, safeColor);
    }
#endif
}
