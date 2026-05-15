/****************************************
 * @file anim_loader.cpp
 * @brief FBXモデル読み込み（メッシュ・マテリアル・テクスチャ・クリップ抽出）
 * @author Natsume Shidara
 * @date 2025/06/06
 *
 * - Assimpによるメッシュ読み込み・頂点バッファ作成
 * - マテリアル・テクスチャ検索・読み込み
 * - アニメーションクリップ抽出
 ****************************************/

#include "anim_loader.h"
#include "anim_bone.h"
#include "WICTextureLoader11.h"
#include "direct3d.h"
#include "texture.h"

#include <DirectXMath.h>
#include <vector>
#include <string>
#include <filesystem>
#include <algorithm>
#include <cmath>

#ifdef USE_ASSET_DLL
#include "asset_dll.h"
#include "resource.h"

//--------------------------------------
// FBX外部テクスチャ ファイル名 → DLLリソースID マッピング
//--------------------------------------
static int GetAnimTextureResourceId(const std::string& filename)
{
    if (filename == "white.png")  return IDR_TEX_WHITE;
    // robot.fbx はテクスチャ埋め込みのため、通常ここには来ない
    return -1;
}
#endif

using namespace DirectX;

namespace {
    constexpr double DEFAULT_TICKS_PER_SECOND = 24.0;  // デフォルトのアニメーション速度（ticks/秒）
    constexpr int BYTES_PER_PIXEL = 4;                  // テクスチャのピクセルあたりバイト数（RGBA）
    constexpr float WEIGHT_NORMALIZE_EPSILON = 0.0001f; // ウェイト正規化の許容誤差
    constexpr int TRIANGLE_VERTEX_COUNT = 3;            // 三角形の頂点数
}

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(x) if(x){ x->Release(); x = nullptr; }
#endif

// 白テクスチャ（共有リソース）
static int g_TextureWhite_Anim = -1;
static ID3D11ShaderResourceView* g_pWhiteSRV_Anim = nullptr;

ID3D11ShaderResourceView* AnimLoader_GetWhiteSRV()
{
    return g_pWhiteSRV_Anim;
}

void AnimLoader_InitWhiteTexture()
{
    if (g_TextureWhite_Anim == -1)
    {
#ifdef USE_ASSET_DLL
        const void* pData = nullptr;
        size_t dataSize = 0;
        if (AssetDLL_GetData(IDR_TEX_WHITE, &pData, &dataSize))
        {
            g_TextureWhite_Anim = Texture_LoadFromMemory(pData, dataSize, L"anim_white");
            CreateWICTextureFromMemory(Direct3D_GetDevice(),
                static_cast<const uint8_t*>(pData), dataSize, nullptr, &g_pWhiteSRV_Anim);
        }
#else
        g_TextureWhite_Anim = Texture_Load(L"assets/white.png");
        CreateWICTextureFromFile(Direct3D_GetDevice(), L"assets/white.png", nullptr, &g_pWhiteSRV_Anim);
#endif
    }
}

void AnimLoader_ReleaseWhiteTexture()
{
    SAFE_RELEASE(g_pWhiteSRV_Anim);
    g_TextureWhite_Anim = -1;
}

// アニメーションクリップ抽出
static void ExtractAnimations(ANIM_MODEL* model, const aiScene* scene, const char* sourceTag)
{
    for (unsigned int a = 0; a < scene->mNumAnimations; a++)
    {
        aiAnimation* anim = scene->mAnimations[a];
        AnimationClip clip;
        clip.aiSceneRef = scene;
        clip.name = anim->mName.length > 0 ? anim->mName.C_Str() : (std::string(sourceTag) + "_anim_" + std::to_string(a));
        clip.duration = anim->mDuration;
        clip.ticksPerSecond = (anim->mTicksPerSecond != 0.0) ? anim->mTicksPerSecond : DEFAULT_TICKS_PER_SECOND;
        clip.animIndex = a;

        int clipIdx = static_cast<int>(model->clips.size());
        model->clipNameToIndex[clip.name] = clipIdx;
        model->clips.push_back(clip);
    }
}

