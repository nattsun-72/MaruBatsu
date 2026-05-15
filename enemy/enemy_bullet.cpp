/****************************************
 * @file enemy_bullet.cpp
 * @brief 敵の弾の実装
 * @author Natsume Shidara
 * @date 2026/01/10
 * @update 2026/02/19 - ビルボード描画・トレイル調整
 ****************************************/

#include "enemy_bullet.h"
#include "player.h"
#include "model.h"
#include "debug_renderer.h"
#include "trail.h"
#include "billboard.h"
#include "texture.h"
#include "direct3d.h"
#include "ray.h"
#include "collision.h"
#include "slicer.h"
#include "prop_manager.h"
#include "combo.h"
#include "hitstop.h"
#include "bullet_hit_effect.h"
#include "sound_manager.h"
#include "stage.h"
#include "game/score.h"
#ifdef USE_ASSET_DLL
#include "asset_dll.h"
#include "resource.h"
#endif

#include <vector>
#include <algorithm>
#include <cmath>

using namespace DirectX;

// 弾丸ビジュアル設定
namespace
{
    constexpr const char* BULLET_MODEL_PATH = "assets/fbx/bullet.fbx";
    constexpr float BULLET_MODEL_SCALE = 25.4f;        // 弾丸モデルのスケール

    constexpr const wchar_t* BULLET_TEXTURE_PATH = L"assets/effect000.jpg";
    constexpr float BULLET_GLOW_SIZE = 0.6f;          // 弾丸グロウサイズ
    constexpr float TRAIL_SIZE = 0.4f;                // トレイルサイズ（細さ）
    constexpr float TRAIL_LIFETIME = 0.3f;            // トレイル寿命（短め）

    // 弾丸切断パラメータ
    constexpr float BULLET_MASS = 1.0f;               // 弾丸の質量
    constexpr float BULLET_DEBRIS_LIFETIME = 2.0f;    // 切断破片の寿命（秒）
    constexpr float BULLET_SEPARATION_SPEED = 4.0f;   // 切断面に沿った分離速度
    constexpr float BULLET_OFFSET_DISTANCE = 0.1f;    // 切断片の初期オフセット

    // トレイル色（弾丸軌跡用）
    constexpr float TRAIL_COLOR_R = 1.0f;
    constexpr float TRAIL_COLOR_G = 0.5f;
    constexpr float TRAIL_COLOR_B = 0.15f;
    constexpr float TRAIL_COLOR_A = 0.25f;

    // 弾丸ベクタの初期予約サイズ
    constexpr int BULLET_RESERVE_SIZE = 256;

    // 進行方向が真上に近いかの閾値
    constexpr float PARALLEL_UP_THRESHOLD = 0.99f;

    // グロウエフェクト色（ビルボード加算合成用）
    constexpr float GLOW_COLOR_R = 1.0f;
    constexpr float GLOW_COLOR_G = 0.4f;
    constexpr float GLOW_COLOR_B = 0.05f;
    constexpr float GLOW_COLOR_A = 0.5f;

    // 切断片の質量比率（弾丸質量に対する各片の割合）
    constexpr float BULLET_SLICE_MASS_RATIO = 0.5f;
}

static int g_BulletTexId = -1;

// 弾クラス（内部用）
class EnemyBulletInternal
{
private:
    XMFLOAT3 m_Position;
    XMFLOAT3 m_Direction;
    float m_LifeTime;
    bool m_Active;

public:
    EnemyBulletInternal(const XMFLOAT3& position, const XMFLOAT3& direction)
        : m_Position(position)
        , m_LifeTime(0.0f)
        , m_Active(true)
    {
        // 方向を正規化
        XMStoreFloat3(&m_Direction, XMVector3Normalize(XMLoadFloat3(&direction)));
    }

