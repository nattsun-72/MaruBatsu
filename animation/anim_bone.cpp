/****************************************
 * @file anim_bone.cpp
 * @brief ボーン計算（キーフレーム補間・階層構造・ボーン行列計算）
 * @author Natsume Shidara
 * @date 2025/06/06
 *
 * - ノード階層ツリー構築
 * - ボーン・ウェイト抽出
 * - キーフレーム補間（Position/Rotation/Scaling）
 * - ブレンド付きボーン行列計算
 ****************************************/

#include "anim_bone.h"
#include <DirectXMath.h>
#include <vector>
#include <string>
#include <algorithm>

using namespace DirectX;

// 行列変換: aiMatrix4x4 → XMFLOAT4X4
// aiMatrix4x4: row-major storage, column-vector convention (v' = M * v).
// XMFLOAT4X4: row-major storage, row-vector convention (v' = v * M).
// Transpose is required to convert between conventions.
XMFLOAT4X4 AiMatrixToXM(const aiMatrix4x4& m)
{
    XMFLOAT4X4 result;
    result._11 = m.a1; result._12 = m.b1; result._13 = m.c1; result._14 = m.d1;
    result._21 = m.a2; result._22 = m.b2; result._23 = m.c2; result._24 = m.d2;
    result._31 = m.a3; result._32 = m.b3; result._33 = m.c3; result._34 = m.d3;
    result._41 = m.a4; result._42 = m.b4; result._43 = m.c4; result._44 = m.d4;
    return result;
}

// ノード階層構築
void AnimBone_BuildNodeHierarchy(ANIM_MODEL* model, const aiNode* node, int parentIndex)
{
    int currentIndex = static_cast<int>(model->nodeHierarchy.size());

    ANIM_MODEL::NodeInfo info;
    info.name = node->mName.C_Str();
    info.localTransform = AiMatrixToXM(node->mTransformation);
    info.localTransformAi = node->mTransformation;
    info.parentIndex = parentIndex;

    model->nodeHierarchy.push_back(info);
    model->nodeNameToIndex[info.name] = currentIndex;

    if (parentIndex >= 0)
    {
        model->nodeHierarchy[parentIndex].children.push_back(currentIndex);
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        AnimBone_BuildNodeHierarchy(model, node->mChildren[i], currentIndex);
    }
}

// ボーン・ウェイト抽出
void AnimBone_ExtractBones(ANIM_MODEL* model, const aiMesh* mesh,
    std::vector<SkinnedVertex>& vertices)
{
    for (unsigned int b = 0; b < mesh->mNumBones; b++)
    {
        aiBone* bone = mesh->mBones[b];
        std::string boneName = bone->mName.C_Str();

        int boneIndex;
        auto it = model->boneNameToIndex.find(boneName);
        if (it == model->boneNameToIndex.end())
        {
            boneIndex = static_cast<int>(model->bones.size());
            if (boneIndex >= MAX_BONES)
            {
                OutputDebugStringA("[AnimModel] WARNING: Bone count exceeds MAX_BONES, skipping bone.\n");
                continue;
            }
            BoneInfo boneInfo;
            boneInfo.name = boneName;
            boneInfo.offsetMatrix = AiMatrixToXM(bone->mOffsetMatrix);
            boneInfo.offsetMatrixAi = bone->mOffsetMatrix;
            XMStoreFloat4x4(&boneInfo.finalTransform, XMMatrixIdentity());
            boneInfo.parentIndex = -1;
            model->bones.push_back(boneInfo);
            model->boneNameToIndex[boneName] = boneIndex;
        }
        else
        {
            boneIndex = it->second;
        }

        for (unsigned int w = 0; w < bone->mNumWeights; w++)
        {
            unsigned int vertId = bone->mWeights[w].mVertexId;
            float weight = bone->mWeights[w].mWeight;

            for (int k = 0; k < MAX_BONE_INFLUENCE; k++)
            {
                if (vertices[vertId].boneWeights[k] == 0.0f)
                {
                    vertices[vertId].boneIndices[k] = boneIndex;
                    vertices[vertId].boneWeights[k] = weight;
                    break;
                }
            }
        }
    }
}

// ボーン親インデックス解決
void AnimBone_ResolveBoneParents(ANIM_MODEL* model)
{
    for (size_t b = 0; b < model->bones.size(); b++)
    {
        auto& bone = model->bones[b];
        auto nodeIt = model->nodeNameToIndex.find(bone.name);
        if (nodeIt == model->nodeNameToIndex.end())
        {
            bone.parentIndex = -1;
            continue;
        }

        int nodeIdx = nodeIt->second;
        int parentNodeIdx = model->nodeHierarchy[nodeIdx].parentIndex;

        bone.parentIndex = -1;
        while (parentNodeIdx >= 0)
        {
            const std::string& parentNodeName = model->nodeHierarchy[parentNodeIdx].name;
            auto boneIt = model->boneNameToIndex.find(parentNodeName);
            if (boneIt != model->boneNameToIndex.end())
            {
                bone.parentIndex = boneIt->second;
                break;
            }
            parentNodeIdx = model->nodeHierarchy[parentNodeIdx].parentIndex;
        }
    }
}

