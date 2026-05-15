/****************************************
 * @file model.cpp
 * @brief FBX/OBJなどの3Dモデル読み込み・描画管理クラス
 * - Assimpによるモデルロード
 * - リソースキャッシュ機能（参照カウント）
 * - メッシュ切断用CPUデータ保持
 * @author Natsume Shidara
 * @update 2025/12/12
 ****************************************/

#include "model.h"
#include "WICTextureLoader11.h"
#include "direct3d.h"
#include "texture.h"
#include "shader3d.h"
#include "shader3d_unlit.h"
#include "shader_shadow_map.h"
#include "debug_ostream.h"
#include "debug_renderer.h"

#include <DirectXMath.h>
#include <vector>
#include <string>
#include <filesystem>
#include <algorithm>
#include <cassert>
#include <unordered_map>
#include <cstdio>

#ifdef USE_ASSET_DLL
#include "asset_dll.h"
#include "resource.h"

//--------------------------------------
// FBX外部テクスチャ ファイル名 → DLLリソースID マッピング
// モデルが参照するPBRテクスチャをDLLから取得するために使用
//--------------------------------------
static int GetExternalTextureResourceId(const std::string& filename)
{
    // LongSword PBRテクスチャ
    if (filename == "DefaultMaterial_BaseColor.jpg")    return IDR_TEX_SWORD_BASECOLOR;
    if (filename == "DefaultMaterial_Normal.jpg")       return IDR_TEX_SWORD_NORMAL;
    if (filename == "DefaultMaterial_Metallic.jpg")     return IDR_TEX_SWORD_METALLIC;
    if (filename == "DefaultMaterial_Roughness.jpg")    return IDR_TEX_SWORD_ROUGHNESS;
    if (filename == "DefaultMaterial_Emission.jpg")     return IDR_TEX_SWORD_EMISSION;
    if (filename == "DefaultMaterial_Alpha.jpg")        return IDR_TEX_SWORD_ALPHA;
    if (filename == "DefaultMaterial_Displacement.jpg") return IDR_TEX_SWORD_DISPLACEMENT;
    if (filename == "\xe3\x83\xad\xe3\x83\xb3\xe3\x82\xb0\xe3\x82\xbd\xe3\x83\xbc\xe3\x83\x89\xe3\x83\x99\xe3\x82\xa4\xe3\x82\xaf.jpg") return IDR_TEX_SWORD_BAKE; // ロングソードベイク.jpg

    // OptionBooster PBRテクスチャ
    if (filename == "OP_B_BaseColor.png")       return IDR_TEX_BOOSTER_BASECOLOR;
    if (filename == "OP_B_Normal.png")          return IDR_TEX_BOOSTER_NORMAL;
    if (filename == "OP_B_Metallic.png")        return IDR_TEX_BOOSTER_METALLIC;
    if (filename == "OP_B_Roughness.png")       return IDR_TEX_BOOSTER_ROUGHNESS;
    if (filename == "OP_B_Emission.png")        return IDR_TEX_BOOSTER_EMISSION;
    if (filename == "OP_B_Alpha.png")           return IDR_TEX_BOOSTER_ALPHA;
    if (filename == "OP_B_Displacement.png")    return IDR_TEX_BOOSTER_DISPLACEMENT;
    if (filename == "OP-B_bake.jpg")            return IDR_TEX_BOOSTER_BAKE;

    // 敵スライステクスチャ
    if (filename == "enemy_sl.png")             return IDR_TEX_ENEMY_SL;
    if (filename == "enemy_sl_hl.png")          return IDR_TEX_ENEMY_SL_HL;
    if (filename == "drone_danmen.png")         return IDR_TEX_DRONE_DANMEN;

    // 汎用
    if (filename == "white.png")                return IDR_TEX_WHITE;

    return -1; // 未登録
}
#endif

using namespace DirectX;

namespace
{
    // テクスチャデータの1ピクセルあたりのバイト数（RGBA）
    constexpr size_t BYTES_PER_PIXEL = 4;

    // 断面テクスチャ用ピクセルシェーダーSRVスロット
    constexpr UINT PS_SRV_SLOT_SLICE = 2;
}

//--------------------------------------
// 内部変数・定数
//--------------------------------------
static int g_TextureWhite = -1; // Texture管理側のID
static ID3D11ShaderResourceView* g_pWhiteSRV = nullptr; // 内部フォールバック用SRV

// モデルキャッシュ（ファイルパス -> モデルポインタ）
static std::unordered_map<std::string, MODEL*> g_ModelCache;

