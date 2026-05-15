/****************************************
 * @file animation_model.h
 * @brief アニメーションモデルのデータ構造と公開API定義
 * @author Natsume Shidara
 * @date 2025/06/06
 *
 * スキニングメッシュ・ボーン・アニメーションクリップ・
 * 再生状態を管理するための構造体と関数宣言
 ****************************************/

#ifndef ANIMATION_MODEL_H
#define ANIMATION_MODEL_H

#include <unordered_map>
#include <vector>
#include <string>
#include <d3d11.h>
#include <DirectXMath.h>

#include "assimp/cimport.h"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "assimp/matrix4x4.h"
#include "assimp/anim.h"

#pragma comment (lib, "assimp-vc143-mt.lib")

#include "collision.h"

constexpr int MAX_BONES = 128;            ///< ボーンの最大数
constexpr int MAX_BONE_INFLUENCE = 4;     ///< 1頂点に影響するボーンの最大数

/** @brief スキニングメッシュ用の頂点データ */
struct SkinnedVertex
{
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 normal;
    DirectX::XMFLOAT4 color;
    DirectX::XMFLOAT2 uv;
    unsigned int boneIndices[MAX_BONE_INFLUENCE];
    float boneWeights[MAX_BONE_INFLUENCE];
};

/** @brief ボーン情報（オフセット行列・最終変換行列・親子関係） */
struct BoneInfo
{
    DirectX::XMFLOAT4X4 offsetMatrix{};
    DirectX::XMFLOAT4X4 finalTransform{};
    aiMatrix4x4 offsetMatrixAi{};  // Original Assimp offset matrix (column-vector convention)
    std::string name;
    int parentIndex = -1;
};

/** @brief アニメーション再生状態 */
enum class AnimPlayState
{
    Stopped,
    Playing,
    Paused,
    PlayingReverse,
};

/** @brief アニメーションクリップ（1つのアニメーション動作に対応） */
struct AnimationClip
{
    const aiScene* aiSceneRef;
    std::string name;
    double duration;
    double ticksPerSecond;
    unsigned int animIndex;
};

/** @brief アニメーションブレンド（クロスフェード）の状態情報 */
struct AnimBlendInfo
{
    int clipIndexA;
    int clipIndexB;
    float blendFactor;
    float blendDuration;
    float blendTimer;
    double startTimeA;      ///< ブレンド開始時のclipA再生時刻（tick単位）
    bool isBlending;
};

/** @brief アニメーションモデル本体（メッシュ・ボーン・再生状態を統合管理） */
struct ANIM_MODEL
{
    const aiScene* AiScene;

    ID3D11Buffer** VertexBuffer;
    ID3D11Buffer** IndexBuffer;

    std::unordered_map<std::string, ID3D11ShaderResourceView*> Texture;
    std::vector<ID3D11ShaderResourceView*> materials;

    unsigned int meshCount;
    unsigned int* IndexCount;
    std::vector<unsigned int> meshMaterialIndices;  ///< メッシュごとのマテリアルインデックス（AiScene非依存）

    std::vector<BoneInfo> bones;
    std::unordered_map<std::string, int> boneNameToIndex;
    DirectX::XMFLOAT4X4 globalInverseTransform;
    aiMatrix4x4 globalInverseTransformAi;   // Cached root inverse (column-vector, includes scale inverse)

    std::vector<AnimationClip> clips;
    std::unordered_map<std::string, int> clipNameToIndex;

    int currentClipIndex;
    double currentTime;
    float playSpeed;
    AnimPlayState playState;
    bool loop;
    bool pausedFromReverse;

    AnimBlendInfo blend;

    DirectX::XMFLOAT4X4 boneMatrices[MAX_BONES];

    AABB local_aabb;

    std::string resourceKey;
    int refCount;

