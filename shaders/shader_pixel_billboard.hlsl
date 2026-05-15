/**
 * @file shader_pixel_billboard.hlsl
 * @brief �r���{�[�h�p�s�N�Z���V�F�[�_�[
 * @author Natsume Shidara
 * @date 2025/11/14
 */

// �萔�o�b�t�@ (register b0: Color)
cbuffer ColorBuffer : register(b0)
{
    float4 materialColor;
};

// �s�N�Z���V�F�[�_�[���͍\����
struct PS_INPUT
{
    float4 posH : SV_POSITION; // �ϊ��ςݍ��W
    float4 color : COLOR0; // ���_�J���[
    float2 uv : TEXCOORD0; // �e�N�X�`�����W
};

// �e�N�X�`���ƃT���v���[
Texture2D tex : register(t0); // �e�N�X�`��
SamplerState samp : register(s0); // �T���v���[

struct PS_OUTPUT
{
    float4 color    : SV_TARGET0;
    float4 emissive : SV_TARGET1;
};

PS_OUTPUT main(PS_INPUT ps_in)
{
    PS_OUTPUT output;

    float4 texColor = tex.Sample(samp, ps_in.uv);
    float3 finalRGB = texColor.rgb * ps_in.color.rgb * materialColor.rgb * materialColor.a;

    output.color = float4(finalRGB, 1.0f);
    output.emissive = float4(0.0f, 0.0f, 0.0f, 0.0f);
    return output;
}