// キーフレーム検索
static unsigned int FindPositionKeyIndex(const aiNodeAnim* channel, double time)
{
    if (channel->mNumPositionKeys <= 1) return 0;
    for (unsigned int i = 0; i < channel->mNumPositionKeys - 1; i++)
    {
        if (time < channel->mPositionKeys[i + 1].mTime)
            return i;
    }
    return channel->mNumPositionKeys - 1;
}

static unsigned int FindRotationKeyIndex(const aiNodeAnim* channel, double time)
{
    if (channel->mNumRotationKeys <= 1) return 0;
    for (unsigned int i = 0; i < channel->mNumRotationKeys - 1; i++)
    {
        if (time < channel->mRotationKeys[i + 1].mTime)
            return i;
    }
    return channel->mNumRotationKeys - 1;
}

static unsigned int FindScalingKeyIndex(const aiNodeAnim* channel, double time)
{
    if (channel->mNumScalingKeys <= 1) return 0;
    for (unsigned int i = 0; i < channel->mNumScalingKeys - 1; i++)
    {
        if (time < channel->mScalingKeys[i + 1].mTime)
            return i;
    }
    return channel->mNumScalingKeys - 1;
}

// キーフレーム補間
static aiVector3D InterpolatePositionAi(const aiNodeAnim* channel, double time)
{
    if (channel->mNumPositionKeys == 0)
        return aiVector3D(0.0f, 0.0f, 0.0f);
    if (channel->mNumPositionKeys == 1)
        return channel->mPositionKeys[0].mValue;

    unsigned int idx = FindPositionKeyIndex(channel, time);
    unsigned int nextIdx = idx + 1;
    if (nextIdx >= channel->mNumPositionKeys) nextIdx = idx;

    double dt = channel->mPositionKeys[nextIdx].mTime - channel->mPositionKeys[idx].mTime;
    float factor = (dt > 0.0) ? static_cast<float>((time - channel->mPositionKeys[idx].mTime) / dt) : 0.0f;
    if (factor < 0.0f) factor = 0.0f;
    if (factor > 1.0f) factor = 1.0f;

    const aiVector3D& s = channel->mPositionKeys[idx].mValue;
    const aiVector3D& e = channel->mPositionKeys[nextIdx].mValue;
    return s + factor * (e - s);
}

static aiQuaternion InterpolateRotationAi(const aiNodeAnim* channel, double time)
{
    if (channel->mNumRotationKeys == 0)
        return aiQuaternion(1.0f, 0.0f, 0.0f, 0.0f);
    if (channel->mNumRotationKeys == 1)
        return channel->mRotationKeys[0].mValue;

    unsigned int idx = FindRotationKeyIndex(channel, time);
    unsigned int nextIdx = idx + 1;
    if (nextIdx >= channel->mNumRotationKeys) nextIdx = idx;

    double dt = channel->mRotationKeys[nextIdx].mTime - channel->mRotationKeys[idx].mTime;
    float factor = (dt > 0.0) ? static_cast<float>((time - channel->mRotationKeys[idx].mTime) / dt) : 0.0f;
    if (factor < 0.0f) factor = 0.0f;
    if (factor > 1.0f) factor = 1.0f;

    const aiQuaternion& s = channel->mRotationKeys[idx].mValue;
    const aiQuaternion& e = channel->mRotationKeys[nextIdx].mValue;

    aiQuaternion result;
    aiQuaternion::Interpolate(result, s, e, factor);
    return result;
}

static aiVector3D InterpolateScalingAi(const aiNodeAnim* channel, double time)
{
    if (channel->mNumScalingKeys == 0)
        return aiVector3D(1.0f, 1.0f, 1.0f);
    if (channel->mNumScalingKeys == 1)
        return channel->mScalingKeys[0].mValue;

    unsigned int idx = FindScalingKeyIndex(channel, time);
    unsigned int nextIdx = idx + 1;
    if (nextIdx >= channel->mNumScalingKeys) nextIdx = idx;

    double dt = channel->mScalingKeys[nextIdx].mTime - channel->mScalingKeys[idx].mTime;
    float factor = (dt > 0.0) ? static_cast<float>((time - channel->mScalingKeys[idx].mTime) / dt) : 0.0f;
    if (factor < 0.0f) factor = 0.0f;
    if (factor > 1.0f) factor = 1.0f;

    const aiVector3D& s = channel->mScalingKeys[idx].mValue;
    const aiVector3D& e = channel->mScalingKeys[nextIdx].mValue;
    return s + factor * (e - s);
}

// チャンネル検索
static const aiNodeAnim* FindNodeAnim(const aiAnimation* anim, const std::string& nodeName)
{
    for (unsigned int i = 0; i < anim->mNumChannels; i++)
    {
        if (std::string(anim->mChannels[i]->mNodeName.C_Str()) == nodeName)
            return anim->mChannels[i];
    }
    return nullptr;
}

