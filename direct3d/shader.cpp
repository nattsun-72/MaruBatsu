/****************************************
 * @file shader.cpp
 * @brief 2Dシェーダー管理モジュール
 * @author Natsume Shidara
 * @date 2025/06/06
 *
 * 2D描画用の頂点シェーダー・ピクセルシェーダーの読み込み、
 * 入力レイアウト・定数バッファの作成と管理を行う。
 ****************************************/

#include <d3d11.h>
#include <DirectXMath.h>

using namespace DirectX;
#include "direct3d.h"
#include "sampler.h"
#include "debug_ostream.h"
#include <fstream>

namespace {
    constexpr UINT MAX_ANISOTROPY_2D = 8; // 2Dシェーダー用異方性フィルタリング最大サンプル数
}

#ifdef USE_ASSET_DLL
#include "asset_dll.h"
#include "resource.h"
#endif


static ID3D11VertexShader* g_pVertexShader = nullptr;
static ID3D11InputLayout* g_pInputLayout = nullptr;
static ID3D11PixelShader* g_pPixelShader = nullptr;

// 定数バッファー
static ID3D11Buffer* g_pVSConstantBuffer0 = nullptr; // Projection Matrix
static ID3D11Buffer* g_pVSConstantBuffer1 = nullptr; // World Matrix

// 注意！初期化で外部から設定されるもの。Release不要。
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;


/**
 * @brief 2Dシェーダー初期化
 * @param pDevice  D3D11デバイス
 * @param pContext D3D11デバイスコンテキスト
 * @return 成功時true
 *
 * 事前コンパイル済みシェーダー(CSO)を読み込み、
 * 頂点シェーダー・ピクセルシェーダー・入力レイアウト・定数バッファを作成する。
 */
bool Shader_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    HRESULT hr; // 戻り値格納用

    // デバイスとデバイスコンテキストのチェック
    if (!pDevice || !pContext)
    {
        hal::dout << "Shader_Initialize() : 与えられたデバイスかコンテキストが不正です" << std::endl;
        return false;
    }

    // デバイスとデバイスコンテキストの保存
    g_pDevice = pDevice;
    g_pContext = pContext;


    // 事前コンパイル済み頂点シェーダーの読み込み
    const void* vsData = nullptr;
    size_t vsSize = 0;
    unsigned char* vsAllocated = nullptr; // ファイル読み込み時のみ使用

#ifdef USE_ASSET_DLL
    // ── DLLからメモリ取得 ──
    if (!AssetDLL_GetData(IDR_SHADER_VS_2D, &vsData, &vsSize) || !vsData)
    {
        MessageBox(nullptr, "頂点シェーダーの読み込みに失敗しました (DLL)\n\nshader_vertex_2d.cso", "エラー", MB_OK);
        return false;
    }
#else
    // ── ファイルから読み込み（従来方式） ──
    {
        std::ifstream ifs_vs("assets/shader/shader_vertex_2d.cso", std::ios::binary);
        if (!ifs_vs)
        {
            MessageBox(nullptr, "頂点シェーダーの読み込みに失敗しました\n\nshader_vertex_2d.cso", "エラー", MB_OK);
            return false;
        }
        ifs_vs.seekg(0, std::ios::end);
        vsSize = static_cast<size_t>(ifs_vs.tellg());
        ifs_vs.seekg(0, std::ios::beg);
        vsAllocated = new unsigned char[vsSize];
        ifs_vs.read((char*)vsAllocated, vsSize);
        ifs_vs.close();
        vsData = vsAllocated;
    }
#endif

    // 頂点シェーダーの作成（vsData/vsSizeはDLL/ファイルどちらでも同じ形式）
    hr = g_pDevice->CreateVertexShader(vsData, vsSize, nullptr, &g_pVertexShader);

    if (FAILED(hr))
    {
        hal::dout << "Shader_Initialize() : 頂点シェーダーの作成に失敗しました" << std::endl;
        delete[] vsAllocated;
        return false;
    }


    // 頂点レイアウトの定義
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    UINT num_elements = ARRAYSIZE(layout);

    // 頂点レイアウトの作成
    hr = g_pDevice->CreateInputLayout(layout, num_elements, vsData, vsSize, &g_pInputLayout);

    delete[] vsAllocated; // ファイル読み込み時のみ解放（DLL時はnullptrなので安全）
    vsAllocated = nullptr;

    if (FAILED(hr))
    {
        hal::dout << "Shader_Initialize() : 頂点レイアウトの作成に失敗しました" << std::endl;
        return false;
    }


    // 頂点シェーダー用定数バッファの作成
    D3D11_BUFFER_DESC buffer_desc{};
    buffer_desc.ByteWidth = sizeof(XMFLOAT4X4); // バッファのサイズ
    buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER; // バインドフラグ

    g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pVSConstantBuffer0);
    g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pVSConstantBuffer1);


    // 事前コンパイル済みピクセルシェーダーの読み込み
    const void* psData = nullptr;
    size_t psSize = 0;
    unsigned char* psAllocated = nullptr;

