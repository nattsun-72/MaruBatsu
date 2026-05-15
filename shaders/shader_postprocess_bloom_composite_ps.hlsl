/**
 * @file shader_postprocess_bloom_composite_ps.hlsl
 * @brief ブルーム合成ピクセルシェーダー
 * @detail シーンカラー + ブルーム加算合成 + 既存ブラーエフェクト統合
 * @date 2026/02/23
 */

//=============================================================================
// 定数バッファ
//=============================================================================
cbuffer PostProcessCB : register(b0)
{
    float2 Center;          // ブラー中心（UV座標）
    float Intensity;        // ブラー強度
    float SampleCount;      // サンプル数
    float2 Direction;       // 方向性ブラー用
    float BloomStrength;    // ブルーム合成強度（1.0 = 標準）
    float EffectType;       // 0=none, 1=radial, 2=directional
};

//=============================================================================
// テクスチャ・サンプラ
//=============================================================================
Texture2D SceneTexture : register(t0);  // シーンカラー
Texture2D BloomTexture : register(t1);  // ブルーム（ブラー済み）
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
// ラジアルブラー処理
//=============================================================================
float3 ApplyRadialBlur(float2 uv, float3 sceneColor)
{
    float3 result = sceneColor;

    if (Intensity > 0.001f)
    {
        float2 dir = uv - Center;
        float dist = length(dir);
        float blurAmount = Intensity * dist * 0.5f;

        float4 color = float4(0.0f, 0.0f, 0.0f, 0.0f);
        float totalWeight = 0.0f;

        int samples = (int)SampleCount;

        [unroll(16)]
        for (int i = 0; i < samples; i++)
        {
            float t = (float)i / (float)(samples - 1);
            float scale = 1.0f - blurAmount * t;
            float2 sampleCoord = Center + dir * scale;
            sampleCoord = clamp(sampleCoord, 0.0f, 1.0f);

            float weight = 1.0f - t * 0.5f;
            color += SceneTexture.Sample(LinearSampler, sampleCoord) * weight;
            totalWeight += weight;
        }

        result = (color / totalWeight).rgb;
    }

    return result;
}

//=============================================================================
// 方向性ブラー処理
//=============================================================================
float3 ApplyDirectionalBlur(float2 uv, float3 sceneColor)
{
    float3 result = sceneColor;

    if (Intensity > 0.001f)
    {
        float4 color = float4(0.0f, 0.0f, 0.0f, 0.0f);
        float totalWeight = 0.0f;

        int samples = (int)SampleCount;

        [unroll(16)]
        for (int i = 0; i < samples; i++)
        {
            float t = ((float)i / (float)(samples - 1)) - 0.5f;
            float2 offset = Direction * t * Intensity;
            float2 sampleCoord = clamp(uv + offset, 0.0f, 1.0f);

            float weight = 1.0f - abs(t);
            color += SceneTexture.Sample(LinearSampler, sampleCoord) * weight;
            totalWeight += weight;
        }

        result = (color / totalWeight).rgb;
    }

    return result;
}

//=============================================================================
// ピクセルシェーダーメイン
//=============================================================================
float4 main(PS_INPUT input) : SV_TARGET
{
    float2 uv = input.TexCoord;

    // シーンカラー取得
    float3 sceneColor = SceneTexture.Sample(LinearSampler, uv).rgb;

    // 既存ブラーエフェクト適用
    if (EffectType > 0.5f && EffectType < 1.5f)
    {
        // ラジアルブラー
        sceneColor = ApplyRadialBlur(uv, sceneColor);
    }
    else if (EffectType > 1.5f)
    {
        // 方向性ブラー
        sceneColor = ApplyDirectionalBlur(uv, sceneColor);
    }

    // ブルーム加算合成
    float3 bloom = BloomTexture.Sample(LinearSampler, uv).rgb;
    float3 finalColor = sceneColor + bloom * BloomStrength;

    return float4(saturate(finalColor), 1.0f);
}
