/****************************************
 * @file stage.cpp
 * @brief ステージ管理の実装（MeshField統合版）
 * @author Natsume Shidara
 * @date 2026/01/13
 * @update 2026/02/23 - スポーン関連をenemy_generatorへ分離
 ****************************************/

#include "stage.h"
#include "meshfield.h"
#include "direct3d.h"
#include "texture.h"
#include "model.h"
#include "debug_renderer.h"
#ifdef USE_ASSET_DLL
#include "asset_dll.h"
#include "resource.h"
#endif

#include <algorithm>

using namespace DirectX;

// 定数
namespace
{
    // モデルパス
    constexpr const char* BOX_MODEL_PATH = "assets/fbx/BOX.fbx";

    // テクスチャパス
    constexpr const wchar_t* WALL_TEXTURE_PATH = L"assets/wall.png";

    // 壁の数
    constexpr int WALL_COUNT = 4;

    // デバッグ描画の地面からの浮き高さ
    constexpr float DEBUG_LINE_HEIGHT = 0.2f;

    // プレイエリアクランプ時の壁からの余白
    constexpr float PLAY_AREA_CLAMP_MARGIN = 0.5f;
}

// 壁構造体
struct Wall
{
    MODEL* pModel = nullptr;
    XMFLOAT3 position = { 0, 0, 0 };
    XMFLOAT3 scale = { 1, 1, 1 };
};

// 内部変数
static Wall g_Walls[WALL_COUNT];
static AABB g_WallAABBs[WALL_COUNT];
static int g_WallTexId = -1;

/**
 * @brief Wall構造体からAABBを計算（BOX.fbxは-0.5~+0.5の単位立方体）
 */
static AABB ComputeWallAABB(const Wall& wall)
{
    float halfX = wall.scale.x * 0.5f;
    float halfY = wall.scale.y * 0.5f;
    float halfZ = wall.scale.z * 0.5f;
    return {
        { wall.position.x - halfX, wall.position.y - halfY, wall.position.z - halfZ },
        { wall.position.x + halfX, wall.position.y + halfY, wall.position.z + halfZ }
    };
}

// 内部関数

/** @brief 壁モデルの描画 */
static void DrawWall(const Wall& wall)
{
    if (!wall.pModel) return;

    XMMATRIX mtxScale = XMMatrixScaling(wall.scale.x, wall.scale.y, wall.scale.z);
    XMMATRIX mtxTrans = XMMatrixTranslation(wall.position.x, wall.position.y, wall.position.z);
    XMMATRIX mtxWorld = mtxScale * mtxTrans;

    ModelDraw(wall.pModel, mtxWorld);
}

/** @brief 壁モデルのシャドウマップ描画 */
static void DrawWallShadow(const Wall& wall)
{
    if (!wall.pModel) return;

    XMMATRIX mtxScale = XMMatrixScaling(wall.scale.x, wall.scale.y, wall.scale.z);
    XMMATRIX mtxTrans = XMMatrixTranslation(wall.position.x, wall.position.y, wall.position.z);
    XMMATRIX mtxWorld = mtxScale * mtxTrans;

    ModelDrawShadow(wall.pModel, mtxWorld);
}

// 初期化
void Stage_Initialize()
{
    using namespace StageConfig;

    // MeshField初期化
    MeshField_Initialize(Direct3D_GetDevice(), Direct3D_GetContext());

    // テクスチャ読み込み
#ifdef USE_ASSET_DLL
    {
        const void* pData = nullptr;
        size_t dataSize = 0;
        if (AssetDLL_GetData(IDR_TEX_WALL, &pData, &dataSize))
            g_WallTexId = Texture_LoadFromMemory(pData, dataSize, L"wall_tex");
    }
#else
    g_WallTexId = Texture_Load(WALL_TEXTURE_PATH);
#endif

    // 壁モデル生成（北・南・東・西）
    float wallBaseY = WALL_HEIGHT * 0.5f;

    // 北壁（+Z方向）
    g_Walls[0].pModel = ModelLoad(BOX_MODEL_PATH, 1.0f, false, WALL_TEXTURE_PATH);
    g_Walls[0].position = { 0.0f, wallBaseY, STAGE_DEPTH * 0.5f };
    g_Walls[0].scale = { STAGE_WIDTH + WALL_THICKNESS * 2, WALL_HEIGHT, WALL_THICKNESS };

    // 南壁（-Z方向）
    g_Walls[1].pModel = ModelLoad(BOX_MODEL_PATH, 1.0f, false, WALL_TEXTURE_PATH);
    g_Walls[1].position = { 0.0f, wallBaseY, -STAGE_DEPTH * 0.5f };
    g_Walls[1].scale = { STAGE_WIDTH + WALL_THICKNESS * 2, WALL_HEIGHT, WALL_THICKNESS };

    // 東壁（+X方向）
    g_Walls[2].pModel = ModelLoad(BOX_MODEL_PATH, 1.0f, false, WALL_TEXTURE_PATH);
    g_Walls[2].position = { STAGE_WIDTH * 0.5f, wallBaseY, 0.0f };
    g_Walls[2].scale = { WALL_THICKNESS, WALL_HEIGHT, STAGE_DEPTH };

    // 西壁（-X方向）
    g_Walls[3].pModel = ModelLoad(BOX_MODEL_PATH, 1.0f, false, WALL_TEXTURE_PATH);
    g_Walls[3].position = { -STAGE_WIDTH * 0.5f, wallBaseY, 0.0f };
    g_Walls[3].scale = { WALL_THICKNESS, WALL_HEIGHT, STAGE_DEPTH };

    // 壁AABBの事前計算
    for (int i = 0; i < WALL_COUNT; i++)
    {
        g_WallAABBs[i] = ComputeWallAABB(g_Walls[i]);
    }
}

