/****************************************
 * @file texture.cpp
 * @brief テクスチャ管理モジュール
 * @author Natsume Shidara
 * @date 2025/06/06
 *
 * WICTextureLoaderを使用したテクスチャの読み込みと
 * 管理番号ベースのテクスチャ管理を行う。
 * ハッシュマップによるO(1)重複チェック対応。
 ****************************************/


#include "texture.h"
#include <string>
#include <unordered_map>
#include "WICTextureLoader11.h"
#include "direct3d.h"

using namespace DirectX;


static constexpr int TEXTURE_MAX = 8192; // テクスチャ管理最大数

struct Texture
{
    std::wstring filename;
    unsigned int width;
    unsigned int height;
    ID3D11Resource* pTexture = nullptr;
    ID3D11ShaderResourceView* pTextureView = nullptr;
};

static Texture g_Textures[TEXTURE_MAX]{};

// ファイル名→インデックスのハッシュマップ（O(1)検索）
static std::unordered_map<std::wstring, int> g_TextureMap;

// 次に空いているスロットのヒント（線形探索の開始位置）
static int g_NextFreeSlot = 0;

static int g_SetTextureIndex = -1;


// 注意！初期化で外部から設定されるもの。Release不要。
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

/**
 * @brief テクスチャ管理の初期化
 * @param pDevice  D3D11デバイス
 * @param pContext D3D11デバイスコンテキスト
 */
void Texture_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    for (Texture& t : g_Textures)
    {
        t.pTexture = nullptr;
    }

    g_TextureMap.clear();
    g_NextFreeSlot = 0;
    g_SetTextureIndex = -1;

    g_pDevice = pDevice;
    g_pContext = pContext;
}

/** @brief テクスチャ管理の終了処理 */
void Texture_Finalize(void) { Texture_AllRelease(); }

/**
 * @brief テクスチャをファイルから読み込み
 * @param pFilename ファイルパス
 * @return 管理番号（失敗時-1）
 *
 * 同一ファイルの重複読み込みはハッシュマップで防止する。
 */
int Texture_Load(const wchar_t* pFilename)
{
    // ハッシュマップで O(1) 重複チェック
    std::wstring key(pFilename);
    auto it = g_TextureMap.find(key);
    if (it != g_TextureMap.end())
    {
        return it->second;
    }

    // 空いている管理領域を探す（ヒント位置から開始）
    for (int count = 0; count < TEXTURE_MAX; count++)
    {
        int i = (g_NextFreeSlot + count) % TEXTURE_MAX;

        if (g_Textures[i].pTexture)
            continue; // 使用中

        // テクスチャの読み込み
        HRESULT hr;

        hr = CreateWICTextureFromFile(g_pDevice, g_pContext, pFilename, &g_Textures[i].pTexture, &g_Textures[i].pTextureView);

        if (FAILED(hr))
        {
            std::wstring errorMsg = L"テクスチャの読み込みに失敗しました\n指定されたパス:";
            errorMsg += pFilename;
            MessageBoxW(nullptr, errorMsg.c_str(), pFilename, MB_OK | MB_ICONERROR);

            return -1;
        }

        ID3D11Texture2D* pTexture = (ID3D11Texture2D*)g_Textures[i].pTexture;
        D3D11_TEXTURE2D_DESC t2desc;
        pTexture->GetDesc(&t2desc);
        g_Textures[i].width = t2desc.Width;
        g_Textures[i].height = t2desc.Height;

        g_Textures[i].filename = key;

        // ハッシュマップに登録
        g_TextureMap[key] = i;
        g_NextFreeSlot = (i + 1) % TEXTURE_MAX;
        return i;
    }

    return -1;
}

/**
 * @brief メモリ上のデータからテクスチャを読み込み（DLL埋め込み用）
 * @param pData     テクスチャデータの先頭ポインタ
 * @param dataSize  データサイズ（バイト）
 * @param pDebugName 重複チェック用の識別名
 * @return 管理番号（失敗時-1）
 */
int Texture_LoadFromMemory(const void* pData, size_t dataSize, const wchar_t* pDebugName)
{
    // ハッシュマップで O(1) 重複チェック
    std::wstring key(pDebugName);
    auto it = g_TextureMap.find(key);
    if (it != g_TextureMap.end())
    {
        return it->second;
    }

    // 空いている管理領域を探す
    for (int count = 0; count < TEXTURE_MAX; count++)
    {
        int i = (g_NextFreeSlot + count) % TEXTURE_MAX;

        if (g_Textures[i].pTexture)
            continue;

        // メモリからテクスチャを読み込み（CreateWICTextureFromMemory を使用）
        HRESULT hr = CreateWICTextureFromMemory(
            g_pDevice, g_pContext,
            static_cast<const uint8_t*>(pData), dataSize,
            &g_Textures[i].pTexture, &g_Textures[i].pTextureView);

        if (FAILED(hr))
        {
            std::wstring errorMsg = L"テクスチャの読み込みに失敗しました (メモリ)\n名前:";
            errorMsg += pDebugName;
            MessageBoxW(nullptr, errorMsg.c_str(), pDebugName, MB_OK | MB_ICONERROR);
            return -1;
        }

        ID3D11Texture2D* pTexture2D = (ID3D11Texture2D*)g_Textures[i].pTexture;
        D3D11_TEXTURE2D_DESC t2desc;
        pTexture2D->GetDesc(&t2desc);
        g_Textures[i].width = t2desc.Width;
        g_Textures[i].height = t2desc.Height;

        g_Textures[i].filename = key;
        g_TextureMap[key] = i;
        g_NextFreeSlot = (i + 1) % TEXTURE_MAX;
        return i;
    }

    return -1;
}

/** @brief 全テクスチャリソースを解放しスロットを初期化 */
void Texture_AllRelease()
{
    for (Texture& t : g_Textures)
    {
        t.filename.clear();
        SAFE_RELEASE(t.pTextureView);
        SAFE_RELEASE(t.pTexture);
    }
    g_TextureMap.clear();
    g_NextFreeSlot = 0;
}

/** @brief 指定テクスチャをピクセルシェーダーの指定スロットにバインド */
void Texture_SetTexture(int texid, int slot)
{
    if (texid < 0) return;

    //g_SetTextureIndex = texid;

    // テクスチャ設定
    g_pContext->PSSetShaderResources(slot, 1, &g_Textures[texid].pTextureView);
}

/** @brief テクスチャの幅を取得（無効なIDの場合は0） */
int Texture_Width(int texid)
{
    if (texid < 0)
        return 0;

    return g_Textures[texid].width;
}

/** @brief テクスチャの高さを取得（無効なIDの場合は0） */
int Texture_Height(int texid)
{
    if (texid < 0)
        return 0;

    return g_Textures[texid].height;
}