    void Update(float dt)
    {
        if (!m_Active) return;

        // 移動
        XMVECTOR pos = XMLoadFloat3(&m_Position);
        XMVECTOR dir = XMLoadFloat3(&m_Direction);
        pos += dir * EnemyBulletConfig::SPEED * dt;
        XMStoreFloat3(&m_Position, pos);

        // トレイル生成（弾の軌跡として小さめ）
        Trail_Create(m_Position, { TRAIL_COLOR_R, TRAIL_COLOR_G, TRAIL_COLOR_B, TRAIL_COLOR_A }, TRAIL_SIZE, TRAIL_LIFETIME);

        // 寿命チェック
        m_LifeTime += dt;
        if (m_LifeTime >= EnemyBulletConfig::MAX_LIFETIME)
        {
            m_Active = false;
            return;
        }

        // プレイエリア外に出たら消滅
        if (!Stage_IsInsidePlayArea(m_Position))
        {
            m_Active = false;
            return;
        }

        // プレイヤーとの当たり判定
        XMFLOAT3 playerPos = Player_GetPosition();
        playerPos.y += 1.0f;  // プレイヤーの中心

        XMVECTOR toPlayer = XMLoadFloat3(&playerPos) - pos;
        float distance = XMVectorGetX(XMVector3Length(toPlayer));

        float hitRadius = EnemyBulletConfig::RADIUS + EnemyBulletConfig::PLAYER_HIT_RADIUS;
        if (distance < hitRadius)
        {
            // ヒット
            Player_TakeDamage(EnemyBulletConfig::DAMAGE);
            m_Active = false;
            return;
        }

    }

    const XMFLOAT3& GetPosition() const { return m_Position; }
    const XMFLOAT3& GetDirection() const { return m_Direction; }
    bool IsActive() const { return m_Active; }
    void Deactivate() { m_Active = false; }

    AABB GetAABB() const
    {
        AABB aabb;
        float r = EnemyBulletConfig::RADIUS;
        aabb.min = { m_Position.x - r, m_Position.y - r, m_Position.z - r };
        aabb.max = { m_Position.x + r, m_Position.y + r, m_Position.z + r };
        return aabb;
    }
};

// 管理用変数
static std::vector<EnemyBulletInternal*> g_Bullets;
static MODEL* g_pBulletModel = nullptr;

// グローバル管理関数

void EnemyBullet_Initialize()
{
    g_Bullets.clear();
    g_Bullets.reserve(BULLET_RESERVE_SIZE);

    g_pBulletModel = ModelLoad(BULLET_MODEL_PATH, BULLET_MODEL_SCALE);
#ifdef USE_ASSET_DLL
    {
        const void* pData = nullptr;
        size_t dataSize = 0;
        if (AssetDLL_GetData(IDR_TEX_EFFECT000, &pData, &dataSize))
            g_BulletTexId = Texture_LoadFromMemory(pData, dataSize, L"bullet_effect");
    }
#else
    g_BulletTexId = Texture_Load(BULLET_TEXTURE_PATH);
#endif
}

void EnemyBullet_Finalize()
{
    for (auto* pBullet : g_Bullets)
    {
        delete pBullet;
    }
    g_Bullets.clear();

    if (g_pBulletModel)
    {
        ModelRelease(g_pBulletModel);
        g_pBulletModel = nullptr;
    }

    g_BulletTexId = -1;
}

void EnemyBullet_Update(float dt)
{
    // 全弾を更新
    for (auto* pBullet : g_Bullets)
    {
        pBullet->Update(dt);
    }

    // 非アクティブな弾を削除
    for (int i = static_cast<int>(g_Bullets.size()) - 1; i >= 0; i--)
    {
        if (!g_Bullets[i]->IsActive())
        {
            delete g_Bullets[i];
            g_Bullets.erase(g_Bullets.begin() + i);
        }
    }
}

// 弾丸のワールド行列を構築
// （描画コードと同じ回転+平行移動ロジック）
static XMMATRIX BuildBulletWorldMatrix(const EnemyBulletInternal* pBullet)
{
    XMFLOAT3 pos = pBullet->GetPosition();
    XMFLOAT3 dir = pBullet->GetDirection();

    // 進行方向に向けた回転行列を構築
    // モデルの弾軸は-Y軸方向を向いているため、
    // ローカル-Yを進行方向に合わせる
    XMVECTOR vFwd = XMVector3Normalize(XMLoadFloat3(&dir));
    XMVECTOR vRefUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    // 進行方向がほぼ真上・真下の場合、参照Upを変更
    float dotUp = fabsf(XMVectorGetX(XMVector3Dot(vFwd, vRefUp)));
    if (dotUp > PARALLEL_UP_THRESHOLD)
    {
        vRefUp = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
    }

    // ローカル-Y → 進行方向に対応させる回転行列
    XMVECTOR vNewY = XMVectorNegate(vFwd);
    XMVECTOR vRight = XMVector3Normalize(XMVector3Cross(vNewY, vRefUp));
    XMVECTOR vNewZ = XMVector3Cross(vRight, vNewY);

    XMMATRIX rotMat = XMMATRIX(
        vRight,
        vNewY,
        vNewZ,
        XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f)
    );

    return rotMat * XMMatrixTranslation(pos.x, pos.y, pos.z);
}