#ifdef USE_ASSET_DLL
    if (!AssetDLL_GetData(IDR_SHADER_PS_2D, &psData, &psSize) || !psData)
    {
        MessageBox(nullptr, "ピクセルシェーダーの読み込みに失敗しました (DLL)\n\nshader_pixel_2d.cso", "エラー", MB_OK);
        return false;
    }
#else
    {
        std::ifstream ifs_ps("assets/shader/shader_pixel_2d.cso", std::ios::binary);
        if (!ifs_ps)
        {
            MessageBox(nullptr, "ピクセルシェーダーの読み込みに失敗しました\n\nshader_pixel_2d.cso", "エラー", MB_OK);
            return false;
        }
        ifs_ps.seekg(0, std::ios::end);
        psSize = static_cast<size_t>(ifs_ps.tellg());
        ifs_ps.seekg(0, std::ios::beg);
        psAllocated = new unsigned char[psSize];
        ifs_ps.read((char*)psAllocated, psSize);
        ifs_ps.close();
        psData = psAllocated;
    }
#endif

    // ピクセルシェーダーの作成
    hr = g_pDevice->CreatePixelShader(psData, psSize, nullptr, &g_pPixelShader);

    delete[] psAllocated;

    if (FAILED(hr))
    {
        hal::dout << "Shader_Initialize() : ピクセルシェーダーの作成に失敗しました" << std::endl;
        return false;
    }
    
    // サンプラーステート設定
    D3D11_SAMPLER_DESC sampler_desc{};

    // フィルタリング
    sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;

    // UV参照外の取り扱い（UVアドレッシングモード）
    sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.MipLODBias = 0;
    sampler_desc.MaxAnisotropy = MAX_ANISOTROPY_2D;
    sampler_desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    sampler_desc.MinLOD = 0;
    sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;

    return true;
}

/** @brief 2Dシェーダー終了処理（全リソース解放） */
void Shader_Finalize()
{
    SAFE_RELEASE(g_pPixelShader);
    SAFE_RELEASE(g_pVSConstantBuffer0);
    SAFE_RELEASE(g_pVSConstantBuffer1);
    SAFE_RELEASE(g_pInputLayout);
    SAFE_RELEASE(g_pVertexShader);
}

/** @brief 投影行列を転置して定数バッファに設定 */
void Shader_SetProjectionMatrix(const DirectX::XMMATRIX& matrix)
{
    // 定数バッファ格納用行列の構造体を定義
    XMFLOAT4X4 transpose;

    // 行列を転置して定数バッファ格納用行列に変換
    XMStoreFloat4x4(&transpose, XMMatrixTranspose(matrix));

    // 定数バッファに行列をセット
    g_pContext->UpdateSubresource(g_pVSConstantBuffer0, 0, nullptr, &transpose, 0, 0);
}

/** @brief ワールド行列を転置して定数バッファに設定 */
void Shader_SetWorldMatrix(const DirectX::XMMATRIX& matrix)
{
    XMFLOAT4X4 transpose;

    XMStoreFloat4x4(&transpose, XMMatrixTranspose(matrix));

    g_pContext->UpdateSubresource(g_pVSConstantBuffer1, 0, nullptr, &transpose, 0, 0);
}

/** @brief 2Dシェーダーを描画パイプラインにバインド */
void Shader_Begin()
{
    // 頂点シェーダーとピクセルシェーダーを描画パイプラインに設定
    g_pContext->VSSetShader(g_pVertexShader, nullptr, 0);
    g_pContext->PSSetShader(g_pPixelShader, nullptr, 0);

    // 頂点レイアウトを描画パイプラインに設定
    g_pContext->IASetInputLayout(g_pInputLayout);

    // 定数バッファを描画パイプラインに設定
    g_pContext->VSSetConstantBuffers(0, 1, &g_pVSConstantBuffer0);
    g_pContext->VSSetConstantBuffers(1, 1, &g_pVSConstantBuffer1);

    // サンプラーステートを描画パイプラインに設定
    Sampler_SetFilterPoint();
}
