/****************************************
 * @file slicer.cpp
 * @brief メッシュ切断実装（Ear Clipping対応・AABBカリング最適化版）
 * @author Natsume Shidara
 * @date 2025/12/08
 * @update 2025/12/17 (Optimization applied)
 * @update 2026/02/22 (Refactored: DRY, slice blend support)
 ****************************************/

#include "slicer.h"
#include <algorithm>
#include <cmath>
#include <list>
#include <map>
#include <vector>
#include <float.h>

using namespace DirectX;

// 内部用構造体・定数

static constexpr float EPSILON = 1e-4f;
static constexpr float VERTICAL_NORMAL_THRESHOLD = 0.9f;  // 法線がほぼ垂直とみなす閾値
static constexpr int MIN_POLYGON_VERTICES = 3;             // ポリゴンの最小頂点数

struct RawEdge
{
    XMFLOAT3 start;
    XMFLOAT3 end;
};

struct Vector2
{
    float x, y;
};

enum class PlaneSide
{
    Front,
    Back,
    Intersect
};

/// ローカル空間での切断パラメータ（座標変換済み）
struct LocalSliceParams
{
    XMVECTOR localNormal;
    XMVECTOR planeEq;
    int      capMaterialIndex;
    int      sliceTextureId;
    float    blendRatio;
};

// 2D幾何ヘルパー

/** @brief 2Dベクトルの外積（符号付き面積） */
static float Cross2D(const Vector2& a, const Vector2& b)
{
    return a.x * b.y - a.y * b.x;
}

/** @brief 点が三角形内部にあるか判定 */
static bool IsPointInTriangle(const Vector2& p, const Vector2& a, const Vector2& b, const Vector2& c)
{
    float cp1 = Cross2D({ b.x - a.x, b.y - a.y }, { p.x - a.x, p.y - a.y });
    float cp2 = Cross2D({ c.x - b.x, c.y - b.y }, { p.x - b.x, p.y - b.y });
    float cp3 = Cross2D({ a.x - c.x, a.y - c.y }, { p.x - c.x, p.y - c.y });
    return (cp1 >= -EPSILON && cp2 >= -EPSILON && cp3 >= -EPSILON)
        || (cp1 <=  EPSILON && cp2 <=  EPSILON && cp3 <=  EPSILON);
}

// 平面法線から正規直交基底を生成
// 法線がほぼ垂直 (|y|>0.9) の場合は代替up軸を使用してゼロベクトルを回避

static void ComputeTangentBasis(const XMVECTOR& normal, XMVECTOR& outTangent, XMVECTOR& outBinormal)
{
    XMVECTOR vUp = (std::abs(XMVectorGetY(normal)) > VERTICAL_NORMAL_THRESHOLD)
        ? XMVectorSet(0, 0, 1, 0)
        : XMVectorSet(0, 1, 0, 0);
    outTangent  = XMVector3Normalize(XMVector3Cross(normal, vUp));
    outBinormal = XMVector3Normalize(XMVector3Cross(normal, outTangent));
}

// UV計算

/// 従来の平面投影UV（タイリング用、テクスチャ未指定時のフォールバック）
static XMFLOAT2 CalculatePlanarUV(const XMFLOAT3& pos, const XMVECTOR& tangent, const XMVECTOR& binormal)
{
    XMVECTOR vPos = XMLoadFloat3(&pos);
    float u = XMVectorGetX(XMVector3Dot(vPos, tangent));
    float v = XMVectorGetX(XMVector3Dot(vPos, binormal));
    return XMFLOAT2(u * 0.5f + 0.5f, v * 0.5f + 0.5f);
}

// 頂点ウェルディング（座標が近い頂点を統合してインデックスを返す）

static int GetWeldedVertexIndex(std::vector<XMFLOAT3>& vertices, const XMFLOAT3& pos)
{
    for (int i = 0; i < (int)vertices.size(); ++i)
    {
        const XMFLOAT3& v = vertices[i];
        if (std::abs(v.x - pos.x) < EPSILON &&
            std::abs(v.y - pos.y) < EPSILON &&
            std::abs(v.z - pos.z) < EPSILON)
        {
            return i;
        }
    }
    vertices.push_back(pos);
    return (int)vertices.size() - 1;
}