//--------------------------------------
// テクスチャタイプ優先順位リスト
// FBXエクスポーターによってディフューズテクスチャが
// 異なるスロットに格納される場合があるため、
// 複数のタイプをフォールバック検索する
//--------------------------------------
static const aiTextureType TEXTURE_TYPE_PRIORITY[] = {
    aiTextureType_DIFFUSE,      // 標準ディフューズ
    aiTextureType_BASE_COLOR,   // PBR BaseColor (glTF/FBX)
    aiTextureType_UNKNOWN,      // FBX PBR (Substance Painter等)
    aiTextureType_AMBIENT,      // 一部エクスポーターのフォールバック
};
static constexpr int TEXTURE_TYPE_PRIORITY_COUNT = sizeof(TEXTURE_TYPE_PRIORITY) / sizeof(TEXTURE_TYPE_PRIORITY[0]);

// 初期化・終了処理
void Model_Initialize()
{
    if (g_TextureWhite != -1) return; // 二重初期化防止

#ifdef USE_ASSET_DLL
    {
        const void* pData = nullptr;
        size_t dataSize = 0;
        if (AssetDLL_GetData(IDR_TEX_WHITE, &pData, &dataSize))
        {
            g_TextureWhite = Texture_LoadFromMemory(pData, dataSize, L"model_white");
            CreateWICTextureFromMemory(Direct3D_GetDevice(),
                static_cast<const uint8_t*>(pData), dataSize, nullptr, &g_pWhiteSRV);
        }
    }
#else
    g_TextureWhite = Texture_Load(L"assets/white.png");
    CreateWICTextureFromFile(Direct3D_GetDevice(), L"assets/white.png", nullptr, &g_pWhiteSRV);
#endif
}

void Model_Finalize()
{
    if (g_pWhiteSRV)
    {
        g_pWhiteSRV->Release();
        g_pWhiteSRV = nullptr;
    }
    g_TextureWhite = -1;
    g_ModelCache.clear();
}

//--------------------------------------
// デバッグ用マテリアル情報ダンプ
//--------------------------------------
#if defined(DEBUG) || defined(_DEBUG)
static void DumpMaterialInfo(const aiScene* scene, const char* modelName)
{
    char buf[512];
    sprintf_s(buf, "=== Material Dump: %s (%u materials) ===\n", modelName, scene->mNumMaterials);
    OutputDebugStringA(buf);

    for (unsigned int m = 0; m < scene->mNumMaterials; m++)
    {
        aiMaterial* mat = scene->mMaterials[m];
        aiString name;
        mat->Get(AI_MATKEY_NAME, name);

        sprintf_s(buf, "  [%u] %s\n", m, name.C_Str());
        OutputDebugStringA(buf);

        // 全テクスチャタイプをスキャン
        for (int t = aiTextureType_NONE; t <= aiTextureType_UNKNOWN; t++)
        {
            unsigned int count = mat->GetTextureCount(static_cast<aiTextureType>(t));
            if (count > 0)
            {
                aiString path;
                mat->GetTexture(static_cast<aiTextureType>(t), 0, &path);
                sprintf_s(buf, "    texType=%d count=%u path=%s\n", t, count, path.C_Str());
                OutputDebugStringA(buf);
            }
        }

        // カラープロパティ
        aiColor3D diffuse(1, 1, 1);
        if (mat->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse) == AI_SUCCESS)
        {
            sprintf_s(buf, "    diffuse=(%.2f, %.2f, %.2f)\n", diffuse.r, diffuse.g, diffuse.b);
            OutputDebugStringA(buf);
        }

        float opacity = 1.0f;
        if (mat->Get(AI_MATKEY_OPACITY, opacity) == AI_SUCCESS)
        {
            sprintf_s(buf, "    opacity=%.2f\n", opacity);
            OutputDebugStringA(buf);
        }

        aiColor3D specular(0, 0, 0);
        if (mat->Get(AI_MATKEY_COLOR_SPECULAR, specular) == AI_SUCCESS)
        {
            sprintf_s(buf, "    specular=(%.2f, %.2f, %.2f)\n", specular.r, specular.g, specular.b);
            OutputDebugStringA(buf);
        }

        aiColor3D emissive(0, 0, 0);
        if (mat->Get(AI_MATKEY_COLOR_EMISSIVE, emissive) == AI_SUCCESS)
        {
            sprintf_s(buf, "    emissive=(%.2f, %.2f, %.2f)\n", emissive.r, emissive.g, emissive.b);
            OutputDebugStringA(buf);
        }
    }

    OutputDebugStringA("=== Material Dump End ===\n");
}
#endif

#ifdef USE_ASSET_DLL
// ファイルパス → リソースID マッピング（モデル）
static int GetModelResourceId(const char* path)
{
    std::string p(path);
    // 正規化: バックスラッシュをスラッシュに
    std::replace(p.begin(), p.end(), '\\', '/');

    if (p.find("BOX.fbx") != std::string::npos) return IDR_MODEL_BOX;
    if (p.find("bullet.fbx") != std::string::npos) return IDR_MODEL_BULLET;
    if (p.find("robot.fbx") != std::string::npos) return IDR_MODEL_ROBOT;
    if (p.find("sky.fbx") != std::string::npos) return IDR_MODEL_SKY;
    if (p.find("LongSword.fbx") != std::string::npos || p.find("LongSword/LongSword.fbx") != std::string::npos) return IDR_MODEL_LONGSWORD;
    if (p.find("option_booster") != std::string::npos) return IDR_MODEL_OPTION_BOOSTER;
    return -1;
}

