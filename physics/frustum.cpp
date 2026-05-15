/****************************************
 * @file frustum.cpp
 * @brief 視錐台カリング判定の実装
 * @author Natsume Shidara
 * @date 2025/06/06
 ****************************************/
#include "frustum.h"
#include "game/collision.h"
#include <cmath>

using namespace DirectX;

namespace {
    constexpr int FRUSTUM_PLANE_COUNT = 6;              // 視錐台の平面数
    constexpr float PLANE_NORMALIZE_EPSILON = 0.0001f;  // 平面正規化の有効判定閾値
}

/**
 * @brief Extract 6 frustum planes from a row-major ViewProjection matrix.
 * Gribb-Hartmann method. Planes are normalized for correct distance tests.
 */
void Frustum_Extract(Frustum& out, const XMFLOAT4X4& vp)
{
    // Left   = row3 + row0
    out.planes[0] = { vp._14 + vp._11, vp._24 + vp._21, vp._34 + vp._31, vp._44 + vp._41 };
    // Right  = row3 - row0
    out.planes[1] = { vp._14 - vp._11, vp._24 - vp._21, vp._34 - vp._31, vp._44 - vp._41 };
    // Bottom = row3 + row1
    out.planes[2] = { vp._14 + vp._12, vp._24 + vp._22, vp._34 + vp._32, vp._44 + vp._42 };
    // Top    = row3 - row1
    out.planes[3] = { vp._14 - vp._12, vp._24 - vp._22, vp._34 - vp._32, vp._44 - vp._42 };
    // Near   = row2
    out.planes[4] = { vp._13, vp._23, vp._33, vp._43 };
    // Far    = row3 - row2
    out.planes[5] = { vp._14 - vp._13, vp._24 - vp._23, vp._34 - vp._33, vp._44 - vp._43 };

    // Normalize each plane
    for (int i = 0; i < FRUSTUM_PLANE_COUNT; i++)
    {
        float len = std::sqrtf(
            out.planes[i].x * out.planes[i].x +
            out.planes[i].y * out.planes[i].y +
            out.planes[i].z * out.planes[i].z);
        if (len > PLANE_NORMALIZE_EPSILON)
        {
            float invLen = 1.0f / len;
            out.planes[i].x *= invLen;
            out.planes[i].y *= invLen;
            out.planes[i].z *= invLen;
            out.planes[i].w *= invLen;
        }
    }
}

/**
 * @brief Test AABB against frustum. Uses the "p-vertex" method for each plane.
 *        Returns false only if the AABB is completely outside any one plane.
 */
bool Frustum_TestAABB(const Frustum& frustum, const AABB& aabb)
{
    for (int i = 0; i < FRUSTUM_PLANE_COUNT; i++)
    {
        const XMFLOAT4& p = frustum.planes[i];

        // Find the point of the AABB most in the direction of the plane normal (p-vertex)
        float px = (p.x >= 0.0f) ? aabb.max.x : aabb.min.x;
        float py = (p.y >= 0.0f) ? aabb.max.y : aabb.min.y;
        float pz = (p.z >= 0.0f) ? aabb.max.z : aabb.min.z;

        // If the p-vertex is outside, the entire AABB is outside this plane
        if (p.x * px + p.y * py + p.z * pz + p.w < 0.0f)
            return false;
    }
    return true;
}

/**
 * @brief Test sphere against frustum.
 */
bool Frustum_TestSphere(const Frustum& frustum, const XMFLOAT3& center, float radius)
{
    for (int i = 0; i < FRUSTUM_PLANE_COUNT; i++)
    {
        const XMFLOAT4& p = frustum.planes[i];
        float dist = p.x * center.x + p.y * center.y + p.z * center.z + p.w;
        if (dist < -radius)
            return false;
    }
    return true;
}
