/**
 * @file shader_pixel_field.hlsl
 * @brief 3D�t�B�[���h�`��p�s�N�Z���V�F�[�_�[
 * @author Natsume Shidara
 * @date 2025/06/10
 * @UpDate 2025/10/29
 */
cbuffer PS_DIFFUSE_BUFFER : register(b0)
{
    float4 diffuse_color; // �}�e���A���̊g�U���ˌW���iRGBA�j
};

cbuffer PS_AMBIENT_BUFFER : register(b1)
{
    float4 ambient_color; // ����
};

cbuffer PS_DIRECTIONAL_BUFFER : register(b2)
{
    float4 directonal_world_vector; // ���̕����i���[���h��ԁj
    float4 directonal_color; // ���s���̐F
};

cbuffer PS_SPECULAR_BUFFER : register(b3)
{
    float4 specular_color; // ���ʔ��ːF
    float3 eye_posW; // ���_�ʒu�i���[���h���W�j
    float specular_power; // ���ʃn�C���C�g�̉s��
};

#define PI 3.1415926535897932384626433f



struct PS_INPUT
{
    float4 posH : SV_POSITION; // �N���b�v��Ԉʒu
    float4 posW : POSITION0; // ���[���h��Ԉʒu
    float4 normalW : NORMAL0; // ���[���h��Ԗ@���ifloat4�j
    float4 blend : COLOR0; // ���_�J���[
    float2 uv : TEXCOORD0; // �e�N�X�`�����W
};

Texture2D tex0 : register(t0); // �e�N�X�`��
Texture2D tex1 : register(t1); // �e�N�X�`��

SamplerState samp : register(s0);


struct PS_OUTPUT
{
    float4 color    : SV_TARGET0;
    float4 emissive : SV_TARGET1;
};

PS_OUTPUT main(PS_INPUT ps_in)
{
    PS_OUTPUT output;

    float2 uv = ps_in.uv;
    float blendRate = saturate(ps_in.blend.r);

    float4 tex0Color = tex0.Sample(samp, ps_in.uv);
    float4 tex1Color = tex1.Sample(samp, ps_in.uv);

    float4 tex_Color = tex0Color * ps_in.blend.g + tex1Color * ps_in.blend.r;
    float alpha = tex0Color.a * ps_in.blend.g + tex1Color.a * ps_in.blend.r;

    float3 N = normalize(ps_in.normalW.xyz);
    float3 L = normalize(-directonal_world_vector.xyz);
    float3 toEye = normalize(eye_posW - ps_in.posW.xyz);
    float3 R = reflect(-L, N);

    float diffFactor = max(dot(N, L), 0.0f);
    float3 diffuse = tex_Color.rgb * directonal_color.rgb * diffFactor;
    float3 ambient = tex_Color.rgb * ambient_color.rgb;

    float specFactor = pow(max(dot(R, toEye), 0.0f), specular_power);
    float3 specular = specular_color.rgb * specFactor * directonal_color.rgb;

    float3 color = ambient + diffuse + specular;

    output.color = float4(saturate(color), alpha);
    output.emissive = float4(0.0f, 0.0f, 0.0f, 0.0f);
    return output;
}