// AABB判定

/** @brief AABBと平面の位置関係を判定（表側/裏側/交差） */
static PlaneSide CheckAABBPlane(const AABB& aabb, const XMVECTOR& planeEq)
{
    XMFLOAT3 corners[8] = {
        { aabb.min.x, aabb.min.y, aabb.min.z },
        { aabb.min.x, aabb.min.y, aabb.max.z },
        { aabb.min.x, aabb.max.y, aabb.min.z },
        { aabb.min.x, aabb.max.y, aabb.max.z },
        { aabb.max.x, aabb.min.y, aabb.min.z },
        { aabb.max.x, aabb.min.y, aabb.max.z },
        { aabb.max.x, aabb.max.y, aabb.min.z },
        { aabb.max.x, aabb.max.y, aabb.max.z }
    };

    int frontCount = 0;
    int backCount  = 0;

    for (int i = 0; i < 8; ++i)
    {
        float dist = XMVectorGetX(XMPlaneDotCoord(planeEq, XMLoadFloat3(&corners[i])));
        if (dist > EPSILON)       frontCount++;
        else if (dist < -EPSILON) backCount++;
    }

    if (backCount  == 0) return PlaneSide::Front;
    if (frontCount == 0) return PlaneSide::Back;
    return PlaneSide::Intersect;
}

/** @brief メッシュの頂点群からAABBを算出 */
static AABB CalculateMeshAABB(const MeshData& mesh)
{
    AABB aabb;
    aabb.min = {  FLT_MAX,  FLT_MAX,  FLT_MAX };
    aabb.max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

    for (const auto& v : mesh.vertices)
    {
        aabb.min.x = std::min(aabb.min.x, v.position.x);
        aabb.min.y = std::min(aabb.min.y, v.position.y);
        aabb.min.z = std::min(aabb.min.z, v.position.z);
        aabb.max.x = std::max(aabb.max.x, v.position.x);
        aabb.max.y = std::max(aabb.max.y, v.position.y);
        aabb.max.z = std::max(aabb.max.z, v.position.z);
    }
    return aabb;
}

// ループ抽出（エッジスープ → 閉ループ群）

static void ExtractLoops(const std::vector<RawEdge>& rawEdges, std::vector<std::vector<XMFLOAT3>>& outLoops)
{
    if (rawEdges.empty())
        return;

    std::vector<XMFLOAT3> uniqueVertices;
    std::map<int, std::vector<int>> adjacency;

    for (const auto& e : rawEdges)
    {
        int s = GetWeldedVertexIndex(uniqueVertices, e.start);
        int eIdx = GetWeldedVertexIndex(uniqueVertices, e.end);
        if (s != eIdx)
        {
            adjacency[s].push_back(eIdx);
        }
    }

    while (!adjacency.empty())
    {
        std::vector<XMFLOAT3> currentLoop;
        int startNode   = adjacency.begin()->first;
        int currentNode  = startNode;
        bool loopClosed  = false;

        while (true)
        {
            currentLoop.push_back(uniqueVertices[currentNode]);

            auto it = adjacency.find(currentNode);
            if (it == adjacency.end() || it->second.empty())
                break;

            int nextNode = it->second.back();
            it->second.pop_back();
            if (it->second.empty())
                adjacency.erase(it);

            currentNode = nextNode;
            if (currentNode == startNode)
            {
                loopClosed = true;
                break;
            }
        }

        if (loopClosed && currentLoop.size() >= MIN_POLYGON_VERTICES)
        {
            outLoops.push_back(currentLoop);
        }
    }
}

// 耳刈り取り法（Ear Clipping）による三角形分割

