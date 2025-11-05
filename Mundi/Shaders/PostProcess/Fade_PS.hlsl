Texture2D SceneColorTexture : register(t0);
SamplerState LinearSampler : register(s0);

cbuffer FadeCB : register(b0)
{
    float4 FadeColor;    // RGB: 페이드 색상
    float FadeAmount;    // 0.0 (투명) ~ 1.0 (불투명)
    float3 Padding;
}

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

float4 mainPS(PS_INPUT input) : SV_TARGET
{
    // Scene Color 샘플링
    float4 sceneColor = SceneColorTexture.Sample(LinearSampler, input.texCoord);

    // Fade Color와 블렌딩 (FadeAmount에 따라 선형 보간)
    float4 finalColor = lerp(sceneColor, FadeColor, FadeAmount);

    return finalColor;
}