// マテリアル・テクスチャ読み込み
static void LoadMaterials(ANIM_MODEL* model, const std::string& directory,
    const std::filesystem::path& texDirectory)
{
    AnimLoader_InitWhiteTexture();

    model->materials.resize(model->AiScene->mNumMaterials, nullptr);

    for (unsigned int mat = 0; mat < model->AiScene->mNumMaterials; mat++)
    {
        aiMaterial* pMat = model->AiScene->mMaterials[mat];
        aiString texPath;

        if (pMat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) != AI_SUCCESS)
            pMat->GetTexture(aiTextureType_BASE_COLOR, 0, &texPath);

        if (texPath.length == 0)
        {
            model->materials[mat] = g_pWhiteSRV_Anim;
            if (g_pWhiteSRV_Anim) g_pWhiteSRV_Anim->AddRef();
            continue;
        }

        // 埋め込みテクスチャ
        const aiTexture* pEmbeddedTex = model->AiScene->GetEmbeddedTexture(texPath.C_Str());
        if (pEmbeddedTex)
        {
            ID3D11Resource* pTex = nullptr;
            ID3D11ShaderResourceView* pSRV = nullptr;
            size_t size = (pEmbeddedTex->mHeight == 0) ?
                pEmbeddedTex->mWidth : pEmbeddedTex->mWidth * pEmbeddedTex->mHeight * BYTES_PER_PIXEL;
            HRESULT hr = CreateWICTextureFromMemory(Direct3D_GetDevice(),
                reinterpret_cast<const uint8_t*>(pEmbeddedTex->pcData), size, &pTex, &pSRV);

            if (SUCCEEDED(hr))
            {
                if (pTex) pTex->Release();
                model->materials[mat] = pSRV;
                std::string texKey = texPath.C_Str();
                if (model->Texture.find(texKey) == model->Texture.end())
                {
                    model->Texture[texKey] = pSRV;
                    pSRV->AddRef();
                }
            }
            else
            {
                model->materials[mat] = g_pWhiteSRV_Anim;
                if (g_pWhiteSRV_Anim) g_pWhiteSRV_Anim->AddRef();
            }
            continue;
        }

        // 外部テクスチャファイル検索
        std::string filenameOnly = std::filesystem::path(texPath.C_Str()).filename().string();
        if (model->Texture.count(filenameOnly))
        {
            model->materials[mat] = model->Texture[filenameOnly];
            model->materials[mat]->AddRef();
            continue;
        }

#ifdef USE_ASSET_DLL
        // DLLリソースからテクスチャを取得
        {
            int texResId = GetAnimTextureResourceId(filenameOnly);
            if (texResId >= 0)
            {
                const void* pTexData = nullptr;
                size_t texDataSize = 0;
                if (AssetDLL_GetData(texResId, &pTexData, &texDataSize))
                {
                    ID3D11Resource* pTex = nullptr;
                    ID3D11ShaderResourceView* pSRV = nullptr;
                    HRESULT hr = CreateWICTextureFromMemory(Direct3D_GetDevice(),
                        static_cast<const uint8_t*>(pTexData), texDataSize, &pTex, &pSRV);

                    if (SUCCEEDED(hr))
                    {
                        if (pTex) pTex->Release();
                        model->Texture[filenameOnly] = pSRV;
                        model->materials[mat] = pSRV;
                        pSRV->AddRef();
                    }
                    else
                    {
                        model->materials[mat] = g_pWhiteSRV_Anim;
                        if (g_pWhiteSRV_Anim) g_pWhiteSRV_Anim->AddRef();
                    }
                }
                else
                {
                    model->materials[mat] = g_pWhiteSRV_Anim;
                    if (g_pWhiteSRV_Anim) g_pWhiteSRV_Anim->AddRef();
                }
            }
            else
            {
                model->materials[mat] = g_pWhiteSRV_Anim;
                if (g_pWhiteSRV_Anim) g_pWhiteSRV_Anim->AddRef();
            }
        }
#else
        std::filesystem::path targetPath;
        bool found = false;

        targetPath = texDirectory / filenameOnly;
        if (std::filesystem::exists(targetPath)) found = true;

        if (!found)
        {
            targetPath = std::filesystem::path(directory) / filenameOnly;
            if (std::filesystem::exists(targetPath)) found = true;
        }
        if (!found)
        {
            targetPath = std::filesystem::path(directory) / texPath.C_Str();
            if (std::filesystem::exists(targetPath)) found = true;
        }
        if (!found)
        {
            targetPath = texPath.C_Str();
            if (std::filesystem::exists(targetPath)) found = true;
        }

        if (found)
        {
            ID3D11Resource* pTex = nullptr;
            ID3D11ShaderResourceView* pSRV = nullptr;
            HRESULT hr = CreateWICTextureFromFile(Direct3D_GetDevice(), targetPath.c_str(), &pTex, &pSRV);

            if (SUCCEEDED(hr))
            {
                if (pTex) pTex->Release();
                model->Texture[filenameOnly] = pSRV;
                model->materials[mat] = pSRV;
                pSRV->AddRef();
            }
            else
            {
                model->materials[mat] = g_pWhiteSRV_Anim;
                if (g_pWhiteSRV_Anim) g_pWhiteSRV_Anim->AddRef();
            }
        }
        else
        {
            model->materials[mat] = g_pWhiteSRV_Anim;
            if (g_pWhiteSRV_Anim) g_pWhiteSRV_Anim->AddRef();
        }
#endif
    }
}

