#define MAX_BONES 128

cbuffer ConstantBuffer : register(b0)
{
    matrix World;
    matrix LightViewProj;
};

cbuffer BoneBuffer : register(b1)
{
    matrix boneMatrices[MAX_BONES];
};

struct VS_IN
{
    float3 pos         : POSITION;
    float3 normal      : NORMAL;
    float4 color       : COLOR;
    float2 uv          : TEXCOORD;
    uint4  boneIdx     : BLENDINDICES;
    float4 boneWeight  : BLENDWEIGHT;
};

struct VS_OUT
{
    float4 pos : SV_POSITION;
};

VS_OUT VS_Main(VS_IN input)
{
    VS_OUT output;

    float4 localPos = float4(input.pos, 1.0f);
    float4 skinnedPos = float4(0, 0, 0, 0);
    float totalWeight = 0.0f;

    [unroll]
    for (int i = 0; i < 4; i++)
    {
        float w = input.boneWeight[i];
        if (w > 0.0f)
        {
            uint idx = input.boneIdx[i];
            skinnedPos += w * mul(localPos, boneMatrices[idx]);
            totalWeight += w;
        }
    }

    if (totalWeight < 0.001f)
        skinnedPos = localPos;
    else
        skinnedPos /= totalWeight;

    skinnedPos.w = 1.0f;

    float4 wPos = mul(skinnedPos, World);
    output.pos = mul(wPos, LightViewProj);

    return output;
}

void PS_Main(VS_OUT input)
{
}
