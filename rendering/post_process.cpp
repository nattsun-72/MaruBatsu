/****************************************
 * @file post_process.cpp
 * @brief ポストプロセスエフェクト管理
 * @detail ラジアルブラー、モーションブラー、ブルーム等の画面効果
 * @author Natsume Shidara
 * @date 2026/01/10
 * @update 2026/02/23 - ブルームパイプライン追加（MRT → ダウンサンプル → ガウシアンブラー → 合成）
 ****************************************/

#include "post_process.h"
#include "direct3d.h"

#include <fstream>
#include <vector>
#include <algorithm>

using namespace DirectX;

namespace
{
    // --- ブラーサンプリング ---
    constexpr float BLUR_SAMPLE_COUNT = 16.0f;           // ブラーのサンプル数

    // --- エフェクト判定閾値 ---
    constexpr float INTENSITY_THRESHOLD = 0.001f;        // ブラー強度の最小有効値
}

#ifdef USE_ASSET_DLL
#include "asset_dll.h"
#include "resource.h"
#endif

// マクロ定義
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(x) if(x){ x->Release(); x = nullptr; }
#endif

// 定数バッファ構造体

// 合成シェーダー用CB（ブラー + ブルーム統合）
struct PostProcessCB
{
    XMFLOAT2 center;        // ブラー中心（UV座標）
    float intensity;        // ブラー強度
    float sampleCount;      // サンプル数
    XMFLOAT2 direction;     // 方向性ブラー用
    float bloomStrength;    // ブルーム合成強度
    float effectType;       // 0=none, 1=radial, 2=directional
};

// ブルームブラー用CB
struct BloomBlurCB
{
    XMFLOAT2 blurDirection; // テクセルサイズ * 方向
    XMFLOAT2 padding;
};

// 内部変数

// --- シーンRT（カラー出力） ---
static ID3D11Texture2D* g_pSceneTexture = nullptr;
static ID3D11RenderTargetView* g_pSceneRTV = nullptr;
static ID3D11ShaderResourceView* g_pSceneSRV = nullptr;

// --- エミッシブRT（MRT slot1、フル解像度、R16G16B16A16_FLOAT） ---
static ID3D11Texture2D* g_pEmissiveTexture = nullptr;
static ID3D11RenderTargetView* g_pEmissiveRTV = nullptr;
static ID3D11ShaderResourceView* g_pEmissiveSRV = nullptr;

// --- ブルームバッファA/B（半解像度、ピンポンブラー用） ---
static ID3D11Texture2D* g_pBloomTextureA = nullptr;
static ID3D11RenderTargetView* g_pBloomRTV_A = nullptr;
static ID3D11ShaderResourceView* g_pBloomSRV_A = nullptr;

static ID3D11Texture2D* g_pBloomTextureB = nullptr;
static ID3D11RenderTargetView* g_pBloomRTV_B = nullptr;
static ID3D11ShaderResourceView* g_pBloomSRV_B = nullptr;

// --- シェーダー ---
static ID3D11VertexShader* g_pFullscreenVS = nullptr;
static ID3D11PixelShader* g_pCompositePS = nullptr;       // 合成（ブラー+ブルーム統合）
static ID3D11PixelShader* g_pBloomDownsamplePS = nullptr;  // ダウンサンプル
static ID3D11PixelShader* g_pBloomBlurPS = nullptr;        // ガウシアンブラー

// 旧シェーダー（互換性維持、合成シェーダーが代替）
static ID3D11PixelShader* g_pRadialBlurPS = nullptr;
static ID3D11PixelShader* g_pDirectionalBlurPS = nullptr;
static ID3D11PixelShader* g_pPassthroughPS = nullptr;

// --- 定数バッファ ---
static ID3D11Buffer* g_pConstantBuffer = nullptr;     // PostProcessCB
static ID3D11Buffer* g_pBloomBlurCB = nullptr;         // BloomBlurCB
static ID3D11SamplerState* g_pSamplerState = nullptr;