// スライステクスチャパス → リソースID マッピング
static int GetSliceTextureResourceId(const wchar_t* path)
{
    std::wstring p(path);
    if (p.find(L"enemy_sl_hl") != std::wstring::npos) return IDR_TEX_ENEMY_SL_HL;
    if (p.find(L"enemy_sl") != std::wstring::npos) return IDR_TEX_ENEMY_SL;
    if (p.find(L"drone_danmen") != std::wstring::npos) return IDR_TEX_DRONE_DANMEN;
    if (p.find(L"wall") != std::wstring::npos) return IDR_TEX_WALL;
    return -1;
}
#endif

// モデル読み込み関数
MODEL* ModelLoad(const char* FileName, float scale, bool RHFlg, const wchar_t* pSlisedTextureFilename, const wchar_t* pSliceVerticalFilename)
{
    
    // キャッシュキーを作成（シンプルにファイルパスとする）
    std::string key = FileName;

    // 1. キャッシュ確認
    auto it = g_ModelCache.find(key);
    if (it != g_ModelCache.end())
    {
        // ヒットしたら参照カウントを増やして既存のポインタを返す
        ModelAddRef(it->second);
        return it->second;
    }

    // 2. 新規ロード
    MODEL* model = new MODEL;
    model->resourceKey = key; // キーを記憶
    model->refCount = 1;      // 初期カウント


#ifdef USE_ASSET_DLL
    if (pSlisedTextureFilename != nullptr) {
        int sliceResId = GetSliceTextureResourceId(pSlisedTextureFilename);
        const void* pSliceData = nullptr;
        size_t sliceDataSize = 0;
        if (sliceResId >= 0 && AssetDLL_GetData(sliceResId, &pSliceData, &sliceDataSize))
        {
            model->SetSlicedTexturId(Texture_LoadFromMemory(pSliceData, sliceDataSize, pSlisedTextureFilename));
            ID3D11Resource* pTex = nullptr;
            CreateWICTextureFromMemory(Direct3D_GetDevice(), Direct3D_GetContext(),
                static_cast<const uint8_t*>(pSliceData), sliceDataSize, &pTex, &model->pSliceTextureSRV);
            SAFE_RELEASE(pTex);
        }
    }
    else {
        model->SetSlicedTexturId(-1);
        model->pSliceTextureSRV = nullptr;
    }

    if (pSliceVerticalFilename != nullptr) {
        int sliceVResId = GetSliceTextureResourceId(pSliceVerticalFilename);
        const void* pSliceVData = nullptr;
        size_t sliceVDataSize = 0;
        if (sliceVResId >= 0 && AssetDLL_GetData(sliceVResId, &pSliceVData, &sliceVDataSize))
        {
            model->SetSliceVerticalTexturId(Texture_LoadFromMemory(pSliceVData, sliceVDataSize, pSliceVerticalFilename));
            ID3D11Resource* pTex = nullptr;
            CreateWICTextureFromMemory(Direct3D_GetDevice(), Direct3D_GetContext(),
                static_cast<const uint8_t*>(pSliceVData), sliceVDataSize, &pTex, &model->pSliceVerticalSRV);
            SAFE_RELEASE(pTex);
        }
    }
    else {
        model->SetSliceVerticalTexturId(-1);
        model->pSliceVerticalSRV = nullptr;
    }
#else
    if (pSlisedTextureFilename != nullptr) {
        model->SetSlicedTexturId(Texture_Load(pSlisedTextureFilename));
        ID3D11Resource* pTex = nullptr;
        HRESULT hr = CreateWICTextureFromFile(Direct3D_GetDevice(), pSlisedTextureFilename, &pTex, &model->pSliceTextureSRV);
        if (SUCCEEDED(hr)) {
            SAFE_RELEASE(pTex);
        }
    }
    else {
        model->SetSlicedTexturId(-1);
        model->pSliceTextureSRV = nullptr;
    }

    if (pSliceVerticalFilename != nullptr) {
        model->SetSliceVerticalTexturId(Texture_Load(pSliceVerticalFilename));
        ID3D11Resource* pTex = nullptr;
        HRESULT hr2 = CreateWICTextureFromFile(Direct3D_GetDevice(), pSliceVerticalFilename, &pTex, &model->pSliceVerticalSRV);
        if (SUCCEEDED(hr2)) {
            SAFE_RELEASE(pTex);
        }
    }
    else {
        model->SetSliceVerticalTexturId(-1);
        model->pSliceVerticalSRV = nullptr;
    }
#endif

    // パス情報の構築
    std::filesystem::path fbxPath(FileName);
    std::filesystem::path directoryPath = fbxPath.parent_path();
    std::string directory = directoryPath.string();
    std::filesystem::path texDirectory = directoryPath / "TEX";

    // Assimpインポート設定
    unsigned int importFlags = aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_GenNormals |
        aiProcess_CalcTangentSpace |
        aiProcess_GenUVCoords |
        aiProcess_ImproveCacheLocality |
        aiProcess_OptimizeMeshes |
        aiProcess_ConvertToLeftHanded |
        aiProcess_SortByPType;

#ifdef USE_ASSET_DLL
    {
        int modelResId = GetModelResourceId(FileName);
        const void* pModelData = nullptr;
        size_t modelDataSize = 0;
        if (modelResId >= 0 && AssetDLL_GetData(modelResId, &pModelData, &modelDataSize))
        {
            // ファイル拡張子のヒントを渡す（Assimpがフォーマット判別に使用）
            model->AiScene = aiImportFileFromMemory(
                static_cast<const char*>(pModelData), static_cast<unsigned int>(modelDataSize),
                importFlags, ".fbx");
        }
    }
#else
    model->AiScene = aiImportFile(FileName, importFlags);
#endif
    if (!model->AiScene)
    {
        std::string msg = "ModelLoad Error: Failed to load file: ";
        msg += FileName;
        msg += "\n";
        OutputDebugStringA(msg.c_str());
        MessageBoxA(nullptr, msg.c_str(), "ModelLoad Error", MB_OK | MB_ICONERROR);
        delete model;
        return nullptr;
    }

    // メッシュデータの構築
    model->meshCount = model->AiScene->mNumMeshes;
    model->VertexBuffer = new ID3D11Buffer * [model->meshCount];
    model->IndexBuffer = new ID3D11Buffer * [model->meshCount];
    model->Meshes.resize(model->meshCount);

    for (unsigned int m = 0; m < model->meshCount; m++)
    {
        aiMesh* mesh = model->AiScene->mMeshes[m];
        MeshData& meshData = model->Meshes[m];
        meshData.materialIndex = mesh->mMaterialIndex;

        // 頂点データ抽出
        meshData.vertices.resize(mesh->mNumVertices);
        for (unsigned int v = 0; v < mesh->mNumVertices; v++)
        {
            Vertex& vert = meshData.vertices[v];
            if (RHFlg)
            {
                vert.position = XMFLOAT3(mesh->mVertices[v].x * scale, -mesh->mVertices[v].z * scale, mesh->mVertices[v].y * scale);
                vert.normal = XMFLOAT3(mesh->mNormals[v].x, -mesh->mNormals[v].z, mesh->mNormals[v].y);
            }
            else
            {
                vert.position = XMFLOAT3(mesh->mVertices[v].x * scale, mesh->mVertices[v].y * scale, mesh->mVertices[v].z * scale);
                vert.normal = XMFLOAT3(mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z);
            }

            if (mesh->mTextureCoords[0])
                vert.uv = XMFLOAT2(mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y);
            else
                vert.uv = XMFLOAT2(0.0f, 0.0f);

            if (mesh->HasVertexColors(0))
                vert.color = XMFLOAT4(mesh->mColors[0][v].r, mesh->mColors[0][v].g, mesh->mColors[0][v].b, mesh->mColors[0][v].a);
            else
                vert.color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

            // AABB更新
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

        // インデックスデータ
        for (unsigned int f = 0; f < mesh->mNumFaces; f++)
        {
            const aiFace* face = &mesh->mFaces[f];
            if (face->mNumIndices == 3)
            {
                meshData.indices.push_back(face->mIndices[0]);
                meshData.indices.push_back(face->mIndices[1]);
                meshData.indices.push_back(face->mIndices[2]);
            }
        }

        // GPUバッファ
        D3D11_BUFFER_DESC bd = {};
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.ByteWidth = sizeof(Vertex) * static_cast<UINT>(meshData.vertices.size());
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA sd = {};
        sd.pSysMem = meshData.vertices.data();
        Direct3D_GetDevice()->CreateBuffer(&bd, &sd, &model->VertexBuffer[m]);

        bd.ByteWidth = sizeof(unsigned int) * static_cast<UINT>(meshData.indices.size());
        bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
        sd.pSysMem = meshData.indices.data();
        Direct3D_GetDevice()->CreateBuffer(&bd, &sd, &model->IndexBuffer[m]);
    }

    // デバッグ用マテリアル情報ダンプ
#if defined(DEBUG) || defined(_DEBUG)
    DumpMaterialInfo(model->AiScene, FileName);
#endif

    // テクスチャ読み込み（Model_Initialize で初期化済みのはず）
    // 万が一未初期化の場合のフォールバック
    if (g_TextureWhite == -1)
    {
        Model_Initialize();
    }

    model->materials.resize(model->AiScene->mNumMaterials, nullptr);
    model->materialColors.resize(model->AiScene->mNumMaterials, { 1.0f, 1.0f, 1.0f, 1.0f });

    for (unsigned int m = 0; m < model->AiScene->mNumMaterials; m++)
    {
        aiMaterial* pMat = model->AiScene->mMaterials[m];

        //--- マテリアルカラー抽出 ---
        aiColor3D diffuseColor(1.0f, 1.0f, 1.0f);
        if (pMat->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor) == AI_SUCCESS)
        {
            float opacity = 1.0f;
            pMat->Get(AI_MATKEY_OPACITY, opacity);
            model->materialColors[m] = { diffuseColor.r, diffuseColor.g, diffuseColor.b, opacity };
        }

        //--- テクスチャパス検索（優先順位付きフォールバック） ---
        aiString texPath;
        bool foundTexType = false;

        for (int ti = 0; ti < TEXTURE_TYPE_PRIORITY_COUNT; ti++)
        {
            if (pMat->GetTexture(TEXTURE_TYPE_PRIORITY[ti], 0, &texPath) == AI_SUCCESS && texPath.length > 0)
            {
                foundTexType = true;
                break;
            }
        }

        if (!foundTexType || texPath.length == 0)
        {
#if defined(DEBUG) || defined(_DEBUG)
            aiString matName;
            pMat->Get(AI_MATKEY_NAME, matName);
            char dbg[256];
            sprintf_s(dbg, "  mat[%u] \"%s\": no texture found in any type, using material color only\n", m, matName.C_Str());
            OutputDebugStringA(dbg);
#endif
            model->materials[m] = g_pWhiteSRV;
            if (g_pWhiteSRV) g_pWhiteSRV->AddRef();
            continue;
        }

        //--- 埋め込みテクスチャ ---
        const aiTexture* pEmbeddedTex = model->AiScene->GetEmbeddedTexture(texPath.C_Str());
        if (pEmbeddedTex)
        {
            ID3D11Resource* pTex = nullptr;
            ID3D11ShaderResourceView* pSRV = nullptr;
            size_t size = (pEmbeddedTex->mHeight == 0)
                ? pEmbeddedTex->mWidth
                : pEmbeddedTex->mWidth * pEmbeddedTex->mHeight * BYTES_PER_PIXEL;
            const uint8_t* pData = reinterpret_cast<const uint8_t*>(pEmbeddedTex->pcData);

            // WICで試行（PNG, JPG, BMP等）
            HRESULT hr = CreateWICTextureFromMemory(Direct3D_GetDevice(), pData, size, &pTex, &pSRV);

            // WIC失敗 + DDSヘッダー検出時はログ出力（DirectXTexライブラリ未リンクのためDDSは未対応）
            if (FAILED(hr) && size >= 4 && memcmp(pData, "DDS ", 4) == 0)
            {
#if defined(DEBUG) || defined(_DEBUG)
                char dbg[256];
                sprintf_s(dbg, "  mat[%u] embedded DDS texture detected but not supported (need DirectXTex.lib): %s\n", m, texPath.C_Str());
                OutputDebugStringA(dbg);
#endif
            }

            if (SUCCEEDED(hr) && pSRV)
            {
                SAFE_RELEASE(pTex);
                model->materials[m] = pSRV;
                std::string texKey = texPath.C_Str();
                if (model->Texture.find(texKey) == model->Texture.end())
                {
                    model->Texture[texKey] = pSRV;
                    pSRV->AddRef();
                }
            }
            else
            {
#if defined(DEBUG) || defined(_DEBUG)
                char dbg[256];
                sprintf_s(dbg, "  mat[%u] embedded texture decode failed (hr=0x%08X): %s\n", m, static_cast<unsigned int>(hr), texPath.C_Str());
                OutputDebugStringA(dbg);
#endif
                model->materials[m] = g_pWhiteSRV;
                if (g_pWhiteSRV) g_pWhiteSRV->AddRef();
            }
            continue;
        }

        //--- 外部ファイル読み込み ---
        std::string filenameOnly = std::filesystem::path(texPath.C_Str()).filename().string();
        if (model->Texture.count(filenameOnly))
        {
            model->materials[m] = model->Texture[filenameOnly];
            model->materials[m]->AddRef();
            continue;
        }

#ifdef USE_ASSET_DLL
        // DLLリソースからテクスチャを取得
        {
            int texResId = GetExternalTextureResourceId(filenameOnly);
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
                        model->materials[m] = pSRV;
                        pSRV->AddRef();
                    }
                    else
                    {
                        model->materials[m] = g_pWhiteSRV;
                        if (g_pWhiteSRV) g_pWhiteSRV->AddRef();
                    }
                }
                else
                {
                    model->materials[m] = g_pWhiteSRV;
                    if (g_pWhiteSRV) g_pWhiteSRV->AddRef();
                }
            }
            else
            {
#if defined(DEBUG) || defined(_DEBUG)
                char dbg[512];
                sprintf_s(dbg, "  mat[%u] no DLL resource mapping for: \"%s\"\n", m, filenameOnly.c_str());
                OutputDebugStringA(dbg);
#endif
                model->materials[m] = g_pWhiteSRV;
                if (g_pWhiteSRV) g_pWhiteSRV->AddRef();
            }
        }