static void TriangulateEarClipping(
    const std::vector<XMFLOAT3>& loop3D,
    const XMVECTOR& planeNormal,
    std::vector<unsigned int>& outIndices,
    int baseIndexOffset)
{
    int count = (int)loop3D.size();
    if (count < MIN_POLYGON_VERTICES)
        return;

    // 2D平面への投影基底
    XMVECTOR vTangent, vBinormal;
    ComputeTangentBasis(planeNormal, vTangent, vBinormal);

    std::vector<Vector2> poly(count);
    std::vector<int> indices(count);

    for (int i = 0; i < count; ++i)
    {
        XMVECTOR pos = XMLoadFloat3(&loop3D[i]);
        poly[i].x  = XMVectorGetX(XMVector3Dot(pos, vTangent));
        poly[i].y  = XMVectorGetX(XMVector3Dot(pos, vBinormal));
        indices[i] = i;
    }

    // 巻き順の確認（符号付き面積）
    float area = 0;
    for (int i = 0; i < count; ++i)
    {
        const Vector2& p1 = poly[i];
        const Vector2& p2 = poly[(i + 1) % count];
        area += (p1.x * p2.y - p2.x * p1.y);
    }
    if (area < 0)
        std::reverse(indices.begin(), indices.end());

    // 耳刈り取りループ
    int safetyCount = count * count;

    while (indices.size() > 2 && safetyCount > 0)
    {
        safetyCount--;
        bool earFound = false;
        int n = (int)indices.size();

        for (int i = 0; i < n; ++i)
        {
            int prevIdx = indices[(i + n - 1) % n];
            int currIdx = indices[i];
            int nextIdx = indices[(i + 1) % n];

            const Vector2& a = poly[prevIdx];
            const Vector2& b = poly[currIdx];
            const Vector2& c = poly[nextIdx];

            float cp = Cross2D({ b.x - a.x, b.y - a.y }, { c.x - b.x, c.y - b.y });
            if (cp <= EPSILON)
                continue;

            bool isEar = true;
            for (int j = 0; j < n; ++j)
            {
                int checkIdx = indices[j];
                if (checkIdx == prevIdx || checkIdx == currIdx || checkIdx == nextIdx)
                    continue;
                if (IsPointInTriangle(poly[checkIdx], a, b, c))
                {
                    isEar = false;
                    break;
                }
            }

            if (isEar)
            {
                outIndices.push_back(baseIndexOffset + prevIdx);
                outIndices.push_back(baseIndexOffset + currIdx);
                outIndices.push_back(baseIndexOffset + nextIdx);
                indices.erase(indices.begin() + i);
                earFound = true;
                break;
            }
        }

        if (!earFound)
        {
            // 自己交差等で耳が見つからない場合、強制的に進める
            outIndices.push_back(baseIndexOffset + indices[0]);
            outIndices.push_back(baseIndexOffset + indices[1]);
            outIndices.push_back(baseIndexOffset + indices[2]);
            indices.erase(indices.begin() + 1);
        }
    }
}

// 断面テクスチャブレンド比率の計算

static float CalculateSliceBlendRatio(const XMVECTOR& planeNormal)
{
    // |normal.y| が大きい = 水平切断 = 横切りテクスチャ (blend = 0.0)
    // |normal.y| が小さい = 垂直切断 = 縦切りテクスチャ (blend = 1.0)
    return 1.0f - std::abs(XMVectorGetY(planeNormal));
}

// 断面メッシュ生成（Ear Clipping版）

