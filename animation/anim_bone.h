/****************************************
 * @file anim_bone.h
 * @brief ボーン計算（キーフレーム補間・階層構造・ボーン行列計算）
 * @author Natsume Shidara
 * @date 2025/06/06
 ****************************************/

#ifndef ANIM_BONE_H
#define ANIM_BONE_H

#include "animation_model.h"

// ノード階層構造構築
void AnimBone_BuildNodeHierarchy(ANIM_MODEL* model, const aiNode* node, int parentIndex);

// ボーン抽出
void AnimBone_ExtractBones(ANIM_MODEL* model, const aiMesh* mesh,
    std::vector<SkinnedVertex>& vertices);

// ボーン親インデックス解決
void AnimBone_ResolveBoneParents(ANIM_MODEL* model);

// ボーン行列計算（アニメーション評価）
void AnimBone_CalcBoneMatrices(ANIM_MODEL* model,
    const AnimationClip* clipA, double timeA,
    const AnimationClip* clipB, double timeB,
    float blendFactor);

// 行列変換ユーティリティ
DirectX::XMFLOAT4X4 AiMatrixToXM(const aiMatrix4x4& m);

#endif // ANIM_BONE_H
