/****************************************
 * @file shader_skinned.cpp
 * @brief スキニングモデル描画用シェーダーの実装
 * @author Natsume Shidara
 * @date 2025/12/10
 *
 * CSOファイルまたはDLLリソースからスキニング用シェーダーを読み込み、
 * ボーン行列・シャドウマップ対応の定数バッファを構築する。
 ****************************************/

#include "shader_skinned.h"
#include "direct3d.h"
#include "sampler.h"
#include "debug_ostream.h"
#include "animation_model.h"

#include <DirectXMath.h>
#include <d3d11.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace DirectX;

#ifdef USE_ASSET_DLL
#include "asset_dll.h"
#include "resource.h"
#endif

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(x) if(x){ x->Release(); x = nullptr; }
#endif

static ID3D11VertexShader* g_pVertexShader = nullptr;
static ID3D11InputLayout* g_pInputLayout = nullptr;
static ID3D11PixelShader* g_pPixelShader = nullptr;

static ID3D11Buffer* g_pVSCB_World = nullptr;
static ID3D11Buffer* g_pVSCB_Shadow = nullptr;
static ID3D11Buffer* g_pVSCB_Bones = nullptr;
static ID3D11Buffer* g_pPSCB_Color = nullptr;

static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

static void LogHR(const char* place, HRESULT hr)
{
    std::ostringstream ss;
    ss << "[ShaderSkinned] " << place << " failed hr=0x" << std::hex << std::uppercase << hr;
    hal::dout << ss.str() << std::endl;
}

static std::vector<char> LoadFileToBlob(const std::string& path)
{
    std::vector<char> blob;
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) return blob;
    ifs.seekg(0, std::ios::end);
    std::streamoff soff = ifs.tellg();
    if (soff <= 0) return blob;
    size_t filesize = static_cast<size_t>(soff);
    ifs.seekg(0, std::ios::beg);
    blob.resize(filesize);
    ifs.read(blob.data(), static_cast<std::streamsize>(filesize));
    if (!ifs) { blob.clear(); return blob; }
    return blob;
}

static bool CreateCB(ID3D11Device* device, UINT byteWidth, ID3D11Buffer** outBuffer)
{
    if (!device || !outBuffer) return false;
    D3D11_BUFFER_DESC desc{};
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.ByteWidth = ((byteWidth + 15) / 16) * 16;
    HRESULT hr = device->CreateBuffer(&desc, nullptr, outBuffer);
    if (FAILED(hr)) { LogHR("CreateCB", hr); *outBuffer = nullptr; return false; }
    return true;
}