#else
        std::filesystem::path targetPath;
        bool found = false;

        // 検索パス1: TEXサブディレクトリ
        targetPath = texDirectory / filenameOnly;
        if (std::filesystem::exists(targetPath)) found = true;

        // 検索パス2: モデルと同じディレクトリ
        if (!found)
        {
            targetPath = std::filesystem::path(directory) / filenameOnly;
            if (std::filesystem::exists(targetPath)) found = true;
        }
        // 検索パス3: Assimpが返した相対パス
        if (!found)
        {
            targetPath = std::filesystem::path(directory) / texPath.C_Str();
            if (std::filesystem::exists(targetPath)) found = true;
        }
        // 検索パス4: 絶対パスとして
        if (!found)
        {
            targetPath = texPath.C_Str();
            if (std::filesystem::exists(targetPath)) found = true;
        }
        // 検索パス5: モデルディレクトリ配下を再帰検索
        if (!found && std::filesystem::exists(directoryPath))
        {
            try
            {
                for (const auto& entry : std::filesystem::recursive_directory_iterator(directoryPath))
                {
                    if (entry.is_regular_file() && entry.path().filename().string() == filenameOnly)
                    {
                        targetPath = entry.path();
                        found = true;
                        break;
                    }
                }
            }
            catch (const std::filesystem::filesystem_error&)
            {
                // ディレクトリアクセスエラーは無視
            }
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
                model->materials[m] = pSRV;
                pSRV->AddRef();
            }
            else
            {
#if defined(DEBUG) || defined(_DEBUG)
                char dbg[512];
                sprintf_s(dbg, "  mat[%u] WIC load failed (hr=0x%08X): %ls\n", m, static_cast<unsigned int>(hr), targetPath.c_str());
                OutputDebugStringA(dbg);
#endif
                model->materials[m] = g_pWhiteSRV;
                if (g_pWhiteSRV) g_pWhiteSRV->AddRef();
            }
        }
        else
        {
#if defined(DEBUG) || defined(_DEBUG)
            char dbg[512];
            sprintf_s(dbg, "  mat[%u] texture file not found: \"%s\" (searched: TEX/, model dir, relative, absolute, recursive)\n", m, texPath.C_Str());
            OutputDebugStringA(dbg);
#endif
            model->materials[m] = g_pWhiteSRV;
            if (g_pWhiteSRV) g_pWhiteSRV->AddRef();
        }