static void CreateCapMesh(
    int texId,
    MeshData& mesh,
    const std::vector<RawEdge>& rawEdges,
    const XMVECTOR& planeNormal)
{
    std::vector<std::vector<XMFLOAT3>> loops;
    ExtractLoops(rawEdges, loops);

    if (loops.empty())
        return;

    XMVECTOR vTangent, vBinormal;
    ComputeTangentBasis(planeNormal, vTangent, vBinormal);

    for (const auto& loop : loops)
    {
        int baseIndex = (int)mesh.vertices.size();

        // テクスチャ指定時：バウンディングボックスからUV正規化範囲を計算
        float minU = FLT_MAX, maxU = -FLT_MAX;
        float minV = FLT_MAX, maxV = -FLT_MAX;

        if (texId != -1)
        {
            for (const auto& pos : loop)
            {
                XMVECTOR vPos = XMLoadFloat3(&pos);
                float u = XMVectorGetX(XMVector3Dot(vPos, vTangent));
                float v = XMVectorGetX(XMVector3Dot(vPos, vBinormal));

                if (u < minU) minU = u;
                if (u > maxU) maxU = u;
                if (v < minV) minV = v;
                if (v > maxV) maxV = v;
            }

            // ゼロ除算対策
            if (std::abs(maxU - minU) < EPSILON) maxU = minU + 1.0f;
            if (std::abs(maxV - minV) < EPSILON) maxV = minV + 1.0f;
        }

        // 頂点生成
        for (const auto& pos : loop)
        {
            Vertex v;
            v.position = pos;
            XMStoreFloat3(&v.normal, planeNormal);
            v.color = { 1, 1, 1, 1 };

            if (texId != -1)
            {
                // バウンディングボックスに合わせてUVを正規化 (0.0 - 1.0)
                XMVECTOR vPos = XMLoadFloat3(&pos);
                float rawU = XMVectorGetX(XMVector3Dot(vPos, vTangent));
                float rawV = XMVectorGetX(XMVector3Dot(vPos, vBinormal));

                v.uv.x = (rawU - minU) / (maxU - minU);
                v.uv.y = (rawV - minV) / (maxV - minV);
            }
            else
            {
                v.uv = CalculatePlanarUV(pos, vTangent, vBinormal);
            }

            mesh.vertices.push_back(v);
        }

        TriangulateEarClipping(loop, planeNormal, mesh.indices, baseIndex);
    }
}

// 平面距離計算

static float DistanceToPlane(const XMFLOAT3& vertex, const XMVECTOR& planeEq)
{
    return XMVectorGetX(XMPlaneDotCoord(planeEq, XMLoadFloat3(&vertex)));
}

// 頂点補間

static Vertex InterpolateVertex(const Vertex& v1, const Vertex& v2, float t)
{
    Vertex out;
    XMVECTOR p1  = XMLoadFloat3(&v1.position);
    XMVECTOR p2  = XMLoadFloat3(&v2.position);
    XMVECTOR n1  = XMLoadFloat3(&v1.normal);
    XMVECTOR n2  = XMLoadFloat3(&v2.normal);
    XMVECTOR c1  = XMLoadFloat4(&v1.color);
    XMVECTOR c2  = XMLoadFloat4(&v2.color);
    XMVECTOR uv1 = XMLoadFloat2(&v1.uv);
    XMVECTOR uv2 = XMLoadFloat2(&v2.uv);

    XMStoreFloat3(&out.position, XMVectorLerp(p1, p2, t));
    XMStoreFloat3(&out.normal,   XMVector3Normalize(XMVectorLerp(n1, n2, t)));
    XMStoreFloat4(&out.color,    XMVectorLerp(c1, c2, t));
    XMStoreFloat2(&out.uv,       XMVectorLerp(uv1, uv2, t));
    return out;
}

// 三角形を1つ出力するヘルパー

/** @brief 三角形を1つ出力（頂点3つ＋インデックス） */
static void EmitTriangle(MeshData& mesh, const Vertex& a, const Vertex& b, const Vertex& c)
{
    unsigned int base = (unsigned int)mesh.vertices.size();
    mesh.vertices.push_back(a);
    mesh.vertices.push_back(b);
    mesh.vertices.push_back(c);
    mesh.indices.push_back(base);
    mesh.indices.push_back(base + 1);
    mesh.indices.push_back(base + 2);
}

/// 四角形を2三角形で出力 (v0,v1,v2) + (v0,v2,v3)
static void EmitQuad(MeshData& mesh, const Vertex& v0, const Vertex& v1, const Vertex& v2, const Vertex& v3)
{
    unsigned int base = (unsigned int)mesh.vertices.size();
    mesh.vertices.push_back(v0);
    mesh.vertices.push_back(v1);
    mesh.vertices.push_back(v2);
    mesh.vertices.push_back(v3);
    mesh.indices.push_back(base);
    mesh.indices.push_back(base + 1);
    mesh.indices.push_back(base + 2);
    mesh.indices.push_back(base);
    mesh.indices.push_back(base + 2);
    mesh.indices.push_back(base + 3);
}

// 共通の座標変換 + 早期棄却
// 成功時に LocalSliceParams を返す。切断不要なら false を返す。

