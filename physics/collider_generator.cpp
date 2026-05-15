/****************************************
 * @file collider_generator.cpp
 * @brief コライダー自動生成の実装
 * @detail DirectXCollisionを利用した最適コライダー算出
 * @author Natsume Shidara
 * @update 2025/12/15
 * @update 2026/02/23 - ColliderType別の生成対応（union安全化）
 ****************************************/

#include "collider_generator.h"
#include <vector>
#include <algorithm>
#include <cfloat>
#include <DirectXCollision.h> // BoundingOrientedBox用

using namespace DirectX;

namespace {
    constexpr int OBB_CORNER_COUNT = 8;                  // OBBの頂点数
    constexpr float DEFAULT_COLLIDER_RADIUS = 0.5f;      // デフォルトコライダー半径
    constexpr float DEFAULT_CAPSULE_RADIUS = 0.25f;      // デフォルトカプセル半径
}

// コライダー自動生成
Collider ColliderGenerator::GenerateBestFit(const MODEL* pModel, ColliderType colliderType)
{
    Collider col;
    col.type = colliderType;

    // モデルが無効ならデフォルト値を返却（typeに応じた正しいメンバーを初期化）
    if (!pModel || pModel->Meshes.empty())
    {
        switch (colliderType)
        {
        case ColliderType::Sphere:
            col.sphere.center = XMFLOAT3(0, 0, 0);
            col.sphere.radius = 1.0f;
            break;
        case ColliderType::AABB:
            col.aabb.min = XMFLOAT3(-1, -1, -1);
            col.aabb.max = XMFLOAT3(1, 1, 1);
            break;
        case ColliderType::Capsule:
            col.capsule.start = XMFLOAT3(0, -1, 0);
            col.capsule.end = XMFLOAT3(0, 1, 0);
            col.capsule.radius = 0.5f;
            break;
        case ColliderType::Box:
        default:
            col.type = ColliderType::Box;
            col.obb.center = XMFLOAT3(0, 0, 0);
            col.obb.extents = XMFLOAT3(1, 1, 1);
            XMStoreFloat4(&col.obb.orientation, XMQuaternionIdentity());
            break;
        }
        return col;
    }

    // ---------------------------------------------------
    // 1. 全頂点の抽出
    // ---------------------------------------------------
    std::vector<XMFLOAT3> points;

    // 全メッシュの頂点を1つのリストに集約
    for (const auto& mesh : pModel->Meshes)
    {
        points.reserve(points.size() + mesh.vertices.size());
        for (const auto& v : mesh.vertices)
        {
            points.push_back(v.position);
        }
    }

    if (points.empty())
    {
        switch (colliderType)
        {
        case ColliderType::Sphere:
            col.sphere.center = XMFLOAT3(0, 0, 0);
            col.sphere.radius = DEFAULT_COLLIDER_RADIUS;
            break;
        case ColliderType::AABB:
            col.aabb.min = XMFLOAT3(-DEFAULT_COLLIDER_RADIUS, -DEFAULT_COLLIDER_RADIUS, -DEFAULT_COLLIDER_RADIUS);
            col.aabb.max = XMFLOAT3(DEFAULT_COLLIDER_RADIUS, DEFAULT_COLLIDER_RADIUS, DEFAULT_COLLIDER_RADIUS);
            break;
        case ColliderType::Capsule:
            col.capsule.start = XMFLOAT3(0, -DEFAULT_COLLIDER_RADIUS, 0);
            col.capsule.end = XMFLOAT3(0, DEFAULT_COLLIDER_RADIUS, 0);
            col.capsule.radius = DEFAULT_CAPSULE_RADIUS;
            break;
        case ColliderType::Box:
        default:
            col.type = ColliderType::Box;
            col.obb.center = XMFLOAT3(0, 0, 0);
            col.obb.extents = XMFLOAT3(DEFAULT_COLLIDER_RADIUS, DEFAULT_COLLIDER_RADIUS, DEFAULT_COLLIDER_RADIUS);
            XMStoreFloat4(&col.obb.orientation, XMQuaternionIdentity());
            break;
        }
        return col;
    }

    // ---------------------------------------------------
    // 2. 最適なOBBの計算 (DirectXCollision利用)
    //    全タイプで基礎データとして使用
    // ---------------------------------------------------
    BoundingOrientedBox dxObb;

    // CreateFromPointsは内部で共分散行列を用いて
    // 点群にフィットする姿勢とサイズを自動計算してくれる
    BoundingOrientedBox::CreateFromPoints(dxObb, points.size(), points.data(), sizeof(XMFLOAT3));

    // ---------------------------------------------------
    // 3. ColliderTypeに応じた変換・格納
    // ---------------------------------------------------
    switch (colliderType)
    {
    case ColliderType::Box:
        col.obb.center = dxObb.Center;
        col.obb.extents = dxObb.Extents;
        col.obb.orientation = dxObb.Orientation;
        break;

    case ColliderType::Sphere:
    {
        // OBBの最大extentを半径とする包含球
        float maxExtent = (std::max)({ dxObb.Extents.x, dxObb.Extents.y, dxObb.Extents.z });
        col.sphere.center = dxObb.Center;
        col.sphere.radius = maxExtent;
        break;
    }

    case ColliderType::AABB:
    {
        // OBBから回転を考慮してAABBに変換（8頂点の包含AABB）
        XMMATRIX rot = XMMatrixRotationQuaternion(XMLoadFloat4(&dxObb.Orientation));
        XMVECTOR vCenter = XMLoadFloat3(&dxObb.Center);
        XMVECTOR vMin = XMVectorSet(FLT_MAX, FLT_MAX, FLT_MAX, 0.0f);
        XMVECTOR vMax = XMVectorSet(-FLT_MAX, -FLT_MAX, -FLT_MAX, 0.0f);

        for (int i = 0; i < OBB_CORNER_COUNT; ++i)
        {
            XMVECTOR corner = XMVectorSet(
                (i & 1) ? dxObb.Extents.x : -dxObb.Extents.x,
                (i & 2) ? dxObb.Extents.y : -dxObb.Extents.y,
                (i & 4) ? dxObb.Extents.z : -dxObb.Extents.z,
                0.0f);
            XMVECTOR worldCorner = XMVectorAdd(XMVector3TransformNormal(corner, rot), vCenter);
            vMin = XMVectorMin(vMin, worldCorner);
            vMax = XMVectorMax(vMax, worldCorner);
        }
        XMStoreFloat3(&col.aabb.min, vMin);
        XMStoreFloat3(&col.aabb.max, vMax);
        break;
    }

    case ColliderType::Capsule:
    {
        // Y軸を長軸とするカプセルを生成
        col.capsule.start = { dxObb.Center.x, dxObb.Center.y - dxObb.Extents.y, dxObb.Center.z };
        col.capsule.end   = { dxObb.Center.x, dxObb.Center.y + dxObb.Extents.y, dxObb.Center.z };
        col.capsule.radius = (std::max)(dxObb.Extents.x, dxObb.Extents.z);
        break;
    }

    default:
        // フォールバック: OBBとして生成
        col.type = ColliderType::Box;
        col.obb.center = dxObb.Center;
        col.obb.extents = dxObb.Extents;
        col.obb.orientation = dxObb.Orientation;
        break;
    }

    return col;
}