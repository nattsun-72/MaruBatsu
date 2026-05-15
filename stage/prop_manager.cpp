/****************************************
 * @file prop_manager.cpp
 * @brief 動的オブジェクト管理（非同期スライス対応版）
 * @details 物理演算オブジェクトの生成、更新、およびマルチスレッドによる切断処理の制御
 * @author Natsume Shidara
 * @date 2026/01/05
 * @update 2026/01/12 - 外部破片追加機能
 ****************************************/

#include "prop_manager.h"
#include "physics_model.h"
#include "rigid_body.h"
#include "model.h"
#include "slicer.h"
#include "slice_task_manager.h"
#include "collision.h"
#include "combo.h"
#include "direct3d.h"
#include "debug_renderer.h"
#include "texture.h"

#include <vector>
#include <cmath>
#include <algorithm>
#include <unordered_map>

#include "frustum.h"
#include "player_camera.h"

using namespace DirectX;

// 定数定義
namespace PropConfig
{
    // 物理演算設定
    constexpr int COLLISION_ITERATIONS = 4;
    constexpr float AUTO_DESTROY_THRESHOLD_RATIO = 0.75f;
    constexpr float AUTO_DESTROY_DELAY = 3.0f;

    // 切断設定
    constexpr float SLICE_OFFSET_DISTANCE = 0.3f;     // 分離オフセット（旧0.02f → 拡大で団子防止）
    constexpr float SLICE_SEPARATION_SPEED = 6.0f;     // 分離速度（旧2.0f → 拡大で団子防止）
    constexpr float SLICE_TORQUE_STRENGTH = 5.0f;
    constexpr float SLICE_IMMUNITY_TIME = 0.8f;        // 衝突無効時間（旧0.5f → 延長で再衝突防止）
    constexpr float SLICE_PLANE_THRESHOLD = 0.001f;
    constexpr float MIN_OBJECT_MASS = 0.1f;
    constexpr float MIN_VOLUME = 0.0001f;

    // デバッグ描画色
    const XMFLOAT4 DEBUG_COLOR_PROPS = { 0.0f, 1.0f, 0.0f, 1.0f };
}

// 内部データ
namespace
{
    std::vector<PhysicsModel*> g_Props;             // アクティブなオブジェクト
    int g_NextGenerationId = 1;                     // 衝突判定グループID

    // 非同期処理用：スライス計算中の削除待ちオブジェクト
    std::unordered_map<int, PhysicsModel*> g_PendingDeleteObjects;
}

// 内部ヘルパー関数
namespace
{
    /**
     * @brief コライダーの形状から近似体積を算出
     */
    float CalculateColliderVolume(const Collider& collider)
    {
        switch (collider.type)
        {
            case ColliderType::Box:
            {
                const auto& extents = collider.obb.extents;
                return (extents.x * 2.0f) * (extents.y * 2.0f) * (extents.z * 2.0f);
            }
            case ColliderType::Sphere:
            {
                constexpr float SPHERE_VOLUME_CONSTANT = 4.18879f; // 4/3 * PI
                return SPHERE_VOLUME_CONSTANT * std::pow(collider.sphere.radius, 3.0f);
            }
            default:
                return 1.0f;
        }
    }

    /**
     * @brief PhysicsModelの初期体積を保存（破片判定での基準）
     */
    void InitializeRootVolume(PhysicsModel* model)
    {
        if (!model) return;

        const Collider& collider = model->GetRigidBody()->GetWorldCollider();
        float volume = CalculateColliderVolume(collider);
        model->SetRootVolume(volume);
    }

    //--------------------------------------
    // 空間ハッシュグリッドによる衝突判定高速化
    //--------------------------------------
    constexpr float GRID_CELL_SIZE = 3.0f;
    constexpr float INV_GRID_CELL_SIZE = 1.0f / GRID_CELL_SIZE;

    struct CellKey {
        int x, y, z;
        bool operator==(const CellKey& o) const { return x == o.x && y == o.y && z == o.z; }
    };
    // 空間ハッシュ用の素数定数
    constexpr int HASH_PRIME_X = 73856093;   // X軸ハッシュ用素数
    constexpr int HASH_PRIME_Y = 19349663;   // Y軸ハッシュ用素数
    constexpr int HASH_PRIME_Z = 83492791;   // Z軸ハッシュ用素数