// メッシュ読み込み・頂点バッファ/インデックスバッファ作成
static void LoadMeshes(ANIM_MODEL* model)
{
    model->meshCount = model->AiScene->mNumMeshes;
    model->VertexBuffer = new ID3D11Buffer * [model->meshCount];
    model->IndexBuffer = new ID3D11Buffer * [model->meshCount];
    model->IndexCount = new unsigned int[model->meshCount];

    // マテリアルインデックスをキャッシュ（描画時にAiScene非依存にする）
    model->meshMaterialIndices.resize(model->meshCount);
    for (unsigned int m = 0; m < model->meshCount; m++)
    {
        model->meshMaterialIndices[m] = model->AiScene->mMeshes[m]->mMaterialIndex;
    }

    for (unsigned int m = 0; m < model->meshCount; m++)
    {
        model->VertexBuffer[m] = nullptr;
        model->IndexBuffer[m] = nullptr;
        model->IndexCount[m] = 0;
    }

    for (unsigned int m = 0; m < model->meshCount; m++)
    {
        aiMesh* mesh = model->AiScene->mMeshes[m];

        std::vector<SkinnedVertex> vertices(mesh->mNumVertices);
        for (unsigned int v = 0; v < mesh->mNumVertices; v++)
        {
            SkinnedVertex& vert = vertices[v];
            // 頂点にscaleを直接掛けない。
            // スケールはルートノード行列に合成済みなので、
            // ボーン行列の階層計算を通じて頂点・行列の双方に一貫して適用される。
            vert.position = XMFLOAT3(
                mesh->mVertices[v].x,
                mesh->mVertices[v].y,
                mesh->mVertices[v].z);
            vert.normal = XMFLOAT3(
                mesh->mNormals[v].x,
                mesh->mNormals[v].y,
                mesh->mNormals[v].z);

            if (mesh->mTextureCoords[0])
                vert.uv = XMFLOAT2(mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y);
            else
                vert.uv = XMFLOAT2(0.0f, 0.0f);

            if (mesh->HasVertexColors(0))
                vert.color = XMFLOAT4(mesh->mColors[0][v].r, mesh->mColors[0][v].g,
                    mesh->mColors[0][v].b, mesh->mColors[0][v].a);
            else
                vert.color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

            for (int k = 0; k < MAX_BONE_INFLUENCE; k++)
            {
                vert.boneIndices[k] = 0;
                vert.boneWeights[k] = 0.0f;
            }

            if (v == 0 && m == 0)
            {
                model->local_aabb.min = vert.position;
                model->local_aabb.max = vert.position;
            }
            else
            {
                model->local_aabb.min.x = std::fminf(model->local_aabb.min.x, vert.position.x);
                model->local_aabb.min.y = std::fminf(model->local_aabb.min.y, vert.position.y);
                model->local_aabb.min.z = std::fminf(model->local_aabb.min.z, vert.position.z);
                model->local_aabb.max.x = std::fmaxf(model->local_aabb.max.x, vert.position.x);
                model->local_aabb.max.y = std::fmaxf(model->local_aabb.max.y, vert.position.y);
                model->local_aabb.max.z = std::fmaxf(model->local_aabb.max.z, vert.position.z);
            }
        }

        AnimBone_ExtractBones(model, mesh, vertices);

        // ウェイト正規化
        for (auto& vert : vertices)
        {
            float totalWeight = 0.0f;
            for (int k = 0; k < MAX_BONE_INFLUENCE; k++)
                totalWeight += vert.boneWeights[k];

            if (totalWeight > 0.0f && std::abs(totalWeight - 1.0f) > WEIGHT_NORMALIZE_EPSILON)
            {
                for (int k = 0; k < MAX_BONE_INFLUENCE; k++)
                    vert.boneWeights[k] /= totalWeight;
            }
            else if (totalWeight == 0.0f)
            {
                vert.boneIndices[0] = 0;
                vert.boneWeights[0] = 1.0f;
            }
        }

        // インデックス
        std::vector<unsigned int> indices;
        for (unsigned int f = 0; f < mesh->mNumFaces; f++)
        {
            const aiFace* face = &mesh->mFaces[f];
            if (face->mNumIndices == TRIANGLE_VERTEX_COUNT)
            {
                indices.push_back(face->mIndices[0]);
                indices.push_back(face->mIndices[1]);
                indices.push_back(face->mIndices[2]);
            }
        }
        model->IndexCount[m] = static_cast<unsigned int>(indices.size());

        // GPUバッファ作成
        D3D11_BUFFER_DESC bd = {};
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.ByteWidth = sizeof(SkinnedVertex) * static_cast<UINT>(vertices.size());
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA sd = {};
        sd.pSysMem = vertices.data();
        Direct3D_GetDevice()->CreateBuffer(&bd, &sd, &model->VertexBuffer[m]);

        bd.ByteWidth = sizeof(unsigned int) * static_cast<UINT>(indices.size());
        bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
        sd.pSysMem = indices.data();
        Direct3D_GetDevice()->CreateBuffer(&bd, &sd, &model->IndexBuffer[m]);
    }
}