// 終了処理
void Stage_Finalize()
{
    // MeshField終了
    MeshField_Finalize();

    // 壁モデル解放
    for (int i = 0; i < WALL_COUNT; i++)
    {
        if (g_Walls[i].pModel)
        {
            ModelRelease(g_Walls[i].pModel);
            g_Walls[i].pModel = nullptr;
        }
    }
}

// 更新
void Stage_Update(float dt)
{
    MeshField_Update(dt);
}

// 描画
void Stage_Draw()
{
    // 地形（MeshField）描画
    XMMATRIX mtxWorld = XMMatrixIdentity();
    MeshField_Draw(mtxWorld);

    // 壁描画
    for (int i = 0; i < WALL_COUNT; i++)
    {
        DrawWall(g_Walls[i]);
    }
}

// シャドウ描画
void Stage_DrawShadow()
{
    // 地形シャドウ
    XMMATRIX mtxWorld = XMMatrixIdentity();
    //MeshField_DrawShadow(mtxWorld);

    // 壁シャドウ
    for (int i = 0; i < WALL_COUNT; i++)
    {
        DrawWallShadow(g_Walls[i]);
    }
}

// デバッグ描画
void Stage_DrawDebug()
{
    using namespace StageConfig;

    // プレイエリアの境界を表示
    XMFLOAT4 color = { 0.0f, 1.0f, 0.0f, 1.0f };

    // 床の四隅（高さを地形に合わせる）
    float h00 = MeshField_GetHeight(PLAY_AREA_MIN_X, PLAY_AREA_MIN_Z) + DEBUG_LINE_HEIGHT;
    float h10 = MeshField_GetHeight(PLAY_AREA_MAX_X, PLAY_AREA_MIN_Z) + DEBUG_LINE_HEIGHT;
    float h11 = MeshField_GetHeight(PLAY_AREA_MAX_X, PLAY_AREA_MAX_Z) + DEBUG_LINE_HEIGHT;
    float h01 = MeshField_GetHeight(PLAY_AREA_MIN_X, PLAY_AREA_MAX_Z) + DEBUG_LINE_HEIGHT;

    XMFLOAT3 corners[4] = {
        { PLAY_AREA_MIN_X, h00, PLAY_AREA_MIN_Z },
        { PLAY_AREA_MAX_X, h10, PLAY_AREA_MIN_Z },
        { PLAY_AREA_MAX_X, h11, PLAY_AREA_MAX_Z },
        { PLAY_AREA_MIN_X, h01, PLAY_AREA_MAX_Z }
    };

    for (int i = 0; i < 4; i++)
    {
        int next = (i + 1) % 4;
        DebugRenderer::DrawLine(corners[i], corners[next], color);
    }
}

// プレイエリア内判定
bool Stage_IsInsidePlayArea(const XMFLOAT3& pos)
{
    using namespace StageConfig;

    return pos.x >= PLAY_AREA_MIN_X && pos.x <= PLAY_AREA_MAX_X &&
        pos.z >= PLAY_AREA_MIN_Z && pos.z <= PLAY_AREA_MAX_Z;
}

// プレイエリア内にクランプ
XMFLOAT3 Stage_ClampToPlayArea(const XMFLOAT3& pos)
{
    using namespace StageConfig;

    XMFLOAT3 result = pos;
    result.x = std::clamp(result.x, PLAY_AREA_MIN_X + PLAY_AREA_CLAMP_MARGIN, PLAY_AREA_MAX_X - PLAY_AREA_CLAMP_MARGIN);
    result.z = std::clamp(result.z, PLAY_AREA_MIN_Z + PLAY_AREA_CLAMP_MARGIN, PLAY_AREA_MAX_Z - PLAY_AREA_CLAMP_MARGIN);

    return result;
}

// 地形高さ取得
float Stage_GetTerrainHeight(float x, float z)
{
    return MeshField_GetHeight(x, z);
}

// 壁AABB取得
int Stage_GetWallCount()
{
    return WALL_COUNT;
}

const AABB& Stage_GetWallAABB(int index)
{
    return g_WallAABBs[index];
}