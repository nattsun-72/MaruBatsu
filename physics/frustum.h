/****************************************
 * @file frustum.h
 * @brief 視錐台カリング判定
 * @author Natsume Shidara
 * @date 2025/06/06
 *
 * ViewProjection行列から6平面を抽出し、AABB/球との判定を行う。
 ****************************************/
#ifndef FRUSTUM_H
#define FRUSTUM_H

#include <DirectXMath.h>

struct AABB;

/**
 * @struct Frustum
 * @brief 6 planes (left, right, bottom, top, near, far).
 *        Each plane: (nx, ny, nz, d) where nx*x + ny*y + nz*z + d >= 0 is inside.
 */
struct Frustum
{
    DirectX::XMFLOAT4 planes[6];
};

/**
 * @brief Extract frustum from ViewProjection matrix (row-major, LH).
 */
void Frustum_Extract(Frustum& out, const DirectX::XMFLOAT4X4& viewProj);

/**
 * @brief AABB vs frustum test.
 * @return true if AABB intersects or is inside the frustum.
 */
bool Frustum_TestAABB(const Frustum& frustum, const AABB& aabb);

/**
 * @brief Sphere vs frustum test.
 * @return true if sphere intersects or is inside the frustum.
 */
bool Frustum_TestSphere(const Frustum& frustum, const DirectX::XMFLOAT3& center, float radius);

#endif // FRUSTUM_H