#endif
    }

    // 最後にキャッシュへ登録
    g_ModelCache[key] = model;

    return model;
}

// データからモデル生成 (切断結果用)
MODEL* ModelCreateFromData(std::vector<MeshData> meshes, MODEL* original)
{
    MODEL* model = new MODEL;
    model->AiScene = nullptr;

    // 切断されたモデルは独自形状なのでリソースキーは持たない
    model->resourceKey = "";
    model->refCount = 1;

    // マテリアル・テクスチャリソースを継承
    model->materials = original->materials;
    model->materialColors = original->materialColors;

    if (original->pSliceTextureSRV)
    {
        model->materials.push_back(original->pSliceTextureSRV);

        // 新しいモデルにも断面情報を引き継ぐ（再切断のため）
        model->pSliceTextureSRV = original->pSliceTextureSRV;
        model->sliceTextureId = original->sliceTextureId; // IDも引き継ぐ
    }
    else
    {
        model->pSliceTextureSRV = nullptr;
        model->sliceTextureId = -1;
    }

    // 縦切りテクスチャも引き継ぐ
    model->pSliceVerticalSRV = original->pSliceVerticalSRV;
    model->sliceVerticalTextureId = original->sliceVerticalTextureId;

    for (auto* srv : model->materials)
    {
        if (srv) srv->AddRef();
    }

    model->Texture = original->Texture;
    for (auto& pair : model->Texture)
    {
        if (pair.second) pair.second->AddRef();
    }

    model->meshCount = static_cast<unsigned int>(meshes.size());
    model->Meshes = std::move(meshes); // std::move で大量の頂点・インデックスデータのコピーを回避
    model->VertexBuffer = new ID3D11Buffer * [model->meshCount];
    model->IndexBuffer = new ID3D11Buffer * [model->meshCount];

    for (unsigned int m = 0; m < model->meshCount; m++)
    {
        const MeshData& data = model->Meshes[m];

        if (data.vertices.empty())
        {
            model->VertexBuffer[m] = nullptr;
            model->IndexBuffer[m] = nullptr;
            continue;
        }

        D3D11_BUFFER_DESC bd = {};
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.ByteWidth = sizeof(Vertex) * static_cast<UINT>(data.vertices.size());
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA sd = {};
        sd.pSysMem = data.vertices.data();
        Direct3D_GetDevice()->CreateBuffer(&bd, &sd, &model->VertexBuffer[m]);

        bd.ByteWidth = sizeof(unsigned int) * static_cast<UINT>(data.indices.size());
        bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
        sd.pSysMem = data.indices.data();
        Direct3D_GetDevice()->CreateBuffer(&bd, &sd, &model->IndexBuffer[m]);

        for (size_t i = 0; i < data.vertices.size(); i++)
        {
            const XMFLOAT3& p = data.vertices[i].position;
            if (m == 0 && i == 0)
            {
                model->local_aabb.min = p;
                model->local_aabb.max = p;
            }
            else
            {
                model->local_aabb.min.x = std::fminf(model->local_aabb.min.x, p.x);
                model->local_aabb.min.y = std::fminf(model->local_aabb.min.y, p.y);
                model->local_aabb.min.z = std::fminf(model->local_aabb.min.z, p.z);
                model->local_aabb.max.x = std::fmaxf(model->local_aabb.max.x, p.x);
                model->local_aabb.max.y = std::fmaxf(model->local_aabb.max.y, p.y);
                model->local_aabb.max.z = std::fmaxf(model->local_aabb.max.z, p.z);
            }
        }
    }

    return model;
}