bool ShaderSkinned_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    hal::dout << "ShaderSkinned_Initialize: begin" << std::endl;
    if (!pDevice || !pContext) return false;

    g_pDevice = pDevice;
    g_pContext = pContext;
    HRESULT hr = S_OK;

    {
        const void* vsData = nullptr;
        size_t vsSize = 0;
#ifdef USE_ASSET_DLL
        if (!AssetDLL_GetData(IDR_SHADER_VS_SKINNED, &vsData, &vsSize) || !vsData)
        {
            MessageBoxA(nullptr, "shader_vertex_skinned.cso load failed (DLL)", "Error", MB_OK);
            ShaderSkinned_Finalize();
            return false;
        }
#else
        const std::string vsPath = "assets/shader/shader_vertex_skinned.cso";
        std::vector<char> vsBlob = LoadFileToBlob(vsPath);
        if (vsBlob.empty())
        {
            MessageBoxA(nullptr, "shader_vertex_skinned.cso load failed", "Error", MB_OK);
            ShaderSkinned_Finalize();
            return false;
        }
        vsData = vsBlob.data();
        vsSize = vsBlob.size();
#endif

        hr = g_pDevice->CreateVertexShader(vsData, vsSize, nullptr, &g_pVertexShader);
        if (FAILED(hr))
        {
            LogHR("CreateVertexShader", hr);
            ShaderSkinned_Finalize();
            return false;
        }

        D3D11_INPUT_ELEMENT_DESC layout[] = {
            { "POSITION",    0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NORMAL",      0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR",       0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "BLENDINDICES",0, DXGI_FORMAT_R32G32B32A32_UINT,  0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "BLENDWEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        hr = g_pDevice->CreateInputLayout(layout, ARRAYSIZE(layout), vsData, vsSize, &g_pInputLayout);
        if (FAILED(hr))
        {
            LogHR("CreateInputLayout", hr);
            ShaderSkinned_Finalize();
            return false;
        }
    }

    if (!CreateCB(g_pDevice, sizeof(XMFLOAT4X4), &g_pVSCB_World))
    { ShaderSkinned_Finalize(); return false; }

    if (!CreateCB(g_pDevice, sizeof(XMFLOAT4X4), &g_pVSCB_Shadow))
    { ShaderSkinned_Finalize(); return false; }

    if (!CreateCB(g_pDevice, sizeof(XMFLOAT4X4) * MAX_BONES, &g_pVSCB_Bones))
    { ShaderSkinned_Finalize(); return false; }

    if (!CreateCB(g_pDevice, sizeof(XMFLOAT4), &g_pPSCB_Color))
    { ShaderSkinned_Finalize(); return false; }

    {
        const void* psData = nullptr;
        size_t psSize = 0;
#ifdef USE_ASSET_DLL
        if (!AssetDLL_GetData(IDR_SHADER_PS_SKINNED, &psData, &psSize) || !psData)
        {
            MessageBoxA(nullptr, "shader_pixel_skinned.cso load failed (DLL)", "Error", MB_OK);
            ShaderSkinned_Finalize();
            return false;
        }
#else
        const std::string psPath = "assets/shader/shader_pixel_skinned.cso";
        std::vector<char> psBlob = LoadFileToBlob(psPath);
        if (psBlob.empty())
        {
            MessageBoxA(nullptr, "shader_pixel_skinned.cso load failed", "Error", MB_OK);
            ShaderSkinned_Finalize();
            return false;
        }
        psData = psBlob.data();
        psSize = psBlob.size();
#endif

        hr = g_pDevice->CreatePixelShader(psData, psSize, nullptr, &g_pPixelShader);
        if (FAILED(hr))
        {
            LogHR("CreatePixelShader", hr);
            ShaderSkinned_Finalize();
            return false;
        }
    }

    Sampler_SetFilterPoint();

    hal::dout << "ShaderSkinned_Initialize: success" << std::endl;
    return true;
}

void ShaderSkinned_Finalize()
{
    SAFE_RELEASE(g_pPixelShader);
    SAFE_RELEASE(g_pVSCB_World);
    SAFE_RELEASE(g_pVSCB_Shadow);
    SAFE_RELEASE(g_pVSCB_Bones);
    SAFE_RELEASE(g_pPSCB_Color);
    SAFE_RELEASE(g_pInputLayout);
    SAFE_RELEASE(g_pVertexShader);
    g_pDevice = nullptr;
    g_pContext = nullptr;
}

void ShaderSkinned_SetWorldMatrix(const DirectX::XMMATRIX& matrix)
{
    if (!g_pContext || !g_pVSCB_World) return;
    XMFLOAT4X4 transpose;
    XMStoreFloat4x4(&transpose, XMMatrixTranspose(matrix));
    g_pContext->UpdateSubresource(g_pVSCB_World, 0, nullptr, &transpose, 0, 0);
}

/**
 * @brief ボーン行列を定数バッファに転送する
 * @param bones ボーン行列配列（MAX_BONES要素分が確保済みであること）
 * @param boneCount 実使用ボーン数（現在は未使用：定数バッファはUpdateSubresourceで全体転送のため）
 * @note 部分更新が必要な場合は D3D11_USAGE_DYNAMIC + Map/Unmap に切り替えること
 */
void ShaderSkinned_SetBoneMatrices(const DirectX::XMFLOAT4X4* bones, int boneCount)
{
    (void)boneCount;  // 定数バッファは全体転送（UpdateSubresourceはBOX指定不可）
    if (!g_pContext || !g_pVSCB_Bones || !bones) return;
    g_pContext->UpdateSubresource(g_pVSCB_Bones, 0, nullptr, bones, 0, 0);
}

void ShaderSkinned_SetColor(const XMFLOAT4& color)
{
    if (!g_pContext || !g_pPSCB_Color) return;
    g_pContext->UpdateSubresource(g_pPSCB_Color, 0, nullptr, &color, 0, 0);
}

void ShaderSkinned_SetLightViewProjection(const DirectX::XMFLOAT4X4& matrix)
{
    if (!g_pContext || !g_pVSCB_Shadow) return;
    XMMATRIX m = XMLoadFloat4x4(&matrix);
    XMFLOAT4X4 transpose;
    XMStoreFloat4x4(&transpose, XMMatrixTranspose(m));
    g_pContext->UpdateSubresource(g_pVSCB_Shadow, 0, nullptr, &transpose, 0, 0);
}

void ShaderSkinned_SetShadowMap(ID3D11ShaderResourceView* pShadowSRV)
{
    if (!g_pContext) return;
    g_pContext->PSSetShaderResources(1, 1, &pShadowSRV);
}

void ShaderSkinned_Begin()
{
    if (!g_pContext || !g_pVertexShader) return;

    g_pContext->VSSetShader(g_pVertexShader, nullptr, 0);
    g_pContext->PSSetShader(g_pPixelShader, nullptr, 0);
    g_pContext->IASetInputLayout(g_pInputLayout);

    g_pContext->VSSetConstantBuffers(0, 1, &g_pVSCB_World);
    g_pContext->VSSetConstantBuffers(3, 1, &g_pVSCB_Shadow);
    g_pContext->VSSetConstantBuffers(4, 1, &g_pVSCB_Bones);

    g_pContext->PSSetConstantBuffers(0, 1, &g_pPSCB_Color);

    // トポロジー・深度テスト設定（各モデルで毎回呼ぶのではなくBeginで1回）
    g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    Direct3D_DepthStencilStateDepthIsEnable(true);

    Sampler_SetFilterPoint();
}