    // 切断片の反発係数
    constexpr float SLICED_PIECE_RESTITUTION = 0.6f; // 切断片は高めの反発で分離しやすく

    struct CellKeyHash {
        size_t operator()(const CellKey& k) const {
            // 大きな素数によるハッシュ
            return static_cast<size_t>(k.x * HASH_PRIME_X) ^
                   static_cast<size_t>(k.y * HASH_PRIME_Y) ^
                   static_cast<size_t>(k.z * HASH_PRIME_Z);
        }
    };

    static std::unordered_map<CellKey, std::vector<size_t>, CellKeyHash> g_SpatialGrid;

    CellKey PositionToCell(const XMFLOAT3& pos)
    {
        return {
            static_cast<int>(floorf(pos.x * INV_GRID_CELL_SIZE)),
            static_cast<int>(floorf(pos.y * INV_GRID_CELL_SIZE)),
            static_cast<int>(floorf(pos.z * INV_GRID_CELL_SIZE))
        };
    }

    /**
     * @brief オブジェクト間の衝突を空間ハッシュで高速化（O(n) 広域 + 近傍のみ狭域）
     */
    void ResolveCollisions()
    {
        const size_t n = g_Props.size();
        if (n < 2) return;

        for (int iter = 0; iter < PropConfig::COLLISION_ITERATIONS; ++iter)
        {
            // グリッド構築
            g_SpatialGrid.clear();
            for (size_t i = 0; i < n; ++i)
            {
                const XMFLOAT3& pos = g_Props[i]->GetRigidBody()->GetPosition();
                CellKey key = PositionToCell(pos);
                g_SpatialGrid[key].push_back(i);
            }

            // 各セルの中と隣接セルとのペアのみ判定
            for (auto& [cellKey, indices] : g_SpatialGrid)
            {
                // 同一セル内ペア
                for (size_t a = 0; a < indices.size(); ++a)
                {
                    for (size_t b = a + 1; b < indices.size(); ++b)
                    {
                        RigidBody::SolveCollision(
                            g_Props[indices[a]]->GetRigidBody(),
                            g_Props[indices[b]]->GetRigidBody()
                        );
                    }
                }

                // 隣接セル（半分のみ走査して重複回避: +x, +y, +z, +xy, +xz, +yz, +xyz, etc.）
                static const CellKey offsets[] = {
                    {1,0,0},{0,1,0},{0,0,1},{1,1,0},{1,0,1},{0,1,1},{1,1,1},
                    {1,-1,0},{1,0,-1},{0,1,-1},{1,1,-1},{1,-1,1},{1,-1,-1}
                };
                for (const auto& off : offsets)
                {
                    CellKey neighbor = { cellKey.x + off.x, cellKey.y + off.y, cellKey.z + off.z };
                    auto it = g_SpatialGrid.find(neighbor);
                    if (it == g_SpatialGrid.end()) continue;

                    for (size_t idx : indices)
                    {
                        for (size_t nIdx : it->second)
                        {
                            RigidBody::SolveCollision(
                                g_Props[idx]->GetRigidBody(),
                                g_Props[nIdx]->GetRigidBody()
                            );
                        }
                    }
                }
            }
        }
    }

    /**
     * @brief 2つのレイから切断平面の法線を算出
     */
    bool CalculateSlicePlaneNormal(const Ray& startRay, const Ray& endRay, XMFLOAT3& outNormal)
    {
        using namespace PropConfig;

        XMFLOAT3 startDir = startRay.GetDirection();
        XMFLOAT3 endDir = endRay.GetDirection();

        XMVECTOR vStartDir = XMLoadFloat3(&startDir);
        XMVECTOR vEndDir = XMLoadFloat3(&endDir);
        XMVECTOR vPlaneNormal = XMVector3Normalize(XMVector3Cross(vStartDir, vEndDir));

        if (XMVectorGetX(XMVector3LengthSq(vPlaneNormal)) <= SLICE_PLANE_THRESHOLD)
        {
            return false;
        }

        XMStoreFloat3(&outNormal, vPlaneNormal);
        return true;
    }

