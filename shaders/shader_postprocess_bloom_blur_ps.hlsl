/**
 * @file shader_postprocess_bloom_blur_ps.hlsl
 * @brief ブルーム用分離型ガウシアンブラーピクセルシェーダー
 * @detail 9-tap ガウシアンブラー。Direction で水平/垂直を切り替え
 * @date 2026/02/23
 */

//=============================================================================
// 定数バッファ
//=============================================================================
cbuffer BloomBlurCB : register(b0)
{
    float2 BlurDirection; // (1/w, 0) = 水平、(0, 1/h) = 垂直
    float2 BlurPadding;
};

//=============================================================================
// テクスチャ・サンプラ
//=============================================================================
Texture2D BloomTexture : register(t0);
SamplerState LinearSampler : register(s0);

//=============================================================================
// 入力構造体
//=============================================================================
struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

//=============================================================================
// 9-tap ガウシアン重み（sigma ~= 4.0）
//=============================================================================
static const float weights[5] = {
    0.227027f, 0.194594f, 0.121621f, 0.054054f, 0.016216f
};

//=============================================================================
// ピクセルシェーダーメイン
//=============================================================================
float4 main(PS_INPUT input) : SV_TARGET
{
    float2 uv = input.TexCoord;

    // 中心テクセル
    float4 result = BloomTexture.Sample(LinearSampler, uv) * weights[0];

    // 両方向にサンプリング（9-tap = 中心 + 4*2）
    [unroll]
    for (int i = 1; i < 5; i++)
    {
        float2 offset = BlurDirection * (float)i;
        result += BloomTexture.Sample(LinearSampler, uv + offset) * weights[i];
        result += BloomTexture.Sample(LinearSampler, uv - offset) * weights[i];
    }

    return result;
}
