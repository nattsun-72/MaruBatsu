/****************************************
 * @file shader_shadow_map_skinned.cpp
 * @brief スキニングモデル用シャドウマップ生成シェーダーの実装
 * @author Natsume Shidara
 * @date 2025/12/10
 *
 * HLSLソースをランタイムコンパイルし、
 * ボーン行列付きシャドウマップ描画パイプラインを構築する。
 ****************************************/

#include "shader_shadow_map_skinned.h"
#include "animation_model.h"
#include "debug_ostream.h"

#ifdef USE_ASSET_DLL
#include "asset_dll.h"
#include "resource.h"
#endif

#include <d3dcompiler.h>
#include <string>
#include <sstream>
#include <vector>

#pragma comment(lib, "d3dcompiler.lib")

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(x) if(x){ x->Release(); x = nullptr; }
#endif

struct CB_ShadowMapSkinned
{
    DirectX::XMFLOAT4X4 World;
    DirectX::XMFLOAT4X4 LightViewProj;
};

static ID3D11VertexShader* g_pVertexShader = nullptr;
static ID3D11PixelShader* g_pPixelShader = nullptr;
static ID3D11InputLayout* g_pInputLayout = nullptr;
static ID3D11Buffer* g_pConstantBuffer = nullptr;
static ID3D11Buffer* g_pBoneBuffer = nullptr;

static CB_ShadowMapSkinned g_CBData = {};

static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

static void LogHR(const char* place, HRESULT hr)
{
    std::ostringstream ss;
    ss << "[ShaderShadowMapSkinned] " << place << " failed hr=0x" << std::hex << std::uppercase << hr;
    hal::dout << ss.str() << std::endl;
}

#ifdef USE_ASSET_DLL
static HRESULT CompileShaderFromMemory(const void* pSrcData, size_t srcDataSize, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
{
    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(DEBUG) || defined(_DEBUG)
    dwShaderFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
    ID3DBlob* pErrorBlob = nullptr;
    HRESULT hr = D3DCompile(pSrcData, srcDataSize, "shadow_map_skinned.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        szEntryPoint, szShaderModel, dwShaderFlags, 0, ppBlobOut, &pErrorBlob);
    if (FAILED(hr))
    {
        if (pErrorBlob)
        {
            hal::dout << reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer()) << std::endl;
            pErrorBlob->Release();
        }
        return hr;
    }
    if (pErrorBlob) pErrorBlob->Release();
    return S_OK;
}
#else
static HRESULT CompileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
{
    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(DEBUG) || defined(_DEBUG)
    dwShaderFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
    ID3DBlob* pErrorBlob = nullptr;
    HRESULT hr = D3DCompileFromFile(szFileName, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        szEntryPoint, szShaderModel, dwShaderFlags, 0, ppBlobOut, &pErrorBlob);
    if (FAILED(hr))
    {
        if (pErrorBlob)
        {
            hal::dout << reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer()) << std::endl;
            pErrorBlob->Release();
        }
        return hr;
    }
    if (pErrorBlob) pErrorBlob->Release();
    return S_OK;
}
#endif

bool ShaderShadowMapSkinned_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    hal::dout << "ShaderShadowMapSkinned_Initialize: begin" << std::endl;
    if (!pDevice || !pContext) return false;

    g_pDevice = pDevice;
    g_pContext = pContext;

    HRESULT hr = S_OK;
    ID3DBlob* pVSBlob = nullptr;
    ID3DBlob* pPSBlob = nullptr;

#ifdef USE_ASSET_DLL
    const void* pHlslData = nullptr;
    size_t hlslSize = 0;
    if (!AssetDLL_GetData(IDR_SHADER_HLSL_SHADOW_MAP_SKINNED, &pHlslData, &hlslSize))
    {
        LogHR("DLL GetData shadow_map_skinned.hlsl", E_FAIL);
        return false;
    }
    hr = CompileShaderFromMemory(pHlslData, hlslSize, "VS_Main", "vs_5_0", &pVSBlob);
#else
    {
        const WCHAR* hlslPath = L"assets/shader/shadow_map_skinned.hlsl";
        hr = CompileShaderFromFile(hlslPath, "VS_Main", "vs_5_0", &pVSBlob);
    }
#endif
    if (FAILED(hr))
    {
        LogHR("Compile VS", hr);
        return false;
    }

    hr = g_pDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &g_pVertexShader);
    if (FAILED(hr))
    {
        LogHR("CreateVertexShader", hr);
        pVSBlob->Release();
        return false;
    }

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION",     0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",       0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",        0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",     0, DXGI_FORMAT_R32G32_FLOAT,       0, 40, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT,  0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BLENDWEIGHT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 64, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    hr = g_pDevice->CreateInputLayout(layout, ARRAYSIZE(layout), pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &g_pInputLayout);
    pVSBlob->Release();
    if (FAILED(hr)) { LogHR("CreateInputLayout", hr); return false; }

