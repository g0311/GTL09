
Texture2D SceneColorTexture : register(t0);
SamplerState LinearSampler : register(s0);

cbuffer LetterBoxCB : register(b0)
{
    float LetterBoxHeight;
    float3 Padding;
    float4 LetterBoxColor;
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

    // 뷰포트 로컬 UV 계산 [0, 1]
    float2 localUV = (input.position.xy - ViewportRect.xy) / ViewportRect.zw;

    bool isLetterbox = (localUV.y < LetterBoxHeight) ||
                       (localUV.y > (1.0 - LetterBoxHeight));

    if (isLetterbox)
    {
        // 레터박스 영역: 씬 컬러와 레터박스 컬러를 알파 블렌딩
        float3 finalColor = lerp(sceneColor, LetterBoxColor.xyz, LetterBoxColor.w);
        return float4(finalColor, 1.0);
    }
    else
    {
        return float4(sceneColor, 1.0);
    }
}