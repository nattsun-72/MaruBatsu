/****************************************
 * @file animation_model.cpp
 * @brief アニメーションモデル管理（キャッシュ・描画・解放・ゲッター）
 * @author Natsume Shidara
 * @date 2025/06/06
 *
 * 各機能は以下のファイルに分離:
 *   anim_bone.cpp     - ボーン計算・キーフレーム補間
 *   anim_loader.cpp   - FBX読み込み・メッシュ・マテリアル
 *   anim_playback.cpp - 再生制御・ブレンド・更新
 ****************************************/

#include "animation_model.h"
#include "anim_bone.h"
#include "anim_loader.h"
#include "anim_playback.h"
#include "direct3d.h"
#include "shader_skinned.h"
#include "shader_shadow_map_skinned.h"

#include <DirectXMath.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>

using namespace DirectX;

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(x) if(x){ x->Release(); x = nullptr; }
#endif

// モデルキャッシュ
static std::unordered_map<std::string, ANIM_MODEL*> g_AnimModelCache;

// 読み込み（キャッシュ付き）
ANIM_MODEL* AnimModel_Load(const char* fileName, float scale)
{
    std::string key = std::string(fileName) + "|scale=" + std::to_string(scale);
    auto it = g_AnimModelCache.find(key);
    if (it != g_AnimModelCache.end())
    {
        AnimModel_AddRef(it->second);
        return it->second;
    }

    ANIM_MODEL* model = AnimLoader_LoadFromFile(fileName, scale);
    if (!model) return nullptr;

    model->resourceKey = key;
    g_AnimModelCache[key] = model;

    return model;
}

// クリップ追加読み込み（デリゲート）
bool AnimModel_LoadClip(ANIM_MODEL* model, const char* clipFileName, const char* clipName)
{
    return AnimLoader_LoadClip(model, clipFileName, clipName);
}

// 参照カウント
void AnimModel_AddRef(ANIM_MODEL* model)
{
    if (model) model->refCount++;
}

// 解放
void AnimModel_Release(ANIM_MODEL* model)
{
    if (!model) return;

    model->refCount--;
    if (model->refCount > 0) return;

    if (!model->resourceKey.empty())
        g_AnimModelCache.erase(model->resourceKey);

    if (model->VertexBuffer)
    {
        for (unsigned int m = 0; m < model->meshCount; m++)
            SAFE_RELEASE(model->VertexBuffer[m]);
        delete[] model->VertexBuffer;
    }

    if (model->IndexBuffer)
    {
        for (unsigned int m = 0; m < model->meshCount; m++)
            SAFE_RELEASE(model->IndexBuffer[m]);
        delete[] model->IndexBuffer;
    }

    delete[] model->IndexCount;
    model->IndexCount = nullptr;

    for (auto* srv : model->materials)
        SAFE_RELEASE(srv);
    model->materials.clear();

    for (auto& tex : model->Texture)
    {
        if (tex.second)
        {
            tex.second->Release();
            tex.second = nullptr;
        }
    }
    model->Texture.clear();

    std::unordered_set<const aiScene*> releasedScenes;
    if (model->AiScene)
    {
        releasedScenes.insert(model->AiScene);
        aiReleaseImport(model->AiScene);
        model->AiScene = nullptr;
    }

    for (auto& clip : model->clips)
    {
        if (clip.aiSceneRef && releasedScenes.find(clip.aiSceneRef) == releasedScenes.end())
        {
            releasedScenes.insert(clip.aiSceneRef);
            aiReleaseImport(clip.aiSceneRef);
        }
        clip.aiSceneRef = nullptr;
    }

    delete model;
}

// 全体破棄
void AnimModel_Finalize()
{
    std::vector<ANIM_MODEL*> models;
    for (auto& pair : g_AnimModelCache)
        models.push_back(pair.second);
    g_AnimModelCache.clear();

    for (auto* m : models)
    {
        m->resourceKey.clear();
        m->refCount = 1;
        AnimModel_Release(m);
    }

    AnimLoader_ReleaseWhiteTexture();
}

// 再生制御（デリゲート）
void AnimModel_Play(ANIM_MODEL* model, const char* clipName, bool loop, float speed)
{
    AnimPlayback_Play(model, clipName, loop, speed);
}

void AnimModel_PlayByIndex(ANIM_MODEL* model, int clipIndex, bool loop, float speed)
{
    AnimPlayback_PlayByIndex(model, clipIndex, loop, speed);
}

void AnimModel_Stop(ANIM_MODEL* model)
{
    AnimPlayback_Stop(model);
}

void AnimModel_Pause(ANIM_MODEL* model)
{
    AnimPlayback_Pause(model);
}

void AnimModel_Resume(ANIM_MODEL* model)
{
    AnimPlayback_Resume(model);
}

void AnimModel_PlayReverse(ANIM_MODEL* model, float speed)
{
    AnimPlayback_PlayReverse(model, speed);
}

void AnimModel_SetSpeed(ANIM_MODEL* model, float speed)
{
    AnimPlayback_SetSpeed(model, speed);
}