    struct SeparationParams
    {
        XMFLOAT3 position;
        XMFLOAT3 velocity;
    };

    /**
     * @brief 切断後の反発による分離速度と位置のオフセットを計算
     */
    void CalculateSeparationParams(
        const XMFLOAT3& originalPos,
        const XMFLOAT3& originalVel,
        const XMFLOAT3& planeNormal,
        bool isFrontSide,
        SeparationParams& outParams)
    {
        using namespace PropConfig;

        XMVECTOR vPos = XMLoadFloat3(&originalPos);
        XMVECTOR vVel = XMLoadFloat3(&originalVel);
        XMVECTOR vNormal = XMLoadFloat3(&planeNormal);

        float sign = isFrontSide ? 1.0f : -1.0f;
        XMVECTOR vOffset = vNormal * (SLICE_OFFSET_DISTANCE * sign);
        XMVECTOR vSepVel = vNormal * (SLICE_SEPARATION_SPEED * sign);

        XMStoreFloat3(&outParams.position, vPos + vOffset);
        XMStoreFloat3(&outParams.velocity, vVel + vSepVel);
    }

    /**
     * @brief 生成されたメッシュデータからGPUリソースを構築しPhysicsModelを実体化
     * @detail メインスレッドでの実行が必須
     */
    PhysicsModel* CreateSlicedObjectFromMeshes(
        const std::vector<MeshData>& meshes,
        MODEL* originalModel,
        const SeparationParams& params,
        const ColliderType& colliderType,
        const XMFLOAT3& planeNormal,
        float originalMass,
        float oldVolume,
        float rootVolume,
        int generationId,
        bool isFrontSide,
        float lifeTime = -1.0f)
    {
        using namespace PropConfig;

        // メッシュデータからDirectXのバッファを持つMODELを生成
        MODEL* newModel = ModelCreateFromData(meshes, originalModel);
        if (!newModel) return nullptr;

        auto* newObject = new PhysicsModel(newModel, params.position, params.velocity, originalMass, colliderType);
        RigidBody* rb = newObject->GetRigidBody();

        // 基底体積の継承
        newObject->SetRootVolume(rootVolume);

        // 新しいコライダー形状に基づく質量のスケーリング
        float newVolume = CalculateColliderVolume(rb->GetWorldCollider());
        float newMass = std::max(originalMass * (newVolume / std::max(oldVolume, MIN_VOLUME)), MIN_OBJECT_MASS);

        RigidBody::Params params_rb = rb->GetParams();
        params_rb.mass = newMass;
        params_rb.restitution = SLICED_PIECE_RESTITUTION;  // 切断片は高めの反発で分離しやすく（デフォルト0.2f）
        rb->SetParams(params_rb);

        // 寿命設定
        if (lifeTime > 0.0f)
        {
            // 外部から指定された寿命を使用
            newObject->SetAutoDestroyTimer(lifeTime);
        }
        else
        {
            // 体積が一定以下（破片）になった場合は自動消滅タイマーを起動
            float threshold = rootVolume * AUTO_DESTROY_THRESHOLD_RATIO;
            if (newVolume <= threshold)
            {
                newObject->SetAutoDestroyTimer(AUTO_DESTROY_DELAY);
            }
        }

        // 親子・破片間の衝突無視設定
        rb->SetGenerationId(generationId);
        rb->SetIgnoreCollisionTimer(SLICE_IMMUNITY_TIME);

        // 切断方向への回転トルクを付与
        XMVECTOR vNormal = XMLoadFloat3(&planeNormal);
        float torqueSign = isFrontSide ? 1.0f : -1.0f;
        XMFLOAT3 torque;
        XMStoreFloat3(&torque, vNormal * (SLICE_TORQUE_STRENGTH * torqueSign));
        rb->AddTorque(torque);

        // 断面グロー開始
        newObject->SetGlowTimer(PhysicsModel::GLOW_DURATION);

        // PhysicsModel内部で管理されるため、ここで一度Releaseしておく（参照カウンタ調整）
        ModelRelease(newModel);
        return newObject;
    }