// エフェクト状態
static PostProcessEffect g_CurrentEffect = PostProcessEffect::None;
static float g_Intensity = 0.0f;
static float g_Duration = 0.0f;
static float g_Timer = 0.0f;
static bool g_FadeOut = true;
static XMFLOAT2 g_BlurDirection = { 0.0f, 0.0f };
static float g_InitialIntensity = 0.0f;

// ブルームフラグ（エミッシブ書き込みがあったフレームのみtrue）
static bool g_BloomActive = false;

// 元のレンダーターゲット保存用
static ID3D11RenderTargetView* g_pOriginalRTV = nullptr;
static ID3D11DepthStencilView* g_pOriginalDSV = nullptr;

// デバイス参照
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

// 解像度キャッシュ
static UINT g_FullWidth = 0;
static UINT g_FullHeight = 0;
static UINT g_HalfWidth = 0;
static UINT g_HalfHeight = 0;

// ヘルパー関数：ファイル読み込み
static std::vector<char> LoadFileToBlob(const std::string& path)
{
    std::vector<char> blob;
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs)
    {
        OutputDebugStringA(("PostProcess: file open failed: " + path + "\n").c_str());
        return blob;
    }

    ifs.seekg(0, std::ios::end);
    std::streamoff soff = ifs.tellg();
    if (soff <= 0)
    {
        OutputDebugStringA(("PostProcess: file size invalid: " + path + "\n").c_str());
        return blob;
    }

    size_t filesize = static_cast<size_t>(soff);
    ifs.seekg(0, std::ios::beg);
    blob.resize(filesize);

    ifs.read(blob.data(), static_cast<std::streamsize>(filesize));
    if (!ifs)
    {
        OutputDebugStringA(("PostProcess: read failed: " + path + "\n").c_str());
        blob.clear();
        return blob;
    }

    return blob;
}

// ヘルパー：RT作成
static bool CreateRenderTarget(UINT width, UINT height, DXGI_FORMAT format,
    ID3D11Texture2D** ppTex, ID3D11RenderTargetView** ppRTV, ID3D11ShaderResourceView** ppSRV)
{
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = width;
    texDesc.Height = height;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = format;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    HRESULT hr = g_pDevice->CreateTexture2D(&texDesc, nullptr, ppTex);
    if (FAILED(hr))
    {
        OutputDebugStringA("PostProcess: CreateTexture2D failed\n");
        return false;
    }

    hr = g_pDevice->CreateRenderTargetView(*ppTex, nullptr, ppRTV);
    if (FAILED(hr))
    {
        OutputDebugStringA("PostProcess: CreateRenderTargetView failed\n");
        SAFE_RELEASE(*ppTex);
        return false;
    }

    hr = g_pDevice->CreateShaderResourceView(*ppTex, nullptr, ppSRV);
    if (FAILED(hr))
    {
        OutputDebugStringA("PostProcess: CreateShaderResourceView failed\n");
        SAFE_RELEASE(*ppRTV);
        SAFE_RELEASE(*ppTex);
        return false;
    }

    return true;
}

// ヘルパー：ピクセルシェーダー読み込み
#ifdef USE_ASSET_DLL
static bool LoadPixelShader(int resourceId, ID3D11PixelShader** ppPS)
{
    const void* psData = nullptr;
    size_t psSize = 0;
    if (!AssetDLL_GetData(resourceId, &psData, &psSize) || !psData)
    {
        OutputDebugStringA("PostProcess: PS load failed (DLL)\n");
        return false;
    }

    HRESULT hr = g_pDevice->CreatePixelShader(psData, psSize, nullptr, ppPS);
    if (FAILED(hr))
    {
        OutputDebugStringA("PostProcess: CreatePixelShader failed (DLL)\n");
        return false;
    }
    return true;
}
#else
static bool LoadPixelShader(const std::string& path, ID3D11PixelShader** ppPS)
{
    std::vector<char> blob = LoadFileToBlob(path);
    if (blob.empty())
    {
        OutputDebugStringA(("PostProcess: PS load failed: " + path + "\n").c_str());
        return false;
    }

    HRESULT hr = g_pDevice->CreatePixelShader(blob.data(), blob.size(), nullptr, ppPS);
    if (FAILED(hr))
    {
        OutputDebugStringA(("PostProcess: CreatePixelShader failed: " + path + "\n").c_str());
        return false;
    }
    return true;
}
#endif

