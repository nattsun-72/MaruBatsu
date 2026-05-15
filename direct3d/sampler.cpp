/****************************************
 * @file sampler.cpp
 * @brief テクスチャサンプラーステート管理モジュール
 * @author Natsume Shidara
 * @date 2025/06/06
 *
 * ポイント・リニア・異方性フィルタリングの
 * サンプラーステートを生成・管理・切り替えする。
 ****************************************/

#include "sampler.h"
#include "direct3d.h"

namespace {
    constexpr UINT MAX_ANISOTROPY = 16; // 異方性フィルタリング最大サンプル数
}

static ID3D11Device* g_pDevice;
static ID3D11DeviceContext* g_pContext;
static ID3D11SamplerState* g_pSamplerStatePoint = nullptr;
static ID3D11SamplerState* g_pSamplerStateLinear = nullptr;
static ID3D11SamplerState* g_pSamplerStateAnisotropic = nullptr;

/**
 * @brief サンプラーステート初期化
 * @param pDevice  D3D11デバイス
 * @param pContext D3D11デバイスコンテキスト
 *
 * ポイント・リニア・異方性の3種類のサンプラーステートを作成する。
 */
void Sampler_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    g_pDevice = pDevice;
    g_pContext = pContext;

    // サンプラーステート設定
    D3D11_SAMPLER_DESC sampler_desc{};

    // UV参照外の取り扱い（UVアドレッシングモード）
    sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.MipLODBias = 0;
    sampler_desc.MaxAnisotropy = MAX_ANISOTROPY;
    sampler_desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    sampler_desc.MinLOD = 0;
    sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;

    // フィルタリング
    sampler_desc.Filter = D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
    g_pDevice->CreateSamplerState(&sampler_desc, &g_pSamplerStatePoint);

    sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    g_pDevice->CreateSamplerState(&sampler_desc, &g_pSamplerStateLinear);

    sampler_desc.Filter = D3D11_FILTER_ANISOTROPIC;
    g_pDevice->CreateSamplerState(&sampler_desc, &g_pSamplerStateAnisotropic);
}

/** @brief サンプラー終了処理（全サンプラーステート解放） */
void Sampler_Finalize()
{
    SAFE_RELEASE(g_pSamplerStatePoint)
    SAFE_RELEASE(g_pSamplerStateLinear)
    SAFE_RELEASE(g_pSamplerStateAnisotropic)
}

/** @brief ポイントフィルタリングをピクセルシェーダーに設定 */
void Sampler_SetFilterPoint()
{
    g_pContext->PSSetSamplers(0, 1, &g_pSamplerStatePoint);
}

/** @brief リニアフィルタリングをピクセルシェーダーに設定 */
void Sampler_SetFilterLinear()
{
    g_pContext->PSSetSamplers(0, 1, &g_pSamplerStateLinear);
}

/** @brief 異方性フィルタリングをピクセルシェーダーに設定 */
void Sampler_SetFilterAnisotropic()
{
    g_pContext->PSSetSamplers(0, 1, &g_pSamplerStateAnisotropic);
}