// 弾丸の体積を推定（AABB近似）
static float ComputeBulletVolume(const EnemyBulletInternal* pBullet)
{
    AABB aabb = pBullet->GetAABB();
    float sx = aabb.max.x - aabb.min.x;
    float sy = aabb.max.y - aabb.min.y;
    float sz = aabb.max.z - aabb.min.z;
    return sx * sy * sz;
}

void EnemyBullet_Draw()
{
    // 1. 弾丸モデル描画（不透明パス）
    if (g_pBulletModel)
    {
        for (const auto* pBullet : g_Bullets)
        {
            if (!pBullet->IsActive()) continue;

            XMMATRIX world = BuildBulletWorldMatrix(pBullet);
            ModelDraw(g_pBulletModel, world);
        }
    }

    // 2. グロウエフェクト（加算合成のビルボード）
    if (g_BulletTexId >= 0)
    {
        Direct3D_SetDepthWriteEnable(false);
        Direct3D_SetBlendState(BlendMode::Add);

        Billboard_BeginBatch();
        for (const auto* pBullet : g_Bullets)
        {
            if (!pBullet->IsActive()) continue;

            XMFLOAT3 pos = pBullet->GetPosition();
            XMFLOAT2 pivot = { 0.0f, 0.0f };

            // 弾丸周囲のグロウ（オレンジ）
            Billboard_DrawBatch(g_BulletTexId, pos,
                { BULLET_GLOW_SIZE, BULLET_GLOW_SIZE }, pivot,
                { GLOW_COLOR_R, GLOW_COLOR_G, GLOW_COLOR_B, GLOW_COLOR_A });
        }
        Billboard_EndBatch();

        Direct3D_SetBlendState(BlendMode::Alpha);
        Direct3D_SetDepthWriteEnable(true);
    }
}

void EnemyBullet_DrawDebug()
{
    for (const auto* pBullet : g_Bullets)
    {
        if (!pBullet->IsActive()) continue;

        Sphere sphere;
        sphere.center = pBullet->GetPosition();
        sphere.radius = EnemyBulletConfig::RADIUS;
        DebugRenderer::DrawSphere(sphere, { 1.0f, 0.3f, 0.0f, 1.0f });

        // 進行方向
        XMFLOAT3 start = pBullet->GetPosition();
        XMFLOAT3 end;
        XMStoreFloat3(&end, XMLoadFloat3(&start) + XMLoadFloat3(&pBullet->GetDirection()) * 1.0f);
        DebugRenderer::DrawLine(start, end, { 1.0f, 1.0f, 0.0f, 1.0f });
    }
}

void EnemyBullet_Create(const XMFLOAT3& position, const XMFLOAT3& direction)
{
    g_Bullets.push_back(new EnemyBulletInternal(position, direction));
}

void EnemyBullet_Clear()
{
    for (auto* pBullet : g_Bullets)
    {
        delete pBullet;
    }
    g_Bullets.clear();
}

int EnemyBullet_GetCount()
{
    return static_cast<int>(g_Bullets.size());
}

const XMFLOAT3& EnemyBullet_GetPosition(int index)
{
    static XMFLOAT3 zero = { 0, 0, 0 };
    if (index < 0 || index >= static_cast<int>(g_Bullets.size()))
        return zero;
    return g_Bullets[index]->GetPosition();
}

AABB EnemyBullet_GetAABB(int index)
{
    if (index < 0 || index >= static_cast<int>(g_Bullets.size()))
        return AABB{};
    return g_Bullets[index]->GetAABB();
}