// 初期化
void PostProcess_Initialize()
{
    g_pDevice = Direct3D_GetDevice();
    g_pContext = Direct3D_GetContext();

    if (!g_pDevice || !g_pContext)
    {
        OutputDebugStringA("PostProcess_Initialize: invalid device/context\n");
        return;
    }

    g_FullWidth = Direct3D_GetBackBufferWidth();
    g_FullHeight = Direct3D_GetBackBufferHeight();
    g_HalfWidth = (std::max)(g_FullWidth / 2, 1u);
    g_HalfHeight = (std::max)(g_FullHeight / 2, 1u);

    HRESULT hr = S_OK;

    //--------------------------------------
    // レンダーターゲット作成
    //--------------------------------------
    // シーンRT（LDR）
    if (!CreateRenderTarget(g_FullWidth, g_FullHeight, DXGI_FORMAT_R8G8B8A8_UNORM,
        &g_pSceneTexture, &g_pSceneRTV, &g_pSceneSRV))
    {
        PostProcess_Finalize();
        return;
    }

    // エミッシブRT（HDR、フル解像度）
    if (!CreateRenderTarget(g_FullWidth, g_FullHeight, DXGI_FORMAT_R16G16B16A16_FLOAT,
        &g_pEmissiveTexture, &g_pEmissiveRTV, &g_pEmissiveSRV))
    {
        PostProcess_Finalize();
        return;
    }

    // ブルームバッファA（HDR、半解像度）
    if (!CreateRenderTarget(g_HalfWidth, g_HalfHeight, DXGI_FORMAT_R16G16B16A16_FLOAT,
        &g_pBloomTextureA, &g_pBloomRTV_A, &g_pBloomSRV_A))
    {
        PostProcess_Finalize();
        return;
    }

    // ブルームバッファB（HDR、半解像度）
    if (!CreateRenderTarget(g_HalfWidth, g_HalfHeight, DXGI_FORMAT_R16G16B16A16_FLOAT,
        &g_pBloomTextureB, &g_pBloomRTV_B, &g_pBloomSRV_B))
    {
        PostProcess_Finalize();
        return;
    }

    //--------------------------------------
    // 頂点シェーダー読み込み
    //--------------------------------------
    {
        const void* vsData = nullptr;
        size_t vsSize = 0;
#ifdef USE_ASSET_DLL
        std::vector<char> vsBlob;
        if (!AssetDLL_GetData(IDR_SHADER_VS_POSTPROCESS, &vsData, &vsSize) || !vsData)
        {
            OutputDebugStringA("PostProcess: VS load failed (DLL)\n");
            PostProcess_Finalize();
            return;
        }
#else
        const std::string vsPath = "assets/shader/shader_postprocess_vs.cso";
        std::vector<char> vsBlob = LoadFileToBlob(vsPath);
        if (vsBlob.empty())
        {
            OutputDebugStringA("PostProcess: VS load failed\n");
            PostProcess_Finalize();
            return;
        }
        vsData = vsBlob.data();
        vsSize = vsBlob.size();
#endif

        hr = g_pDevice->CreateVertexShader(vsData, vsSize, nullptr, &g_pFullscreenVS);
        if (FAILED(hr))
        {
            OutputDebugStringA("PostProcess: CreateVertexShader failed\n");
            PostProcess_Finalize();
            return;
        }
    }

    //--------------------------------------
    // ピクセルシェーダー読み込み（旧 + 新）
    //--------------------------------------
#ifdef USE_ASSET_DLL
    // 旧シェーダー（フォールバック用に残す）
    if (!LoadPixelShader(IDR_SHADER_PS_PP_RADIAL, &g_pRadialBlurPS))
    { PostProcess_Finalize(); return; }

    if (!LoadPixelShader(IDR_SHADER_PS_PP_DIRECTIONAL, &g_pDirectionalBlurPS))
    { PostProcess_Finalize(); return; }

    if (!LoadPixelShader(IDR_SHADER_PS_PP_PASSTHROUGH, &g_pPassthroughPS))
    { PostProcess_Finalize(); return; }

    // 新ブルームシェーダー
    if (!LoadPixelShader(IDR_SHADER_PS_PP_BLOOM_DOWN, &g_pBloomDownsamplePS))
    { PostProcess_Finalize(); return; }

    if (!LoadPixelShader(IDR_SHADER_PS_PP_BLOOM_BLUR, &g_pBloomBlurPS))
    { PostProcess_Finalize(); return; }

    if (!LoadPixelShader(IDR_SHADER_PS_PP_BLOOM_COMP, &g_pCompositePS))
    { PostProcess_Finalize(); return; }
#else
    // 旧シェーダー（フォールバック用に残す）
    if (!LoadPixelShader("assets/shader/shader_postprocess_radial_ps.cso", &g_pRadialBlurPS))
    { PostProcess_Finalize(); return; }

    if (!LoadPixelShader("assets/shader/shader_postprocess_directional_ps.cso", &g_pDirectionalBlurPS))
    { PostProcess_Finalize(); return; }

    if (!LoadPixelShader("assets/shader/shader_postprocess_passthrough_ps.cso", &g_pPassthroughPS))
    { PostProcess_Finalize(); return; }

    // 新ブルームシェーダー
    if (!LoadPixelShader("assets/shader/shader_postprocess_bloom_downsample_ps.cso", &g_pBloomDownsamplePS))
    { PostProcess_Finalize(); return; }

    if (!LoadPixelShader("assets/shader/shader_postprocess_bloom_blur_ps.cso", &g_pBloomBlurPS))
    { PostProcess_Finalize(); return; }

    if (!LoadPixelShader("assets/shader/shader_postprocess_bloom_composite_ps.cso", &g_pCompositePS))
    { PostProcess_Finalize(); return; }
#endif

    //--------------------------------------
    // 定数バッファ作成
    //--------------------------------------
    {
        D3D11_BUFFER_DESC cbDesc = {};
        cbDesc.ByteWidth = ((sizeof(PostProcessCB) + 15) / 16) * 16;
        cbDesc.Usage = D3D11_USAGE_DYNAMIC;
        cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        hr = g_pDevice->CreateBuffer(&cbDesc, nullptr, &g_pConstantBuffer);
        if (FAILED(hr))
        {
            OutputDebugStringA("PostProcess: CreateBuffer (CB) failed\n");
            PostProcess_Finalize();
            return;
        }
    }

    {
        D3D11_BUFFER_DESC cbDesc = {};
        cbDesc.ByteWidth = ((sizeof(BloomBlurCB) + 15) / 16) * 16;
        cbDesc.Usage = D3D11_USAGE_DYNAMIC;
        cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        hr = g_pDevice->CreateBuffer(&cbDesc, nullptr, &g_pBloomBlurCB);
        if (FAILED(hr))
        {
            OutputDebugStringA("PostProcess: CreateBuffer (BloomBlurCB) failed\n");
            PostProcess_Finalize();
            return;
        }
    }

    //--------------------------------------
    // サンプラーステート作成
    //--------------------------------------
    {
        D3D11_SAMPLER_DESC sampDesc = {};
        sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;

        hr = g_pDevice->CreateSamplerState(&sampDesc, &g_pSamplerState);
        if (FAILED(hr))
        {
            OutputDebugStringA("PostProcess: CreateSamplerState failed\n");
            PostProcess_Finalize();
            return;
        }
    }

    // 初期状態
    g_CurrentEffect = PostProcessEffect::None;
    g_Intensity = 0.0f;
    g_Timer = 0.0f;

    OutputDebugStringA("PostProcess_Initialize: success (with bloom)\n");
}