static bool PrepareSliceParams(
    MODEL* targetModel,
    const XMFLOAT4X4& worldMatrix,
    const XMFLOAT3& planePoint,
    const XMFLOAT3& planeNormal,
    LocalSliceParams& outParams)
{
    // ワールド → ローカル座標変換
    XMMATRIX mWorld    = XMLoadFloat4x4(&worldMatrix);
    XMMATRIX mInvWorld = XMMatrixInverse(nullptr, mWorld);

    XMVECTOR vLocalPoint  = XMVector3TransformCoord(XMLoadFloat3(&planePoint), mInvWorld);
    XMVECTOR vLocalNormal = XMVector3Normalize(XMVector3TransformNormal(XMLoadFloat3(&planeNormal), mInvWorld));

    float distanceD     = -XMVectorGetX(XMVector3Dot(vLocalNormal, vLocalPoint));
    XMVECTOR planeEq    = XMVectorSetW(vLocalNormal, distanceD);

    // モデル全体のAABBで早期棄却
    PlaneSide modelSide = CheckAABBPlane(targetModel->local_aabb, planeEq);
    if (modelSide != PlaneSide::Intersect)
        return false;

    // 断面用マテリアルインデックス
    int capMaterialIndex = 0;
    if (targetModel->pSliceTextureSRV != nullptr)
    {
        capMaterialIndex = (int)targetModel->materials.size();
    }
    else if (!targetModel->Meshes.empty())
    {
        capMaterialIndex = targetModel->Meshes[0].materialIndex;
    }

    // ブレンド比率
    int texId = targetModel->GetSlicedTexturId();
    float blendRatio = (texId != -1) ? CalculateSliceBlendRatio(vLocalNormal) : -1.0f;

    outParams.localNormal       = vLocalNormal;
    outParams.planeEq           = planeEq;
    outParams.capMaterialIndex  = capMaterialIndex;
    outParams.sliceTextureId    = texId;
    outParams.blendRatio        = blendRatio;

    return true;
}

// 共通のメッシュ分割処理
// AABBカリング + 三角形分割 + 断面エッジ収集 + 断面メッシュ生成