void EnemyBullet_TrySlice(const Ray& ray, const XMFLOAT3& sliceNormal)
{
    for (auto* pBullet : g_Bullets)
    {
        if (!pBullet->IsActive()) continue;

        // 切断判定用にやや大きめのAABBを使用
        AABB aabb = pBullet->GetAABB();
        constexpr float SLICE_MARGIN = 0.5f;
        aabb.min.x -= SLICE_MARGIN;
        aabb.min.y -= SLICE_MARGIN;
        aabb.min.z -= SLICE_MARGIN;
        aabb.max.x += SLICE_MARGIN;
        aabb.max.y += SLICE_MARGIN;
        aabb.max.z += SLICE_MARGIN;

        float hitDist = 0.0f;
        if (Collision_IntersectRayAABB(ray, aabb, &hitDist))
        {
            pBullet->Deactivate();

            // ヒット位置を計算
            XMFLOAT3 rayOrig = ray.GetOrigin();
            XMFLOAT3 rayDir = ray.GetDirection();
            XMFLOAT3 hitPos;
            XMStoreFloat3(&hitPos, XMLoadFloat3(&rayOrig) + XMLoadFloat3(&rayDir) * hitDist);

            // 弾丸の速度ベクトル
            XMFLOAT3 bulletDir = pBullet->GetDirection();
            XMFLOAT3 bulletVel;
            XMStoreFloat3(&bulletVel, XMLoadFloat3(&bulletDir) * EnemyBulletConfig::SPEED);

            // ワールド行列を構築（描画と同じ回転行列）
            XMMATRIX world = BuildBulletWorldMatrix(pBullet);
            XMFLOAT4X4 worldMat;
            XMStoreFloat4x4(&worldMat, world);

            // 同期でメッシュ切断（弾丸は小さいので瞬時に完了）
            std::vector<MeshData> frontMeshes, backMeshes;
            bool sliceSuccess = Slicer::SliceCPUOnly(
                g_pBulletModel, worldMat, hitPos, sliceNormal,
                frontMeshes, backMeshes);

            if (sliceSuccess)
            {
                // 弾丸の体積
                float bulletVolume = ComputeBulletVolume(pBullet);
                XMFLOAT3 bulletPos = pBullet->GetPosition();

                XMVECTOR vPos = XMLoadFloat3(&bulletPos);
                XMVECTOR vVel = XMLoadFloat3(&bulletVel);
                XMVECTOR vNormal = XMLoadFloat3(&sliceNormal);

                // 表裏ペアで共通のgenerationIdを割り当て（衝突無視用）
                int genId = PropManager_AllocateGenerationId();

                // Front片をPropManagerへ
                if (!frontMeshes.empty())
                {
                    XMFLOAT3 frontPos, frontVel;
                    XMStoreFloat3(&frontPos, vPos + vNormal * BULLET_OFFSET_DISTANCE);
                    XMStoreFloat3(&frontVel, vVel + vNormal * BULLET_SEPARATION_SPEED);

                    SlicedPieceParams params;
                    params.meshes = std::move(frontMeshes);
                    params.originalModel = g_pBulletModel;
                    ModelAddRef(g_pBulletModel);
                    params.position = frontPos;
                    params.velocity = frontVel;
                    params.planeNormal = sliceNormal;
                    params.mass = BULLET_MASS * BULLET_SLICE_MASS_RATIO;
                    params.rootVolume = bulletVolume;
                    params.lifeTime = BULLET_DEBRIS_LIFETIME;
                    params.colliderType = ColliderType::Box;
                    params.isFrontSide = true;
                    params.generationId = genId;

                    PropManager_AddSlicedPiece(params);
                }

                // Back片をPropManagerへ
                if (!backMeshes.empty())
                {
                    XMFLOAT3 backPos, backVel;
                    XMStoreFloat3(&backPos, vPos - vNormal * BULLET_OFFSET_DISTANCE);
                    XMStoreFloat3(&backVel, vVel - vNormal * BULLET_SEPARATION_SPEED);

                    SlicedPieceParams params;
                    params.meshes = std::move(backMeshes);
                    params.originalModel = g_pBulletModel;
                    ModelAddRef(g_pBulletModel);
                    params.position = backPos;
                    params.velocity = backVel;
                    params.planeNormal = sliceNormal;
                    params.mass = BULLET_MASS * BULLET_SLICE_MASS_RATIO;
                    params.rootVolume = bulletVolume;
                    params.lifeTime = BULLET_DEBRIS_LIFETIME;
                    params.colliderType = ColliderType::Box;
                    params.isFrontSide = false;
                    params.generationId = genId;

                    PropManager_AddSlicedPiece(params);
                }
            }

            // コンボ・スコア・SE・ヒットストップ（切断成否に関わらず発動）
            Combo_Add(1);
            int bonusScore = Combo_GetBonusScore();
            Score_AddScore(bonusScore);
            SoundManager_PlayComboSE(Combo_GetCount());
            HitStop_Trigger();
            BulletHItEffect_Create(pBullet->GetPosition());

            // AirDash中に弾を切断した場合、AirDash回復
            PlayerState playerState = Player_GetState();
            if (playerState == PlayerState::AirDash || playerState == PlayerState::AirDashCharge)
            {
                Player_RecoverAirDash(1);
            }
        }
    }
}