// 終了処理
void PostProcess_Finalize()
{
    SAFE_RELEASE(g_pSamplerState);
    SAFE_RELEASE(g_pBloomBlurCB);
    SAFE_RELEASE(g_pConstantBuffer);

    SAFE_RELEASE(g_pCompositePS);
    SAFE_RELEASE(g_pBloomBlurPS);
    SAFE_RELEASE(g_pBloomDownsamplePS);
    SAFE_RELEASE(g_pPassthroughPS);
    SAFE_RELEASE(g_pDirectionalBlurPS);
    SAFE_RELEASE(g_pRadialBlurPS);
    SAFE_RELEASE(g_pFullscreenVS);

    SAFE_RELEASE(g_pBloomSRV_B);
    SAFE_RELEASE(g_pBloomRTV_B);
    SAFE_RELEASE(g_pBloomTextureB);
    SAFE_RELEASE(g_pBloomSRV_A);
    SAFE_RELEASE(g_pBloomRTV_A);
    SAFE_RELEASE(g_pBloomTextureA);
    SAFE_RELEASE(g_pEmissiveSRV);
    SAFE_RELEASE(g_pEmissiveRTV);
    SAFE_RELEASE(g_pEmissiveTexture);
    SAFE_RELEASE(g_pSceneSRV);
    SAFE_RELEASE(g_pSceneRTV);
    SAFE_RELEASE(g_pSceneTexture);

    g_pDevice = nullptr;
    g_pContext = nullptr;

    OutputDebugStringA("PostProcess_Finalize: released\n");
}