    /**
     * @brief タスクマネージャに切断リクエストをキューイング
     */
    int SubmitSliceRequest(PhysicsModel* obj, const XMFLOAT3& hitPos, const XMFLOAT3& planeNormal)
    {
        RigidBody* rb = obj->GetRigidBody();

        SliceRequest request;
        request.targetModel = obj->GetModel();
        request.worldMatrix = rb->GetWorldMatrix(XMFLOAT3(1.0f, 1.0f, 1.0f));
        request.planePoint = hitPos;
        request.planeNormal = planeNormal;
        request.originalPosition = rb->GetPosition();
        request.originalVelocity = rb->GetVelocity();
        request.originalMass = rb->GetParams().mass;
        request.rootVolume = obj->GetRootVolume();
        request.colliderType = obj->GetColliderType();

        return SliceTaskManager::EnqueueSlice(request);
    }

    /**
     * @brief 非同期スライス完了後のコールバック処理
     */
    void ProcessCompletedSlice(const SliceResult& result)
    {
        using namespace PropConfig;

        if (!result.success)
        {
            ModelRelease(result.originalModel);
            return;
        }

        // 元の体積を計算（質量分布の基準）
        float oldVolume = std::max(MIN_VOLUME, MIN_VOLUME);
        int generationId = g_NextGenerationId++;

        // 切断平面法線（リクエスト時のものを使用）
        XMFLOAT3 planeNormal = result.planeNormal;

        // Front側の生成
        SeparationParams frontParams;
        CalculateSeparationParams(result.originalPosition, result.originalVelocity, planeNormal, true, frontParams);
        PhysicsModel* frontObj = CreateSlicedObjectFromMeshes(
            result.frontMeshes,
            result.originalModel,
            frontParams,
            result.colliderType,
            planeNormal,
            result.originalMass,
            oldVolume,
            result.rootVolume,
            generationId,
            true
        );

        // Back側の生成
        SeparationParams backParams;
        CalculateSeparationParams(result.originalPosition, result.originalVelocity, planeNormal, false, backParams);
        PhysicsModel* backObj = CreateSlicedObjectFromMeshes(
            result.backMeshes,
            result.originalModel,
            backParams,
            result.colliderType,
            planeNormal,
            result.originalMass,
            oldVolume,
            result.rootVolume,
            generationId,
            false
        );

        if (frontObj) g_Props.push_back(frontObj);
        if (backObj) g_Props.push_back(backObj);

        // コンボ加算（何かしらを切断したらコンボ発動）
        Combo_Add(1);

        ModelRelease(result.originalModel);

        // 削除待ちオブジェクトのクリーンアップ
        auto it = g_PendingDeleteObjects.find(result.requestId);
        if (it != g_PendingDeleteObjects.end())
        {
            delete it->second;
            g_PendingDeleteObjects.erase(it);
        }
    }
}

// 公開関数

void PropManager_Initialize()
{
    using namespace PropConfig;

    // 非同期タスクマネージャの起動（PCのコア数から自動決定）
    SliceTaskManager::Initialize();
}

void PropManager_Finalize()
{
    SliceTaskManager::Finalize();

    for (auto obj : g_Props)
    {
        delete obj;
    }
    g_Props.clear();

    for (auto& pair : g_PendingDeleteObjects)
    {
        delete pair.second;
    }
    g_PendingDeleteObjects.clear();
}

void PropManager_Update(double elapsed_time)
{
    // 非同期スライスの完了通知を確認
    SliceResult result;
    while (SliceTaskManager::TryGetCompletedResult(result))
    {
        ProcessCompletedSlice(result);
    }

    // オブジェクトの更新と寿命管理（swap-and-popで中間erase回避）
    for (size_t i = 0; i < g_Props.size();)
    {
        PhysicsModel* obj = g_Props[i];
        obj->Update(elapsed_time);

        if (obj->IsDead())
        {
            delete obj;
            g_Props[i] = g_Props.back();
            g_Props.pop_back();
        }
        else
        {
            ++i;
        }
    }

    // 物理衝突解決
    ResolveCollisions();
}

