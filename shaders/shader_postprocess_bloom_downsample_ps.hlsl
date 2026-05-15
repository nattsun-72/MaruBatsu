/**
 * @file shader_postprocess_bloom_downsample_ps.hlsl
 * @brief ブルーム用ダウンサンプルピクセルシェーダー
 * @detail エミッシブRTを半解像度にダウンサンプル（4テクセル平均）
 * @date 2026/02/23
 */

//=============================================================================
// テクスチャ・サンプラ
//=============================================================================
Texture2D EmissiveTexture : register(t0);
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
// ピクセルシェーダーメイン
// 2x2 テクセルの平均でダウンサンプル
//=============================================================================
float4 main(PS_INPUT input) : SV_TARGET
{
    // テクスチャのサイズ取得
    float2 texSize;
    EmissiveTexture.GetDimensions(texSize.x, texSize.y);
    float2 texelSize = 1.0f / texSize;

    // 2x2 ボックスフィルタ（ハーフピクセルオフセット）
    float2 uv = input.TexCoord;
    float4 color = float4(0.0f, 0.0f, 0.0f, 0.0f);
    color += EmissiveTexture.Sample(LinearSampler, uv + float2(-0.5f, -0.5f) * texelSize);
    color += EmissiveTexture.Sample(LinearSampler, uv + float2( 0.5f, -0.5f) * texelSize);
    color += EmissiveTexture.Sample(LinearSampler, uv + float2(-0.5f,  0.5f) * texelSize);
    color += EmissiveTexture.Sample(LinearSampler, uv + float2( 0.5f,  0.5f) * texelSize);

    return color * 0.25f;
}