    /** @brief ノード階層の1ノード分の情報 */
    struct NodeInfo
    {
        std::string name;
        DirectX::XMFLOAT4X4 localTransform{};
        aiMatrix4x4 localTransformAi{};  // Original Assimp matrix (column-vector convention)
        int parentIndex = -1;
        std::vector<int> children;
    };
    std::vector<NodeInfo> nodeHierarchy;
    std::unordered_map<std::string, int> nodeNameToIndex;
};

// モデル読み込み・解放

/** @brief FBXファイルからアニメーションモデルを読み込む（キャッシュ付き） */
ANIM_MODEL* AnimModel_Load(const char* fileName, float scale = 1.0f);

/** @brief 参照カウントを加算 */
void AnimModel_AddRef(ANIM_MODEL* model);

/** @brief 参照カウントを減算し、0になったらリソースを解放 */
void AnimModel_Release(ANIM_MODEL* model);

/** @brief 全モデルキャッシュを破棄 */
void AnimModel_Finalize();

/** @brief 追加アニメーションクリップをFBXファイルから読み込む */
bool AnimModel_LoadClip(ANIM_MODEL* model, const char* clipFileName, const char* clipName);

// 再生制御

/** @brief クリップ名を指定してアニメーション再生 */
void AnimModel_Play(ANIM_MODEL* model, const char* clipName, bool loop = true, float speed = 1.0f);

/** @brief クリップインデックスを指定してアニメーション再生 */
void AnimModel_PlayByIndex(ANIM_MODEL* model, int clipIndex, bool loop = true, float speed = 1.0f);

/** @brief アニメーション停止（時刻を0にリセット） */
void AnimModel_Stop(ANIM_MODEL* model);

/** @brief アニメーション一時停止 */
void AnimModel_Pause(ANIM_MODEL* model);

/** @brief 一時停止中のアニメーションを再開 */
void AnimModel_Resume(ANIM_MODEL* model);

/** @brief アニメーション逆再生 */
void AnimModel_PlayReverse(ANIM_MODEL* model, float speed = 1.0f);

/** @brief 再生速度を設定 */
void AnimModel_SetSpeed(ANIM_MODEL* model, float speed);

/** @brief 再生時刻を直接設定 */
void AnimModel_SetTime(ANIM_MODEL* model, double time);

// ブレンド

/** @brief クリップ名を指定してアニメーションブレンド（クロスフェード） */
void AnimModel_BlendTo(ANIM_MODEL* model, const char* clipName, float blendDuration, bool loop = true);

/** @brief クリップインデックスを指定してアニメーションブレンド */
void AnimModel_BlendToByIndex(ANIM_MODEL* model, int clipIndex, float blendDuration, bool loop = true);

// 更新・描画

/** @brief アニメーションの時刻進行とボーン行列計算 */
void AnimModel_Update(ANIM_MODEL* model, float deltaTime);

/** @brief スキニングメッシュの描画 */
void AnimModel_Draw(ANIM_MODEL* model, const DirectX::XMMATRIX& mtxWorld);

/** @brief シャドウマップ用のスキニングメッシュ描画 */
void AnimModel_DrawShadow(ANIM_MODEL* model, const DirectX::XMMATRIX& mtxWorld);

// ゲッター

/** @brief ワールド座標でのAABBを取得 */
AABB AnimModel_GetAABB(ANIM_MODEL* model, const DirectX::XMFLOAT3& position);

/** @brief 登録されたクリップ数を取得 */
int AnimModel_GetClipCount(ANIM_MODEL* model);

/** @brief 指定インデックスのクリップ名を取得 */
const char* AnimModel_GetClipName(ANIM_MODEL* model, int index);

/** @brief 現在の再生時刻（tick単位）を取得 */
double AnimModel_GetCurrentTime(ANIM_MODEL* model);

/** @brief 指定クリップの総再生時間（tick単位）を取得 */
double AnimModel_GetClipDuration(ANIM_MODEL* model, int clipIndex);

/** @brief アニメーションが再生中かどうかを返す */
bool AnimModel_IsPlaying(ANIM_MODEL* model);

#endif