// シーン描画開始（MRT: slot0=シーン, slot1=エミッシブ）
void PostProcess_BeginScene()
{
    if (!g_pContext || !g_pSceneRTV) return;

    // 現在のレンダーターゲットを保存
    g_pContext->OMGetRenderTargets(1, &g_pOriginalRTV, &g_pOriginalDSV);

    // MRTバインド: slot0=シーン, slot1=エミッシブ
    ID3D11RenderTargetView* rtvs[2] = { g_pSceneRTV, g_pEmissiveRTV };
    g_pContext->OMSetRenderTargets(2, rtvs, g_pOriginalDSV);

    // ブルームフラグリセット（このフレームでエミッシブ書き込みがあるか監視）
    g_BloomActive = false;

    // 両方クリア
    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    float clearBlack[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    g_pContext->ClearRenderTargetView(g_pSceneRTV, clearColor);
    g_pContext->ClearRenderTargetView(g_pEmissiveRTV, clearBlack);
}

// MRT再バインド（影パス等の後に呼ぶ）
void PostProcess_RestoreSceneTargets()
{
    if (!g_pContext || !g_pSceneRTV) return;

    // MRT復帰: slot0=シーン, slot1=エミッシブ
    ID3D11RenderTargetView* rtvs[2] = { g_pSceneRTV, g_pEmissiveRTV };
    g_pContext->OMSetRenderTargets(2, rtvs, g_pOriginalDSV);

    // 深度バッファをクリア（影パスで使われた可能性があるため）
    if (g_pOriginalDSV)
    {
        g_pContext->ClearDepthStencilView(g_pOriginalDSV,
            D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    }

    // ビューポートをフル解像度に復帰
    D3D11_VIEWPORT vp = {};
    vp.Width = static_cast<float>(g_FullWidth);
    vp.Height = static_cast<float>(g_FullHeight);
    vp.MaxDepth = 1.0f;
    g_pContext->RSSetViewports(1, &vp);
}

// ヘルパー：ビューポート設定
static void SetViewport(UINT width, UINT height)
{
    D3D11_VIEWPORT vp = {};
    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;
    vp.Width = static_cast<float>(width);
    vp.Height = static_cast<float>(height);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    g_pContext->RSSetViewports(1, &vp);
}

// ヘルパー：フルスクリーン描画パス
static void DrawFullscreenPass(ID3D11PixelShader* pPS,
    ID3D11ShaderResourceView* srv0,
    ID3D11ShaderResourceView* srv1,
    ID3D11RenderTargetView* pRTV,
    UINT vpWidth, UINT vpHeight)
{
    // RT設定
    g_pContext->OMSetRenderTargets(1, &pRTV, nullptr);
    SetViewport(vpWidth, vpHeight);

    // テクスチャ設定
    ID3D11ShaderResourceView* srvs[2] = { srv0, srv1 };
    g_pContext->PSSetShaderResources(0, 2, srvs);

    // シェーダー設定
    g_pContext->PSSetShader(pPS, nullptr, 0);

    // 描画
    g_pContext->Draw(3, 0);

    // SRVクリア
    ID3D11ShaderResourceView* nullSRVs[2] = { nullptr, nullptr };
    g_pContext->PSSetShaderResources(0, 2, nullSRVs);
}

// シーン描画終了 & ポストプロセス適用
void PostProcess_EndScene()
{
    if (!g_pContext || !g_pOriginalRTV) return;

    // MRT解除（シーンRTをSRVとして使うため）
    ID3D11RenderTargetView* nullRTVs[2] = { nullptr, nullptr };
    g_pContext->OMSetRenderTargets(2, nullRTVs, nullptr);

    // 深度テスト無効化
    Direct3D_DepthStencilStateDepthIsEnable(false);

    // 共通設定：VS / トポロジ / サンプラ
    g_pContext->VSSetShader(g_pFullscreenVS, nullptr, 0);
    g_pContext->IASetInputLayout(nullptr);
    g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    g_pContext->PSSetSamplers(0, 1, &g_pSamplerState);

    // ブルームバッファを毎フレームクリア（前フレームの残像防止）
    float clearBlack[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    g_pContext->ClearRenderTargetView(g_pBloomRTV_A, clearBlack);
    g_pContext->ClearRenderTargetView(g_pBloomRTV_B, clearBlack);

    // ブルームパス（エミッシブ描画があったフレームのみ実行）
    if (g_BloomActive)
    {
        // Pass 1: エミッシブRT → ダウンサンプル → BloomA
        DrawFullscreenPass(g_pBloomDownsamplePS,
            g_pEmissiveSRV, nullptr,
            g_pBloomRTV_A,
            g_HalfWidth, g_HalfHeight);

        // Pass 2: BloomA → 水平ブラー → BloomB
        {
            D3D11_MAPPED_SUBRESOURCE mapped;
            g_pContext->Map(g_pBloomBlurCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
            BloomBlurCB* cb = (BloomBlurCB*)mapped.pData;
            cb->blurDirection = { 1.0f / static_cast<float>(g_HalfWidth), 0.0f };
            cb->padding = { 0.0f, 0.0f };
            g_pContext->Unmap(g_pBloomBlurCB, 0);
            g_pContext->PSSetConstantBuffers(0, 1, &g_pBloomBlurCB);

            DrawFullscreenPass(g_pBloomBlurPS,
                g_pBloomSRV_A, nullptr,
                g_pBloomRTV_B,
                g_HalfWidth, g_HalfHeight);
        }

        // Pass 3: BloomB → 垂直ブラー → BloomA
        {
            D3D11_MAPPED_SUBRESOURCE mapped;
            g_pContext->Map(g_pBloomBlurCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
            BloomBlurCB* cb = (BloomBlurCB*)mapped.pData;
            cb->blurDirection = { 0.0f, 1.0f / static_cast<float>(g_HalfHeight) };
            cb->padding = { 0.0f, 0.0f };
            g_pContext->Unmap(g_pBloomBlurCB, 0);
            g_pContext->PSSetConstantBuffers(0, 1, &g_pBloomBlurCB);

            DrawFullscreenPass(g_pBloomBlurPS,
                g_pBloomSRV_B, nullptr,
                g_pBloomRTV_A,
                g_HalfWidth, g_HalfHeight);
        }
    }

    // 最終合成パス: シーン(t0) + BloomA(t1) → バックバッファ
    {
        D3D11_MAPPED_SUBRESOURCE mapped;
        g_pContext->Map(g_pConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        PostProcessCB* cb = (PostProcessCB*)mapped.pData;
        cb->center = { 0.5f, 0.5f };
        cb->intensity = g_Intensity;
        cb->sampleCount = BLUR_SAMPLE_COUNT;
        cb->direction = g_BlurDirection;
        cb->bloomStrength = g_BloomActive ? 1.0f : 0.0f;

        if (g_CurrentEffect == PostProcessEffect::RadialBlur && g_Intensity > INTENSITY_THRESHOLD)
            cb->effectType = 1.0f;
        else if (g_CurrentEffect == PostProcessEffect::DirectionalBlur && g_Intensity > INTENSITY_THRESHOLD)
            cb->effectType = 2.0f;
        else
            cb->effectType = 0.0f;

        g_pContext->Unmap(g_pConstantBuffer, 0);
        g_pContext->PSSetConstantBuffers(0, 1, &g_pConstantBuffer);

        DrawFullscreenPass(g_pCompositePS,
            g_pSceneSRV, g_BloomActive ? g_pBloomSRV_A : nullptr,
            g_pOriginalRTV,
            g_FullWidth, g_FullHeight);
    }

    // 保存していたリソースを解放
    SAFE_RELEASE(g_pOriginalRTV);
    SAFE_RELEASE(g_pOriginalDSV);

    // 深度テスト有効化
    Direct3D_DepthStencilStateDepthIsEnable(true);

    // ビューポートをフル解像度に復帰
    SetViewport(g_FullWidth, g_FullHeight);
}

// エフェクト制御
void PostProcess_StartRadialBlur(float intensity, float duration, bool fadeOut)
{
    g_CurrentEffect = PostProcessEffect::RadialBlur;
    g_Intensity = intensity;
    g_InitialIntensity = intensity;
    g_Duration = duration;
    g_Timer = 0.0f;
    g_FadeOut = fadeOut;
}

void PostProcess_StartDirectionalBlur(const XMFLOAT2& direction, float intensity, float duration)
{
    g_CurrentEffect = PostProcessEffect::DirectionalBlur;
    g_Intensity = intensity;
    g_InitialIntensity = intensity;
    g_Duration = duration;
    g_Timer = 0.0f;
    g_FadeOut = true;

    // 方向を正規化
    XMVECTOR vDir = XMLoadFloat2(&direction);
    vDir = XMVector2Normalize(vDir);
    XMStoreFloat2(&g_BlurDirection, vDir);
}

void PostProcess_Stop()
{
    g_CurrentEffect = PostProcessEffect::None;
    g_Intensity = 0.0f;
    g_Timer = 0.0f;
}

void PostProcess_Update(float dt)
{
    if (g_CurrentEffect == PostProcessEffect::None) return;

    g_Timer += dt;

    if (g_Timer >= g_Duration)
    {
        // エフェクト終了
        g_CurrentEffect = PostProcessEffect::None;
        g_Intensity = 0.0f;
        return;
    }

    // フェードアウト処理
    if (g_FadeOut)
    {
        float t = g_Timer / g_Duration;
        // イージング（最初は強く、徐々に弱く）
        float easedT = 1.0f - (1.0f - t) * (1.0f - t);
        g_Intensity = g_InitialIntensity * (1.0f - easedT);
    }
}

// 状態取得
bool PostProcess_IsActive()
{
    return g_CurrentEffect != PostProcessEffect::None && g_Intensity > INTENSITY_THRESHOLD;
}

PostProcessEffect PostProcess_GetCurrentEffect()
{
    return g_CurrentEffect;
}

float PostProcess_GetCurrentIntensity()
{
    return g_Intensity;
}

void PostProcess_NotifyBloomActive()
{
    g_BloomActive = true;
}