void AnimModel_SetTime(ANIM_MODEL* model, double time)
{
    AnimPlayback_SetTime(model, time);
}

void AnimModel_BlendTo(ANIM_MODEL* model, const char* clipName, float blendDuration, bool loop)
{
    AnimPlayback_BlendTo(model, clipName, blendDuration, loop);
}

void AnimModel_BlendToByIndex(ANIM_MODEL* model, int clipIndex, float blendDuration, bool loop)
{
    AnimPlayback_BlendToByIndex(model, clipIndex, blendDuration, loop);
}

void AnimModel_Update(ANIM_MODEL* model, float deltaTime)
{
    AnimPlayback_Update(model, deltaTime);
}

// 描画
void AnimModel_Draw(ANIM_MODEL* model, const DirectX::XMMATRIX& mtxWorld)
{
    if (!model) return;

    ID3D11DeviceContext* pContext = Direct3D_GetContext();

    ShaderSkinned_Begin();
    ShaderSkinned_SetWorldMatrix(mtxWorld);
    ShaderSkinned_SetBoneMatrices(model->boneMatrices, static_cast<int>(model->bones.size()));

    // ※ Topology / DepthStencilState は ShaderSkinned_Begin() で設定済み

    ID3D11ShaderResourceView* pWhiteSRV = AnimLoader_GetWhiteSRV();

    for (unsigned int m = 0; m < model->meshCount; m++)
    {
        if (!model->VertexBuffer[m]) continue;

        unsigned int matIdx = (m < model->meshMaterialIndices.size()) ? model->meshMaterialIndices[m] : 0;
        ID3D11ShaderResourceView* pSRV = nullptr;

        if (matIdx < model->materials.size())
            pSRV = model->materials[matIdx];

        ID3D11ShaderResourceView* textureToBind = pSRV ? pSRV : pWhiteSRV;
        pContext->PSSetShaderResources(0, 1, &textureToBind);
        ShaderSkinned_SetColor({ 1, 1, 1, 1 });

        UINT stride = sizeof(SkinnedVertex);
        UINT offset = 0;
        pContext->IASetVertexBuffers(0, 1, &model->VertexBuffer[m], &stride, &offset);
        pContext->IASetIndexBuffer(model->IndexBuffer[m], DXGI_FORMAT_R32_UINT, 0);

        pContext->DrawIndexed(model->IndexCount[m], 0, 0);
    }
    // ※ DepthStencilState は ShaderSkinned_Begin() で管理。ここでは変更しない
}

void AnimModel_DrawShadow(ANIM_MODEL* model, const DirectX::XMMATRIX& mtxWorld)
{
    if (!model) return;

    ID3D11DeviceContext* pContext = Direct3D_GetContext();
    ShaderShadowMapSkinned_Begin();
    ShaderShadowMapSkinned_SetWorldMatrix(mtxWorld);
    ShaderShadowMapSkinned_SetBoneMatrices(model->boneMatrices, static_cast<int>(model->bones.size()));

    // ※ IASetPrimitiveTopology は ShaderShadowMapSkinned_Begin() で設定済み

    for (unsigned int m = 0; m < model->meshCount; m++)
    {
        if (!model->VertexBuffer[m]) continue;

        UINT stride = sizeof(SkinnedVertex);
        UINT offset = 0;
        pContext->IASetVertexBuffers(0, 1, &model->VertexBuffer[m], &stride, &offset);
        pContext->IASetIndexBuffer(model->IndexBuffer[m], DXGI_FORMAT_R32_UINT, 0);

        pContext->DrawIndexed(model->IndexCount[m], 0, 0);
    }
}

// ゲッター
AABB AnimModel_GetAABB(ANIM_MODEL* model, const DirectX::XMFLOAT3& position)
{
    if (!model) return AABB();
    return {
        { position.x + model->local_aabb.min.x, position.y + model->local_aabb.min.y, position.z + model->local_aabb.min.z },
        { position.x + model->local_aabb.max.x, position.y + model->local_aabb.max.y, position.z + model->local_aabb.max.z }
    };
}

int AnimModel_GetClipCount(ANIM_MODEL* model)
{
    if (!model) return 0;
    return static_cast<int>(model->clips.size());
}

const char* AnimModel_GetClipName(ANIM_MODEL* model, int index)
{
    if (!model || index < 0 || index >= static_cast<int>(model->clips.size())) return "";
    return model->clips[index].name.c_str();
}

double AnimModel_GetCurrentTime(ANIM_MODEL* model)
{
    if (!model) return 0.0;
    return model->currentTime;
}

double AnimModel_GetClipDuration(ANIM_MODEL* model, int clipIndex)
{
    if (!model || clipIndex < 0 || clipIndex >= static_cast<int>(model->clips.size())) return 0.0;
    return model->clips[clipIndex].duration;
}

bool AnimModel_IsPlaying(ANIM_MODEL* model)
{
    if (!model) return false;
    return (model->playState == AnimPlayState::Playing || model->playState == AnimPlayState::PlayingReverse);
}