// 参照カウント増加
void ModelAddRef(MODEL* model)
{
    if (model)
    {
        model->refCount++;
    }
}

// モデル解放関数 (参照カウント式)
void ModelRelease(MODEL* model)
{
    if (!model) return;

    // 1. 参照カウントを減らす
    model->refCount--;

    // 2. まだ誰かが使っているなら、ここで終了（削除しない）
    if (model->refCount > 0)
    {
        return;
    }



    // キャッシュ登録済みなら、マップから削除
    if (!model->resourceKey.empty())
    {
        g_ModelCache.erase(model->resourceKey);
    }

    // GPUバッファ解放
    if (model->VertexBuffer)
    {
        for (unsigned int m = 0; m < model->meshCount; m++)
        {
            SAFE_RELEASE(model->VertexBuffer[m]);
        }
        delete[] model->VertexBuffer;
    }

    if (model->IndexBuffer)
    {
        for (unsigned int m = 0; m < model->meshCount; m++)
        {
            SAFE_RELEASE(model->IndexBuffer[m]);
        }
        delete[] model->IndexBuffer;
    }

    model->pSliceTextureSRV = nullptr;

    // マテリアルSRV解放
    for (auto* srv : model->materials)
    {
        SAFE_RELEASE(srv);
    }
    model->materials.clear();

    // テクスチャマップ解放
    for (auto& tex : model->Texture)
    {
        SAFE_RELEASE(tex.second);
    }
    model->Texture.clear();

    // Assimpシーン破棄
    if (model->AiScene)
    {
        aiReleaseImport(model->AiScene);
        model->AiScene = nullptr;
    }

    delete model;
}

