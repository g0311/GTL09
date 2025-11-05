
Texture2D SceneColorTexture : register(t0);
SamplerState LinearSampler : register(s0);

cbuffer GammaCorrectionCB : register(b0)
{
    float GammaValue;
    float3 Padding;
}

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

float4 mainPS(PS_INPUT input) : SV_TARGET
{
    float3 sceneColor = SceneColorTexture.Sample(LinearSampler, input.texCoord).rgb;

    // 감마 보정: pow(color, 1.0 / gamma)
    float3 correctedColor = pow(sceneColor, 1.0f / GammaValue);

    return float4(correctedColor, 1.f);
}
