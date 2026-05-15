#define MAX_BONES 128

cbuffer VS_WORLD : register(b0)
{
    float4x4 world;
};

cbuffer VS_VIEW : register(b1)
{
    float4x4 view;
};

cbuffer VS_PROJ : register(b2)
{
    float4x4 proj;
};

cbuffer VS_SHADOW : register(b3)
{
    float4x4 LightViewProjection;
};

cbuffer VS_BONES : register(b4)
{
    float4x4 boneMatrices[MAX_BONES];
};

struct VS_INPUT
{
    float3 posL       : POSITION0;
    float3 normalL    : NORMAL0;
    float4 color      : COLOR0;
    float2 uv         : TEXCOORD0;
    uint4  boneIdx    : BLENDINDICES0;
    float4 boneWeight : BLENDWEIGHT0;
};

struct VS_OUTPUT
{
    float4 posH      : SV_POSITION;
    float4 posW      : POSITION0;
    float4 normalW   : NORMAL0;
    float4 color     : COLOR0;
    float2 uv        : TEXCOORD0;
    float4 wPosLight : TEXCOORD1;
};

VS_OUTPUT main(VS_INPUT vs_in)
{
    VS_OUTPUT vs_out;

    float4 localPos = float4(vs_in.posL, 1.0f);
    float4 localNrm = float4(vs_in.normalL, 0.0f);

    float4 skinnedPos = float4(0, 0, 0, 0);
    float4 skinnedNrm = float4(0, 0, 0, 0);
    float totalWeight = 0.0f;

    [unroll]
    for (int i = 0; i < 4; i++)
    {
        float w = vs_in.boneWeight[i];
        if (w > 0.0f)
        {
            uint idx = vs_in.boneIdx[i];
            skinnedPos += w * mul(localPos, boneMatrices[idx]);
            skinnedNrm += w * mul(localNrm, boneMatrices[idx]);
            totalWeight += w;
        }
    }

    if (totalWeight < 0.001f)
    {
        skinnedPos = localPos;
        skinnedNrm = localNrm;
    }
    else
    {
        skinnedPos /= totalWeight;
        skinnedNrm /= totalWeight;
    }

    skinnedPos.w = 1.0f;

    float4 mtxW = mul(skinnedPos, world);
    float4 mtxWV = mul(mtxW, view);
    float4 mtxH = mul(mtxWV, proj);

    vs_out.posH = mtxH;
    vs_out.posW = mtxW;

    vs_out.normalW = normalize(mul(float4(skinnedNrm.xyz, 0.0f), world));

    vs_out.color = vs_in.color;
    vs_out.uv = vs_in.uv;

    vs_out.wPosLight = mul(mtxW, LightViewProjection);

    return vs_out;
}
