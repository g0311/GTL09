
Texture2D SceneColorTexture : register(t0);
SamplerState LinearSampler : register(s0);

cbuffer PostProcessCB : register(b0)
{
    float Intensity;
    float Smoothness;
    float2 Padding;
}

cbuffer ViewportConstants : register(b10)
{
    float4 ViewportRect;  // (MinX, MinY, Width, Height)
    float4 ScreenSize;    // (Width, Height, 1/Width, 1/Height)
};

struct PS_INPUT
{
    float4 position : SV_POSITION; // Transformed position to pass to the pixel shader
    float2 texCoord : TEXCOORD0;
};

float4 mainPS(PS_INPUT input) : SV_TARGET
{
    float3 sceneColor = SceneColorTexture.Sample(LinearSampler, input.texCoord).rgb;

    float2 localUV = (input.position.xy - ViewportRect.xy) / ViewportRect.zw;
    float2 centeredUV = localUV * 2.0 - 1.0;

    // 중점부터의 거리
    float distance = length(centeredUV);

    // 0~1로 정규화
    float normalizedDist = distance / sqrt(2.f);

    
    // x < min  → 0.0
    // x > max  → 1.0
    // min < x < max → 부드러운 S-커브 보간
    //
    // S-커브 공식:
    // t = (x - min) / (max - min)
    // result = t * t * (3 - 2 * t)
    float vignette = smoothstep(
        1.f - Intensity,                               // min
        1.f - Intensity * Smoothness,        // max
        normalizedDist                              // value
        );

    float vignetteMultiplier = 1.f - vignette;
    float3 finalColor = sceneColor * vignetteMultiplier;

    return float4(finalColor, 1.f);
}