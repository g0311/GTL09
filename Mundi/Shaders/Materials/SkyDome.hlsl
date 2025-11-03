//================================================================================================
// Filename:      SkyDome.hlsl
// Description:   스카이돔 렌더링을 위한 전용 셰이더
//                - 조명 계산 없음 (Unlit)
//                - 깊이를 항상 최대값(1.0)으로 설정하여 모든 객체 뒤에 렌더링
//                - 카메라 이동에 따른 시차 효과 제거 (회전만 유지)
//================================================================================================

// --- 상수 버퍼 ---
// b0: ModelBuffer (VS)
cbuffer ModelBuffer : register(b0)
{
    row_major float4x4 WorldMatrix;              // 64 bytes
    row_major float4x4 WorldInverseTranspose;    // 64 bytes
};

// b1: ViewProjBuffer (VS)
cbuffer ViewProjBuffer : register(b1)
{
    row_major float4x4 ViewMatrix;
    row_major float4x4 ProjectionMatrix;
    row_major float4x4 InverseViewMatrix;
    row_major float4x4 InverseProjectionMatrix;
};

// b3: ColorBuffer (PS)
cbuffer ColorBuffer : register(b3)
{
    float4 LerpColor;   // 색상 블렌딩용
    uint UUID;
};

// --- 텍스처 및 샘플러 ---
Texture2D g_SkyTexture : register(t0);
SamplerState g_Sample : register(s0);

// --- 입출력 구조체 ---
struct VS_INPUT
{
    float3 Position : POSITION;
    float3 Normal : NORMAL0;
    float2 TexCoord : TEXCOORD0;
    float4 Tangent : TANGENT0;
    float4 Color : COLOR;
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
    float3 WorldPos : POSITION;  // 디버깅용
};

struct PS_OUTPUT
{
    float4 Color : SV_Target0;
    uint UUID : SV_Target1;
};

//================================================================================================
// 버텍스 셰이더 (Vertex Shader)
//================================================================================================
PS_INPUT mainVS(VS_INPUT Input)
{
    PS_INPUT Out;

    // 1. 로컬 위치를 월드 공간으로 변환
    float4 worldPos = mul(float4(Input.Position, 1.0f), WorldMatrix);
    Out.WorldPos = worldPos.xyz;

    // 2. 뷰 변환 (카메라 회전만 적용, 이동 제거)
    // ViewMatrix에서 회전 성분만 추출 (3x3 행렬) + 이동 성분 제거
    row_major float4x4 viewRotationOnly = ViewMatrix;
    viewRotationOnly[3][0] = 0.0f;  // x 이동 제거
    viewRotationOnly[3][1] = 0.0f;  // y 이동 제거
    viewRotationOnly[3][2] = 0.0f;  // z 이동 제거

    float4 viewPos = mul(worldPos, viewRotationOnly);

    // 3. 투영 변환
    float4 projPos = mul(viewPos, ProjectionMatrix);

    // 4. ⭐ 깊이를 최대값으로 설정 (z = w)
    // 이렇게 하면 depth test 후 깊이 버퍼에 1.0이 기록됨
    // 모든 불투명 객체가 스카이 앞에 그려지도록 보장
    Out.Position = projPos.xyww;  // z를 w로 대체 (z/w = 1.0)

    // 5. 텍스처 좌표 전달
    Out.TexCoord = Input.TexCoord;

    return Out;
}

//================================================================================================
// 픽셀 셰이더 (Pixel Shader)
//================================================================================================
PS_OUTPUT mainPS(PS_INPUT Input)
{
    PS_OUTPUT Output;

    // 텍스처 샘플링 (조명 계산 없음)
    float4 skyColor = g_SkyTexture.Sample(g_Sample, Input.TexCoord);

    Output.Color = skyColor;
    Output.UUID = UUID;

    return Output;
}