void PropManager_Draw()
{
    // フラスタムカリング: ビュー外のオブジェクトをスキップ
    const XMFLOAT4X4& view = PLCamera_GetViewMatrix();
    const XMFLOAT4X4& proj = PLCamera_GetPerspectiveMatrix();
    XMMATRIX mVP = XMLoadFloat4x4(&view) * XMLoadFloat4x4(&proj);
    XMFLOAT4X4 vpMatrix;
    XMStoreFloat4x4(&vpMatrix, mVP);

    Frustum frustum;
    Frustum_Extract(frustum, vpMatrix);

    for (auto obj : g_Props)
    {
        // AABB取得してフラスタムテスト
        XMFLOAT3 pos = obj->GetPosition();
        AABB aabb = Model_GetAABB(obj->GetModel(), pos);
        if (!Frustum_TestAABB(frustum, aabb))
            continue; // ビュー外：スキップ

        obj->Draw();
    }
}

void PropManager_DrawShadow()
{
    for (auto obj : g_Props)
    {
        XMFLOAT4X4 matWorld = obj->GetRigidBody()->GetWorldMatrix(XMFLOAT3(1.0f, 1.0f, 1.0f));
        ModelDrawShadow(obj->GetModel(), XMLoadFloat4x4(&matWorld));
    }
}

void PropManager_DrawDebug()
{
    using namespace PropConfig;

    for (auto obj : g_Props)
    {
        RigidBody* rb = obj->GetRigidBody();
        Collider col = rb->GetWorldCollider();

        switch (col.type)
        {
            case ColliderType::Box:
                DebugRenderer::DrawOBB(col.obb, DEBUG_COLOR_PROPS);
                break;
            case ColliderType::Sphere:
                DebugRenderer::DrawSphere(col.sphere, DEBUG_COLOR_PROPS);
                break;
        }
    }

}

void PropManager_TrySlice(const Ray& startRay, const Ray& endRay)
{
    XMFLOAT3 planeNormal;
    if (!CalculateSlicePlaneNormal(startRay, endRay, planeNormal))
    {
        return;
    }

    for (auto it = g_Props.begin(); it != g_Props.end();)
    {
        PhysicsModel* obj = *it;

        if (!obj->GetModel())
        {
            ++it;
            continue;
        }

        // 境界ボリュームによる早期棄却
        RigidBody* rb = obj->GetRigidBody();
        XMFLOAT3 pos = rb->GetPosition();
        AABB worldAABB = Model_GetAABB(obj->GetModel(), pos);

        float hitDist = 0.0f;
        if (!Collision_IntersectRayAABB(endRay, worldAABB, &hitDist))
        {
            ++it;
            continue;
        }

        // ヒット位置を特定
        XMFLOAT3 rayOrigin = endRay.GetOrigin();
        XMFLOAT3 rayDir = endRay.GetDirection();
        XMVECTOR vHitPos = XMLoadFloat3(&rayOrigin) + XMLoadFloat3(&rayDir) * hitDist;
        XMFLOAT3 hitPos;
        XMStoreFloat3(&hitPos, vHitPos);

        // スライス計算を別スレッドへ委譲
        int requestId = SubmitSliceRequest(obj, hitPos, planeNormal);

        // メインの更新・描画リストから外し、計算完了まで待機リストで保持
        g_PendingDeleteObjects[requestId] = obj;
        it = g_Props.erase(it);
    }
}

int PropManager_AllocateGenerationId()
{
    return g_NextGenerationId++;
}

void PropManager_AddSlicedPiece(const SlicedPieceParams& params)
{
    using namespace PropConfig;

    if (params.meshes.empty() || !params.originalModel)
    {
        return;
    }

    // 呼び出し元で既に分離速度・位置が計算済みなのでそのまま使用
    SeparationParams sepParams;
    sepParams.position = params.position;
    sepParams.velocity = params.velocity;

    // 呼び出し元から渡されたgenerationIdを使用（表裏ペアで同じIDにすること）
    int generationId = params.generationId;

    PhysicsModel* newObj = CreateSlicedObjectFromMeshes(
        params.meshes,
        params.originalModel,
        sepParams,
        params.colliderType,
        params.planeNormal,
        params.mass,
        std::max(params.rootVolume, MIN_VOLUME),
        params.rootVolume,
        generationId,
        params.isFrontSide,
        params.lifeTime
    );

    if (newObj)
    {
        g_Props.push_back(newObj);
    }
}