static void SplitMeshes(
    MODEL* targetModel,
    const LocalSliceParams& params,
    std::vector<MeshData>& outFront,
    std::vector<MeshData>& outBack)
{
    std::vector<RawEdge> allFrontCapEdges, allBackCapEdges;

    // --- メッシュごとの分割 ---
    for (const auto& srcMesh : targetModel->Meshes)
    {
        // メッシュ単位のAABBカリング
        AABB meshAABB     = CalculateMeshAABB(srcMesh);
        PlaneSide meshSide = CheckAABBPlane(meshAABB, params.planeEq);

        if (meshSide == PlaneSide::Front)
        {
            outFront.push_back(srcMesh);
            continue;
        }
        if (meshSide == PlaneSide::Back)
        {
            outBack.push_back(srcMesh);
            continue;
        }

        // 交差メッシュ：三角形ごとに分割
        MeshData frontMesh, backMesh;
        frontMesh.materialIndex = srcMesh.materialIndex;
        backMesh.materialIndex  = srcMesh.materialIndex;

        const auto& verts = srcMesh.vertices;
        const auto& inds  = srcMesh.indices;

        for (size_t i = 0; i < inds.size(); i += 3)
        {
            if (inds[i + 2] >= verts.size())
                continue;

            Vertex v[3]  = { verts[inds[i]], verts[inds[i + 1]], verts[inds[i + 2]] };
            float  d[3];
            bool   front[3];

            for (int j = 0; j < 3; ++j)
            {
                d[j]     = DistanceToPlane(v[j].position, params.planeEq);
                front[j] = (d[j] >= 0);
            }

            // 全頂点が同一側：そのまま振り分け
            if (front[0] == front[1] && front[1] == front[2])
            {
                MeshData& dest = front[0] ? frontMesh : backMesh;
                EmitTriangle(dest, v[0], v[1], v[2]);
                continue;
            }

            // 孤立頂点の特定
            int isoIdx;
            if (front[0] == front[2])      isoIdx = 1;
            else if (front[1] == front[2]) isoIdx = 0;
            else                           isoIdx = 2;

            // 孤立頂点を先頭に並べ替え
            Vertex tri[3]  = { v[isoIdx], v[(isoIdx + 1) % 3], v[(isoIdx + 2) % 3] };
            float  dist[3] = { d[isoIdx], d[(isoIdx + 1) % 3], d[(isoIdx + 2) % 3] };

            float t0 = dist[0] / (dist[0] - dist[1]);
            float t1 = dist[0] / (dist[0] - dist[2]);

            Vertex split0 = InterpolateVertex(tri[0], tri[1], t0);
            Vertex split1 = InterpolateVertex(tri[0], tri[2], t1);

            // 断面エッジ収集（front/backで逆向き）
            if (front[isoIdx])
            {
                allFrontCapEdges.push_back({ split0.position, split1.position });
                allBackCapEdges.push_back({ split1.position, split0.position });
            }
            else
            {
                allBackCapEdges.push_back({ split0.position, split1.position });
                allFrontCapEdges.push_back({ split1.position, split0.position });
            }

            // 孤立側：三角形1つ、多数側：四角形（三角形2つ）
            MeshData& isoDest = front[isoIdx] ? frontMesh : backMesh;
            MeshData& majDest = front[isoIdx] ? backMesh  : frontMesh;

            EmitTriangle(isoDest, tri[0], split0, split1);
            EmitQuad(majDest, tri[1], tri[2], split1, split0);
        }

        if (!frontMesh.vertices.empty())
            outFront.push_back(frontMesh);
        if (!backMesh.vertices.empty())
            outBack.push_back(backMesh);
    }

    // --- 断面メッシュ生成 ---
    auto generateCap = [&](std::vector<RawEdge>& edges, const XMVECTOR& capNormal, std::vector<MeshData>& dest)
    {
        if (edges.empty())
            return;
        MeshData capMesh;
        capMesh.materialIndex   = params.capMaterialIndex;
        CreateCapMesh(params.sliceTextureId, capMesh, edges, capNormal);
        capMesh.sliceBlendRatio = params.blendRatio;
        if (!capMesh.vertices.empty())
            dest.push_back(capMesh);
    };

    generateCap(allFrontCapEdges, -params.localNormal, outFront);  // Frontは逆法線
    generateCap(allBackCapEdges,   params.localNormal, outBack);
}

// 公開API

bool Slicer::Slice(
    MODEL* targetModel,
    const XMFLOAT4X4& worldMatrix,
    const XMFLOAT3& planePoint,
    const XMFLOAT3& planeNormal,
    MODEL** outFront,
    MODEL** outBack)
{
    *outFront = nullptr;
    *outBack  = nullptr;

    if (!targetModel)
        return false;

    LocalSliceParams params;
    if (!PrepareSliceParams(targetModel, worldMatrix, planePoint, planeNormal, params))
        return false;

    std::vector<MeshData> frontMeshes, backMeshes;
    SplitMeshes(targetModel, params, frontMeshes, backMeshes);

    if (frontMeshes.empty() || backMeshes.empty())
        return false;

    *outFront = ModelCreateFromData(std::move(frontMeshes), targetModel);
    *outBack  = ModelCreateFromData(std::move(backMeshes), targetModel);

    return true;
}

bool Slicer::SliceCPUOnly(
    MODEL* targetModel,
    const XMFLOAT4X4& worldMatrix,
    const XMFLOAT3& planePoint,
    const XMFLOAT3& planeNormal,
    std::vector<MeshData>& outFrontMeshes,
    std::vector<MeshData>& outBackMeshes)
{
    if (!targetModel)
        return false;

    LocalSliceParams params;
    if (!PrepareSliceParams(targetModel, worldMatrix, planePoint, planeNormal, params))
        return false;

    SplitMeshes(targetModel, params, outFrontMeshes, outBackMeshes);

    return !outFrontMeshes.empty() && !outBackMeshes.empty();
}

// Slicer 静的メンバー関数の委譲

Vertex Slicer::Interpolate(const Vertex& v1, const Vertex& v2, float t)
{
    return InterpolateVertex(v1, v2, t);
}

float Slicer::GetDistanceToPlane(const XMFLOAT3& vertex, const XMVECTOR& planeVector)
{
    return DistanceToPlane(vertex, planeVector);
}