// モデル描画関数
void ModelDraw(MODEL* model, const DirectX::XMMATRIX& mtxWorld)
{
    if (!model) return;

    ID3D11DeviceContext* pContext = Direct3D_GetContext();

    // ※ Shader3D_Begin() は呼び出し元（scene.cpp等）で1回だけ呼ぶ前提
    // ここでは毎回呼ばず、ワールド行列のみ更新
    Shader3D_SetWorldMatrix(mtxWorld);

    for (unsigned int m = 0; m < model->meshCount; m++)
    {
        if (!model->VertexBuffer[m]) continue;

        unsigned int matIdx = model->Meshes[m].materialIndex;
        ID3D11ShaderResourceView* pSRV = nullptr;

        if (matIdx < model->materials.size())
        {
            pSRV = model->materials[matIdx];
        }

        ID3D11ShaderResourceView* textureToBind = pSRV ? pSRV : g_pWhiteSRV;
        pContext->PSSetShaderResources(0, 1, &textureToBind);

        // 断面メッシュのブレンド設定
        float blendRatio = model->Meshes[m].sliceBlendRatio;
        if (blendRatio >= 0.0f)
        {
            // 縦切りテクスチャをt2にバインド（未設定時は横切りで代用）
            ID3D11ShaderResourceView* pVertSRV = model->GetEffectiveVerticalSRV();
            ID3D11ShaderResourceView* vertToBind = pVertSRV ? pVertSRV : textureToBind;
            pContext->PSSetShaderResources(PS_SRV_SLOT_SLICE, 1, &vertToBind);
            Shader3D_SetSliceBlend(blendRatio);
        }
        else
        {
            // 通常メッシュ：ブレンド無効
            Shader3D_SetSliceBlend(-1.0f);
        }

        // マテリアルカラーを適用（テクスチャなしモデルでも正しい色で表示）
        XMFLOAT4 matColor = { 1.0f, 1.0f, 1.0f, 1.0f };
        if (matIdx < model->materialColors.size())
        {
            matColor = model->materialColors[matIdx];
        }
        Shader3D_SetColor(matColor);

        UINT stride = sizeof(Vertex);
        UINT offset = 0;
        pContext->IASetVertexBuffers(0, 1, &model->VertexBuffer[m], &stride, &offset);
        pContext->IASetIndexBuffer(model->IndexBuffer[m], DXGI_FORMAT_R32_UINT, 0);

        UINT indexCount = static_cast<UINT>(model->Meshes[m].indices.size());
        pContext->DrawIndexed(indexCount, 0, 0);
    }

    // ※ DepthStencilState は Shader3D_Begin() で設定済み。ここでは変更しない
}

