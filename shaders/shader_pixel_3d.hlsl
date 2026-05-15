/**
 * @file shader_pixel_3d.hlsl
 * @brief 3D�`��p�s�N�Z���V�F�[�_�[�iShadowMap�Ή��EPCF�\�t�g�V���h�E�E��������Phong�Łj
 * @author Natsume
 * @date 2025/12/10
 * @update 2025/12/19 - ���[�v�A�����[���x���Ή�
 * @update 2026/02/03 - ���C�e�B���O�����i���邳���P�j
 */

//=============================================================================
// �萔�o�b�t�@�ݒ�
//=============================================================================
cbuffer PS_DIFFUSE_BUFFER : register(b0)
{
    float4 diffuse_color;
};

cbuffer PS_AMBIENT_BUFFER : register(b1)
{
    float4 ambient_color;
};

cbuffer PS_DIRECTIONAL_BUFFER : register(b2)
{
    float4 directonal_world_vector;
    float4 directonal_color;
};

cbuffer PS_SPECULAR_BUFFER : register(b3)
{
    float4 specular_color;
    float3 eye_posW;
    float specular_power;
};

struct PointLight
{
    float3 posW;
    float range;
    float4 color;
};

cbuffer PS_POINTLIGHT_BUFFER : register(b4)
{
    PointLight pointLight[4];
    int pointLightCount;
    float3 pointLight_dummy;
};

// 断面テクスチャブレンド用（b5）
// sliceBlend < 0 : 通常描画（ブレンドなし）
// sliceBlend = 0 : 横切りテクスチャのみ
// sliceBlend = 1 : 縦切りテクスチャのみ
// 0 < sliceBlend < 1 : 横/縦をブレンド
cbuffer PS_SLICE_BLEND_BUFFER : register(b5)
{
    float sliceBlend;
    float sliceGlowIntensity; // 断面グロー強度（0=なし、1=最大）
    float2 sliceBlend_pad;
};

//=============================================================================
// �e�N�X�`���E�T���v��
//=============================================================================
Texture2D tex : register(t0);
Texture2D g_ShadowMap : register(t1);
Texture2D texSliceVertical : register(t2); // 縦切り断面テクスチャ
SamplerState samp : register(s0);

//=============================================================================
// ���͍\����
//=============================================================================
struct PS_INPUT
{
    float4 posH : SV_POSITION;
    float4 posW : POSITION0;
    float4 normalW : NORMAL0;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
    float4 wPosLight : TEXCOORD1;
};

//=============================================================================
// ���C�e�B���O�����p�����[�^
//=============================================================================
#define SHADOW_MIN          0.6f
#define SHADOW_BIAS         0.002f
#define DIFFUSE_INTENSITY   1.1f
#define SPECULAR_INTENSITY  0.6f
#define POINT_DIFFUSE       0.6f
#define POINT_SPECULAR      0.5f

//=============================================================================
// �\�t�g�V���h�E�v�Z�֐� (PCF 3x3)
//=============================================================================
float CalcShadowFactor(float4 wPosLight)
{
    float3 projCoords = wPosLight.xyz / wPosLight.w;

    float2 shadowUV;
    shadowUV.x = 0.5f * projCoords.x + 0.5f;
    shadowUV.y = -0.5f * projCoords.y + 0.5f;

    float result = 1.0f;

    if (shadowUV.x >= 0.0f && shadowUV.x <= 1.0f &&
        shadowUV.y >= 0.0f && shadowUV.y <= 1.0f)
    {
        float currentDepth = projCoords.z;
        float shadowFactor = 0.0f;

        float2 texelSize = 1.0f / 2048.0f;

        [unroll]
        for (int y = -1; y <= 1; y++)
        {
            [unroll]
            for (int x = -1; x <= 1; x++)
            {
                float pcfDepth = g_ShadowMap.Sample(samp, shadowUV + float2(x, y) * texelSize).r;
                shadowFactor += (currentDepth - SHADOW_BIAS > pcfDepth) ? SHADOW_MIN : 1.0f;
            }
        }

        result = shadowFactor / 9.0f;
    }

    return result;
}

