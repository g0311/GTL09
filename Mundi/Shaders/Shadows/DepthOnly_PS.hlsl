// b1: ViewProjBuffer (VS) - ViewProjBufferType과 일치
cbuffer ViewProjBuffer : register(b1)
{
    row_major float4x4 ViewMatrix;
    row_major float4x4 ProjectionMatrix;
    row_major float4x4 InverseViewMatrix;   // 0행 광원의 월드 좌표 + 스포트라이트 반경
    row_major float4x4 InverseProjectionMatrix;
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float3 WorldPosition : TEXCOORD0;
};

float2 mainPS(PS_INPUT Input) : SV_TARGET
{
    float LightRadius = InverseViewMatrix[0][3];
    float NormalizedDepth;
    
    // Directional Light (Radius == -1): NDC Z 사용 (Orthographic은 선형)
    if (LightRadius < 0.0f)
    {
        NormalizedDepth = Input.Position.z;
    }
    else // Point/Spot Light: World Space Distance
    {
        float3 LightPos = InverseViewMatrix[0].xyz;
        float distance = length(Input.WorldPosition - LightPos);
        NormalizedDepth = saturate(distance / LightRadius);
    }
    
    // VSM을 위한 moments 계산
    float Moment1 = NormalizedDepth;
    float Moment2 = NormalizedDepth * NormalizedDepth;

    return float2(Moment1, Moment2);
}