// ノードローカル変換評価（aiMatrix4x4で返す）
static aiMatrix4x4 EvaluateNodeTransformAi(const aiAnimation* anim, const std::string& nodeName,
    double time, const aiMatrix4x4& defaultTransform)
{
    const aiNodeAnim* channel = FindNodeAnim(anim, nodeName);
    if (!channel) return defaultTransform;

    aiVector3D pos = InterpolatePositionAi(channel, time);
    aiQuaternion rot = InterpolateRotationAi(channel, time);
    aiVector3D scl = InterpolateScalingAi(channel, time);

    // aiMatrix4x4(scaling, rotation, position) builds the SRT matrix
    // in Assimp's own column-vector convention, ensuring consistency.
    return aiMatrix4x4(scl, rot, pos);
}

// aiMatrix4x4ブレンド（SRT分解→補間→再合成）
static aiMatrix4x4 BlendAiMatrices(const aiMatrix4x4& matA, const aiMatrix4x4& matB, float factor)
{
    aiVector3D sA, sB, pA, pB;
    aiQuaternion rA, rB;

    matA.Decompose(sA, rA, pA);
    matB.Decompose(sB, rB, pB);

    // Lerp scale and position, slerp rotation
    aiVector3D sF = sA + factor * (sB - sA);
    aiVector3D pF = pA + factor * (pB - pA);
    aiQuaternion rF;
    aiQuaternion::Interpolate(rF, rA, rB, factor);

    return aiMatrix4x4(sF, rF, pF);
}

// ボーン行列計算（メイン）
void AnimBone_CalcBoneMatrices(ANIM_MODEL* model,
    const AnimationClip* clipA, double timeA,
    const AnimationClip* clipB, double timeB,
    float blendFactor)
{
    if (!clipA) return;

    const aiAnimation* animA = clipA->aiSceneRef->mAnimations[clipA->animIndex];
    const aiAnimation* animB = (clipB) ? clipB->aiSceneRef->mAnimations[clipB->animIndex] : nullptr;

    // All computation is done in Assimp's column-vector convention (v' = M * v).
    //
    // ルート逆行列はロード時に一度だけ計算し、model->globalInverseTransformAi に保存済み。
    // 毎フレーム計算すると、ルートノードにアニメーションが含まれる場合に
    // 基準が動いて不安定になる。
    // また、ロード時にスケール行列がルートノードに合成されているため、
    // この逆行列にはスケールの逆も含まれ、finalBone計算で正しくスケールが反映される。
    const aiMatrix4x4& rootInv = model->globalInverseTransformAi;

    // Accumulate node global transforms: Global = Parent * Local (column-vector convention)
    // ルートノード（index 0）のlocalTransformAiにはスケール行列が合成済みなので、
    // 全てのボーンの平行移動・回転だけでなく一貫してスケールが適用される。
    std::vector<aiMatrix4x4> nodeGlobalTransforms(model->nodeHierarchy.size());

    for (size_t i = 0; i < model->nodeHierarchy.size(); i++)
    {
        auto& node = model->nodeHierarchy[i];
        const aiMatrix4x4& defaultLocal = node.localTransformAi;

        aiMatrix4x4 localA = EvaluateNodeTransformAi(animA, node.name, timeA, defaultLocal);

        aiMatrix4x4 localFinal;
        if (animB && blendFactor > 0.0f)
        {
            aiMatrix4x4 localB = EvaluateNodeTransformAi(animB, node.name, timeB, defaultLocal);
            localFinal = BlendAiMatrices(localA, localB, blendFactor);
        }
        else
        {
            localFinal = localA;
        }

        if (node.parentIndex >= 0)
            nodeGlobalTransforms[i] = nodeGlobalTransforms[node.parentIndex] * localFinal;
        else
            nodeGlobalTransforms[i] = localFinal;
    }

    // FinalBone = GlobalInverse * NodeGlobal * OffsetMatrix
    // GlobalInverseにはスケール逆が含まれ、NodeGlobalにはルートのスケールが含まれるため、
    // 結果として正しいスケールの最終行列が得られる。
    for (size_t b = 0; b < model->bones.size(); b++)
    {
        auto& bone = model->bones[b];
        auto nodeIt = model->nodeNameToIndex.find(bone.name);
        if (nodeIt != model->nodeNameToIndex.end())
        {
            aiMatrix4x4 finalAi = rootInv * nodeGlobalTransforms[nodeIt->second] * bone.offsetMatrixAi;

            // Transpose via AiMatrixToXM: Assimp(col-vec) → DX(row-vec row-major)
            // Upload to HLSL column-major will transpose again → correct for mul(v, M)
            model->boneMatrices[b] = AiMatrixToXM(finalAi);
        }
        else
        {
            XMStoreFloat4x4(&model->boneMatrices[b], XMMatrixIdentity());
        }
    }
}
