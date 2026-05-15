/****************************************
 * @file shader_field.cpp
 * @brief メッシュフィールド描画用シェーダーの実装
 * @author Natsume Shidara
 * @date 2025/09/10
 *
 * CSOファイルまたはDLLリソースからフィールド用シェーダーを読み込み、
 * 入力レイアウト・定数バッファを構築する。
 ****************************************/

#include "shader_field.h"
#include <DirectXMath.h>
#include <d3d11.h>
using namespace DirectX;
#include <fstream>
#include "debug_ostream.h"
#include "direct3d.h"
#include "sampler.h"

#ifdef USE_ASSET_DLL
#include "asset_dll.h"
#include "resource.h"
#endif


static ID3D11VertexShader* g_pVertexShader = nullptr;
static ID3D11InputLayout* g_pInputLayout = nullptr;
static ID3D11PixelShader* g_pPixelShader = nullptr;

// 定数バッファー
static ID3D11Buffer* g_pVSConstantBuffer0 = nullptr; // Projection Matrix

static ID3D11Buffer* g_pPSConstantBuffer0 = nullptr; // Color (XMFLOAT4)

// 注意！初期化で外部から設定されるもの。Release不要。
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

bool ShaderField_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
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
    if (!AssetDLL_GetData(IDR_SHADER_VS_FIELD, &vsData, &vsSize) || !vsData)
    {
        MessageBox(nullptr, "頂点シェーダーの読み込みに失敗しました (DLL)\n\nshader_vertex_field.cso", "エラー", MB_OK);
        return false;
    }
#else
    // ── ファイルから読み込み（従来方式） ──
    {
        std::ifstream ifs_vs("assets/shader/shader_vertex_field.cso", std::ios::binary);
        if (!ifs_vs)
        {
            MessageBox(nullptr, "頂点シェーダーの読み込みに失敗しました\n\nshader_vertex_field.cso", "エラー", MB_OK);
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
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    UINT num_elements = ARRAYSIZE(layout); // 配列の要素数を取得

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
    
    buffer_desc.ByteWidth = sizeof(XMFLOAT4); // バッファのサイズ
    g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pPSConstantBuffer0);



    // 事前コンパイル済みピクセルシェーダーの読み込み
    const void* psData = nullptr;
    size_t psSize = 0;
    unsigned char* psAllocated = nullptr;

#ifdef USE_ASSET_DLL
    if (!AssetDLL_GetData(IDR_SHADER_PS_FIELD, &psData, &psSize) || !psData)
    {
        MessageBox(nullptr, "ピクセルシェーダーの読み込みに失敗しました (DLL)\n\nshader_pixel_field.cso", "エラー", MB_OK);
        return false;
    }
#else
    {
        std::ifstream ifs_ps("assets/shader/shader_pixel_field.cso", std::ios::binary);
        if (!ifs_ps)
        {
            MessageBox(nullptr, "ピクセルシェーダーの読み込みに失敗しました\n\nshader_pixel_field.cso", "エラー", MB_OK);
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

    delete[] psAllocated; // ファイル読み込み時のみ解放（DLL時はnullptrなので安全）

    if (FAILED(hr))
    {
        hal::dout << "Shader_Initialize() : ピクセルシェーダーの作成に失敗しました" << std::endl;
        return false;
    }

    Sampler_SetFilterPoint();
    return true;
}

void ShaderField_Finalize()
{
    SAFE_RELEASE(g_pPixelShader);
    SAFE_RELEASE(g_pVSConstantBuffer0);
    SAFE_RELEASE(g_pPSConstantBuffer0);
    SAFE_RELEASE(g_pInputLayout);
    SAFE_RELEASE(g_pVertexShader);
}


void ShaderField_SetWorldMatrix(const DirectX::XMMATRIX& matrix)
{
    XMFLOAT4X4 transpose;

    XMStoreFloat4x4(&transpose, XMMatrixTranspose(matrix));
    g_pContext->UpdateSubresource(g_pVSConstantBuffer0, 0, nullptr, &transpose, 0, 0);
}


void ShaderField_SetColor(const XMFLOAT4& color)
{
    if (!g_pContext || !g_pPSConstantBuffer0)
        return;

    // g_pPSConstantBuffer0 はカラー用 -- コメントを整合
    g_pContext->UpdateSubresource(g_pPSConstantBuffer0, 0, nullptr, &color, 0, 0);
}

void ShaderField_Begin()
{
    // 頂点シェーダーとピクセルシェーダーを描画パイプラインに設定
    g_pContext->VSSetShader(g_pVertexShader, nullptr, 0);
    g_pContext->PSSetShader(g_pPixelShader, nullptr, 0);

    // 頂点レイアウトを描画パイプラインに設定
    g_pContext->IASetInputLayout(g_pInputLayout);

    // 定数バッファを描画パイプラインに設定
    g_pContext->VSSetConstantBuffers(0, 1, &g_pVSConstantBuffer0);
    g_pContext->PSSetConstantBuffers(0, 1, &g_pPSConstantBuffer0);

    Sampler_SetFilterPoint();
}