// モデル読み込みメイン
ANIM_MODEL* AnimLoader_LoadFromFile(const char* fileName, float scale)
{
    ANIM_MODEL* model = new ANIM_MODEL{};
    model->AiScene = nullptr;
    model->VertexBuffer = nullptr;
    model->IndexBuffer = nullptr;
    model->IndexCount = nullptr;
    model->meshCount = 0;
    model->refCount = 1;
    model->playSpeed = 1.0f;
    model->playState = AnimPlayState::Stopped;
    model->currentClipIndex = -1;
    model->currentTime = 0.0;
    model->loop = true;
    model->pausedFromReverse = false;

    model->blend.isBlending = false;
    model->blend.clipIndexA = -1;
    model->blend.clipIndexB = -1;
    model->blend.blendFactor = 0.0f;
    model->blend.blendDuration = 0.0f;
    model->blend.blendTimer = 0.0f;
    model->blend.startTimeA = 0.0;

    for (int i = 0; i < MAX_BONES; i++)
        XMStoreFloat4x4(&model->boneMatrices[i], XMMatrixIdentity());

    std::filesystem::path fbxPath(fileName);
    std::filesystem::path directoryPath = fbxPath.parent_path();
    std::string directory = directoryPath.string();
    std::filesystem::path texDirectory = directoryPath / "TEX";

    unsigned int importFlags = aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_GenNormals |
        aiProcess_CalcTangentSpace |
        aiProcess_GenUVCoords |
        aiProcess_LimitBoneWeights |
        aiProcess_ConvertToLeftHanded |
        aiProcess_SortByPType;

#ifdef USE_ASSET_DLL
    {
        // robot.fbx のリソースIDを取得
        int resId = -1;
        std::string p(fileName);
        if (p.find("robot") != std::string::npos) resId = IDR_MODEL_ROBOT;
        // 必要に応じて他のアニメーションモデルのマッピングを追加

        const void* pModelData = nullptr;
        size_t modelDataSize = 0;
        if (resId >= 0 && AssetDLL_GetData(resId, &pModelData, &modelDataSize))
        {
            model->AiScene = aiImportFileFromMemory(
                static_cast<const char*>(pModelData), static_cast<unsigned int>(modelDataSize),
                importFlags, ".fbx");
        }
    }
#else
    model->AiScene = aiImportFile(fileName, importFlags);
#endif
    if (!model->AiScene)
    {
        std::string msg = "AnimModel_Load Error: ";
        msg += fileName;
        OutputDebugStringA(msg.c_str());
        MessageBoxA(nullptr, msg.c_str(), "AnimModel_Load Error", MB_OK | MB_ICONERROR);
        delete model;
        return nullptr;
    }

    // スケール行列をルートノードに合成する。
    // こうすることで、ボーン階層の計算を通じて頂点座標と行列（平行移動・回転だけでなく）
    // の双方に一貫してスケールが適用される。
    // 頂点に直接 * scale するとoffsetMatrixやアニメーションキーの移動値と不整合が生じる。
    aiMatrix4x4 scaleMat;
    aiMatrix4x4::Scaling(aiVector3D(scale, scale, scale), scaleMat);
    aiMatrix4x4 scaledRoot = model->AiScene->mRootNode->mTransformation * scaleMat;

    // ルート逆行列をロード時に一度だけ計算して保存する。
    // 毎フレーム計算すると、ルートノードにアニメーションが含まれる場合に
    // 基準が動いて不安定になる。
    aiMatrix4x4 rootInv = scaledRoot;
    rootInv.Inverse();
    model->globalInverseTransformAi = rootInv;
    model->globalInverseTransform = AiMatrixToXM(rootInv);

    // ノード階層構築（ルートノードのlocalTransformAiにスケールを合成）
    // BuildNodeHierarchy はノードのmTransformationをそのまま保存するので、
    // ルートノードだけ事前に差し替えておく。
    // NOTE: mTransformationはconstではないため直接書き換え可能。
    // ただしaiSceneの元データを変更するのは望ましくないため、
    // BuildNodeHierarchy後にルートのlocalTransformAiだけ上書きする。
    AnimBone_BuildNodeHierarchy(model, model->AiScene->mRootNode, -1);

    // ルートノード（index 0）のlocalTransformにスケールを合成
    if (!model->nodeHierarchy.empty())
    {
        model->nodeHierarchy[0].localTransformAi = scaledRoot;
        model->nodeHierarchy[0].localTransform = AiMatrixToXM(scaledRoot);
    }

    LoadMeshes(model);

    // AABBは未スケールの頂点座標で計算されたので、scaleを適用
    model->local_aabb.min.x *= scale;
    model->local_aabb.min.y *= scale;
    model->local_aabb.min.z *= scale;
    model->local_aabb.max.x *= scale;
    model->local_aabb.max.y *= scale;
    model->local_aabb.max.z *= scale;

    AnimBone_ResolveBoneParents(model);

    LoadMaterials(model, directory, texDirectory);

    ExtractAnimations(model, model->AiScene, "model");

    return model;
}