#ifdef USE_ASSET_DLL
    hr = CompileShaderFromMemory(pHlslData, hlslSize, "PS_Main", "ps_5_0", &pPSBlob);
#else
    {
        const WCHAR* hlslPath = L"assets/shader/shadow_map_skinned.hlsl";
        hr = CompileShaderFromFile(hlslPath, "PS_Main", "ps_5_0", &pPSBlob);
    }
#endif
    if (FAILED(hr)) { LogHR("Compile PS", hr); return false; }

    hr = g_pDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &g_pPixelShader);
    pPSBlob->Release();
    if (FAILED(hr)) { LogHR("CreatePixelShader", hr); return false; }

    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.ByteWidth = sizeof(CB_ShadowMapSkinned);
    cbDesc.Usage = D3D11_USAGE_DEFAULT;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    hr = g_pDevice->CreateBuffer(&cbDesc, nullptr, &g_pConstantBuffer);
    if (FAILED(hr)) { LogHR("CreateCB", hr); return false; }

    cbDesc.ByteWidth = sizeof(DirectX::XMFLOAT4X4) * MAX_BONES;
    hr = g_pDevice->CreateBuffer(&cbDesc, nullptr, &g_pBoneBuffer);
    if (FAILED(hr)) { LogHR("CreateBoneCB", hr); return false; }

    DirectX::XMStoreFloat4x4(&g_CBData.World, DirectX::XMMatrixIdentity());
    DirectX::XMStoreFloat4x4(&g_CBData.LightViewProj, DirectX::XMMatrixIdentity());

    hal::dout << "ShaderShadowMapSkinned_Initialize: success" << std::endl;
    return true;
}

void ShaderShadowMapSkinned_Finalize()
{
    SAFE_RELEASE(g_pBoneBuffer);
    SAFE_RELEASE(g_pConstantBuffer);
    SAFE_RELEASE(g_pInputLayout);
    SAFE_RELEASE(g_pPixelShader);
    SAFE_RELEASE(g_pVertexShader);
    g_pDevice = nullptr;
    g_pContext = nullptr;
}

void ShaderShadowMapSkinned_Begin()
{
    if (!g_pContext || !g_pVertexShader) return;
    g_pContext->VSSetShader(g_pVertexShader, nullptr, 0);
    g_pContext->PSSetShader(g_pPixelShader, nullptr, 0);
    g_pContext->IASetInputLayout(g_pInputLayout);
    g_pContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer);
    g_pContext->VSSetConstantBuffers(1, 1, &g_pBoneBuffer);

    // トポロジー設定（各モデルで毎回呼ぶのではなくBeginで1回）
    g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

// Stores the light view-projection in row-major form (non-transposed).
// The transpose for GPU upload is deferred to SetWorldMatrix, which sends both together.
void ShaderShadowMapSkinned_SetLightViewProjection(const DirectX::XMFLOAT4X4& matrix)
{
    g_CBData.LightViewProj = matrix;
}

void ShaderShadowMapSkinned_SetWorldMatrix(const DirectX::XMMATRIX& matrix)
{
    if (!g_pContext || !g_pConstantBuffer) return;

    DirectX::XMStoreFloat4x4(&g_CBData.World, DirectX::XMMatrixTranspose(matrix));

    CB_ShadowMapSkinned sendData;
    sendData.World = g_CBData.World;

    DirectX::XMMATRIX lvp = DirectX::XMLoadFloat4x4(&g_CBData.LightViewProj);
    DirectX::XMStoreFloat4x4(&sendData.LightViewProj, DirectX::XMMatrixTranspose(lvp));

    g_pContext->UpdateSubresource(g_pConstantBuffer, 0, nullptr, &sendData, 0, 0);
}

/**
 * @brief ボーン行列を定数バッファに転送する（シャドウマップ用）
 * @param bones ボーン行列配列（MAX_BONES要素分が確保済みであること）
 * @param boneCount 実使用ボーン数（現在は未使用：定数バッファは全体転送のため）
 */
void ShaderShadowMapSkinned_SetBoneMatrices(const DirectX::XMFLOAT4X4* bones, int boneCount)
{
    (void)boneCount;
    if (!g_pContext || !g_pBoneBuffer || !bones) return;
    g_pContext->UpdateSubresource(g_pBoneBuffer, 0, nullptr, bones, 0, 0);
}