//=============================================================================
// ���C���V�F�[�_�[
//=============================================================================
//=============================================================================
// MRT出力構造体
//=============================================================================
struct PS_OUTPUT
{
    float4 color    : SV_TARGET0; // シーンカラー
    float4 emissive : SV_TARGET1; // エミッシブ（ブルーム用）
};

PS_OUTPUT main(PS_INPUT ps_in)
{
    PS_OUTPUT output;
    //--------------------------------------
    // テクスチャサンプリング（断面ブレンド対応）
    //--------------------------------------
    float4 texColor;
    if (sliceBlend >= 0.0f)
    {
        // 断面メッシュ：横切り(t0)と縦切り(t2)をブレンド
        float4 texH = tex.Sample(samp, ps_in.uv);
        float4 texV = texSliceVertical.Sample(samp, ps_in.uv);
        texColor = lerp(texH, texV, sliceBlend);
    }
    else
    {
        // 通常メッシュ
        texColor = tex.Sample(samp, ps_in.uv);
    }
    float3 baseColor = texColor.rgb * ps_in.color.rgb * diffuse_color.rgb;
    float alpha = texColor.a * ps_in.color.a * diffuse_color.a;

    //--------------------------------------
    // �e�W���̎擾
    //--------------------------------------
    float shadowFactor = CalcShadowFactor(ps_in.wPosLight);

    //--------------------------------------
    // ���C�e�B���O�v�Z����
    //--------------------------------------
    float3 N = normalize(ps_in.normalW.xyz);
    float3 L = normalize(-directonal_world_vector.xyz);
    float3 toEye = normalize(eye_posW - ps_in.posW.xyz);

    //--------------------------------------
    // �g�U���� (Lambert)
    //--------------------------------------
    float diffFactor = max(dot(N, L), 0.0f);
    float3 diffuse = baseColor * directonal_color.rgb * diffFactor * shadowFactor * DIFFUSE_INTENSITY;

    //--------------------------------------
    // ���ʔ��� (Phong)
    //--------------------------------------
    float3 R = reflect(-L, N);
    float specFactor = pow(max(dot(R, toEye), 0.0f), specular_power);
    float3 specular = specular_color.rgb * specFactor * directonal_color.rgb * SPECULAR_INTENSITY * shadowFactor;

    //--------------------------------------
    // ���� (Ambient)
    //--------------------------------------
    float3 ambient = baseColor * ambient_color.rgb;

    //--------------------------------------
    // ���� (Directional Light)
    //--------------------------------------
    float3 color = ambient + diffuse + specular;

    //--------------------------------------
    // �_�������� (Point Lights)
    //--------------------------------------
    [unroll(4)]
    for (int i = 0; i < pointLightCount; i++)
    {
        float3 lightToPixel = ps_in.posW.xyz - pointLight[i].posW;
        float D = length(lightToPixel);
        
        float A = pow(max(1.0f - 1.0f / pointLight[i].range * D, 0.0f), 2.0f);

        float3 L_point = -normalize(lightToPixel);
        float dl = max(0.0f, dot(L_point, N));
        color += baseColor * pointLight[i].color.rgb * A * dl * POINT_DIFFUSE;

        float3 r_point = reflect(normalize(lightToPixel), N);
        float t = pow(max(dot(r_point, toEye), 0.0f), specular_power);
        color += pointLight[i].color.rgb * t * A * POINT_SPECULAR;
    }
    
    //--------------------------------------
    // 断面グロー（電熱線カット表現）
    //--------------------------------------
    float3 emissive = float3(0.0f, 0.0f, 0.0f);
    if (sliceBlend >= 0.0f && sliceGlowIntensity > 0.0f)
    {
        // オレンジ固定 → フェードアウト（白熱なし）
        float3 hotColor = float3(1.0f, 0.4f, 0.05f);

        // エミッシブ出力（強度に応じてフェードアウト）
        emissive = hotColor * sliceGlowIntensity * 2.0f;

        // シーン色にも加算（直接的な発光感）
        color += emissive * 0.4f;
    }

    output.color = float4(saturate(color), alpha);
    output.emissive = float4(emissive, 1.0f);
    return output;
}