// 追加クリップ読み込み
bool AnimLoader_LoadClip(ANIM_MODEL* model, const char* clipFileName, const char* clipName)
{
    if (!model) return false;

    // アニメーションデータのみ必要なため、メッシュ処理フラグは不要。
    // ConvertToLeftHanded はメインモデルと座標系を一致させるために必須。
    unsigned int importFlags = aiProcess_ConvertToLeftHanded;

#ifdef USE_ASSET_DLL
    const aiScene* clipScene = nullptr;
    {
        // クリップファイルもDLLリソースから取得を試みる
        int resId = -1;
        std::string p(clipFileName);
        if (p.find("robot") != std::string::npos) resId = IDR_MODEL_ROBOT;
        // 必要に応じて他のクリップファイルのマッピングを追加

        const void* pClipData = nullptr;
        size_t clipDataSize = 0;
        if (resId >= 0 && AssetDLL_GetData(resId, &pClipData, &clipDataSize))
        {
            clipScene = aiImportFileFromMemory(
                static_cast<const char*>(pClipData), static_cast<unsigned int>(clipDataSize),
                importFlags, ".fbx");
        }
    }
#else
    const aiScene* clipScene = aiImportFile(clipFileName, importFlags);
#endif
    if (!clipScene || clipScene->mNumAnimations == 0)
    {
        std::string msg = "AnimModel_LoadClip Error: ";
        msg += clipFileName;
        OutputDebugStringA(msg.c_str());
        if (clipScene) aiReleaseImport(clipScene);
        return false;
    }

    for (unsigned int a = 0; a < clipScene->mNumAnimations; a++)
    {
        aiAnimation* anim = clipScene->mAnimations[a];
        AnimationClip clip;
        clip.aiSceneRef = clipScene;
        clip.name = (clipName && strlen(clipName) > 0) ?
            std::string(clipName) + (a > 0 ? ("_" + std::to_string(a)) : "") :
            anim->mName.C_Str();
        clip.duration = anim->mDuration;
        clip.ticksPerSecond = (anim->mTicksPerSecond != 0.0) ? anim->mTicksPerSecond : DEFAULT_TICKS_PER_SECOND;
        clip.animIndex = a;

        int clipIdx = static_cast<int>(model->clips.size());
        model->clipNameToIndex[clip.name] = clipIdx;
        model->clips.push_back(clip);
    }

    return true;
}