// モデル描画関数 (Unlit)
void ModelUnlitDraw(MODEL* model, const DirectX::XMMATRIX& mtxWorld)
{
    if (!model) return;

    ID3D11DeviceContext* pContext = Direct3D_GetContext();

    Shader3D_Unlit_Begin();
    Shader3D_Unlit_SetWorldMatrix(mtxWorld);

    pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    Direct3D_DepthStencilStateDepthIsEnable(true);

    for (unsigned int m = 0; m < model->meshCount; m++)
    {
        if (!model->VertexBuffer[m]) continue;

        unsigned int matIdx = model->Meshes[m].materialIndex;

        XMFLOAT4 color = { 1, 1, 1, 1 };
        if (model->AiScene && matIdx < model->AiScene->mNumMaterials)
        {
            aiColor3D diffuse(1, 1, 1);
            if (model->AiScene->mMaterials[matIdx]->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse) == AI_SUCCESS)
            {
                color = { diffuse.r, diffuse.g, diffuse.b, 1.0f };
            }
        }
        Shader3D_Unlit_SetColor(color);

        ID3D11ShaderResourceView* pSRV = nullptr;
        if (matIdx < model->materials.size())
        {
            pSRV = model->materials[matIdx];
        }
        ID3D11ShaderResourceView* textureToBind = pSRV ? pSRV : g_pWhiteSRV;
        pContext->PSSetShaderResources(0, 1, &textureToBind);

        UINT stride = sizeof(Vertex);
        UINT offset = 0;
        pContext->IASetVertexBuffers(0, 1, &model->VertexBuffer[m], &stride, &offset);
        pContext->IASetIndexBuffer(model->IndexBuffer[m], DXGI_FORMAT_R32_UINT, 0);

        UINT indexCount = static_cast<UINT>(model->Meshes[m].indices.size());
        pContext->DrawIndexed(indexCount, 0, 0);
    }
    // ※ DepthStencilState はメインパスの Shader3D_Begin() で再設定されるため、ここでは変更しない
}

// シャドウマップ描画
void ModelDrawShadow(MODEL* model, const DirectX::XMMATRIX& mtxWorld)
{
    if (!model) return;

    ID3D11DeviceContext* pContext = Direct3D_GetContext();
    ShaderShadowMap_SetWorldMatrix(mtxWorld);

    // ※ IASetPrimitiveTopology は ShaderShadowMap_Begin() で設定済み

    for (unsigned int m = 0; m < model->meshCount; m++)
    {
        if (!model->VertexBuffer[m]) continue;

        UINT stride = sizeof(Vertex);
        UINT offset = 0;
        pContext->IASetVertexBuffers(0, 1, &model->VertexBuffer[m], &stride, &offset);
        pContext->IASetIndexBuffer(model->IndexBuffer[m], DXGI_FORMAT_R32_UINT, 0);

        UINT indexCount = static_cast<UINT>(model->Meshes[m].indices.size());
        pContext->DrawIndexed(indexCount, 0, 0);
    }
}

// AABB取得
AABB Model_GetAABB(MODEL* model, const DirectX::XMFLOAT3& position)
{
    if (!model) return AABB();
    return {
        { position.x + model->local_aabb.min.x, position.y + model->local_aabb.min.y, position.z + model->local_aabb.min.z },
        { position.x + model->local_aabb.max.x, position.y + model->local_aabb.max.y, position.z + model->local_aabb.max.z }
    };
}

void ModelDrawDebug(MODEL* model, const DirectX::XMFLOAT3& position)
{
    if (!model) return;

    // 現在の計算式に基づいたAABBを取得
    AABB aabb = Model_GetAABB(model, position);

    // 黄色で描画
    DirectX::XMFLOAT4 color = { 1.0f, 1.0f, 0.0f, 1.0f };
    DebugRenderer::DrawAABB(aabb, color);
}