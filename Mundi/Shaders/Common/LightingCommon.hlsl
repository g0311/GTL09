//================================================================================================
// Filename:      LightingCommon.hlsl
// Description:   조명 계산 공통 함수
//                매크로를 통해 GOURAUD, LAMBERT, PHONG 조명 모델 지원
//================================================================================================

// 주의: 이 파일은 다음 파일들 이후에 include 되어야 함:
// - LightStructures.hlsl
// - LightingBuffers.hlsl

//================================================================================================
// Specular 계산 방식 선택
//================================================================================================
// USE_BLINN_PHONG 정의됨: Half-vector 기반 Blinn-Phong (더 빠르고 부드러운 하이라이트)
// USE_BLINN_PHONG 주석 처리: Reflection vector 기반 전통적인 Phong (더 날카로운 하이라이트)
#define USE_BLINN_PHONG 1

//================================================================================================
// 유틸리티 함수
//================================================================================================

// 타일 인덱스 계산 (픽셀 위치로부터)
// SV_POSITION은 픽셀 중심 좌표 (0.5, 0.5 offset)
uint CalculateTileIndex(float4 screenPos, float viewportStartX, float viewportStartY)
{
    uint localX = uint(screenPos.x) - viewportStartX;
    uint localY = uint(screenPos.y) - viewportStartY;
    
    uint tileX = localX / TileSize;
    uint tileY = localY / TileSize;
    
    return tileY * TileCountX + tileX;
}

// 타일별 라이트 인덱스 데이터의 시작 오프셋 계산
uint GetTileDataOffset(uint tileIndex)
{
    // MaxLightsPerTile = 256 (TileLightCuller.h와 일치)
    const uint MaxLightsPerTile = 256;
    return tileIndex * MaxLightsPerTile;
}

//================================================================================================
// 기본 조명 계산 함수
//================================================================================================

// Ambient Light 계산 (OBJ/MTL 표준)
// 공식: Ambient = La × Ka
// 주의: light.Color (La)는 이미 Intensity와 Temperature가 포함됨
// ambientColor: 재질의 Ambient Color (Ka) - Diffuse Color가 아님!
float3 CalculateAmbientLight(FAmbientLightInfo light, float3 ambientColor)
{
    // OBJ/MTL 표준: La × Ka
    return light.Color.rgb * ambientColor;
}

// Diffuse Light 계산 (Lambert)
// 제공된 materialColor 사용 (텍스처가 있으면 포함됨)
// 주의: lightColor는 이미 Intensity가 포함됨 (C++에서 계산)
float3 CalculateDiffuse(float3 lightDir, float3 normal, float4 lightColor, float4 materialColor)
{
    float NdotL = max(dot(normal, lightDir), 0.0f);
    // materialColor를 직접 사용 (이미 텍스처가 포함되어 있으면 포함됨)
    float3 Result = lightColor.rgb * materialColor.rgb * NdotL;
    return Result;
}

// Specular 색상 결정 매크로
// 각 셰이더에서 include 전에 이 매크로를 정의하여 커스터마이즈 가능
// 기본값: 흰색 스페큘러 (1,1,1)
#ifndef SPECULAR_COLOR
    #define SPECULAR_COLOR float3(1.0f, 1.0f, 1.0f)
#endif

// Specular Light 계산 함수
// USE_BLINN_PHONG 매크로에 따라 Blinn-Phong 또는 전통적인 Phong 방식 사용
// 주의: lightColor는 이미 Intensity가 포함됨 (C++에서 계산)
// 주의: SPECULAR_COLOR 매크로로 specular 색상 제어 (기본: 흰색)
float3 CalculateSpecular(float3 lightDir, float3 normal, float3 viewDir, float4 lightColor, float specularPower)
{
    float3 specularColor = SPECULAR_COLOR;

#ifdef USE_BLINN_PHONG
    // Blinn-Phong 방식: Half-vector 기반 (더 빠르고 부드러운 하이라이트)
    float3 halfVec = normalize(lightDir + viewDir);
    float NdotH = max(dot(normal, halfVec), 0.0f);
    float specular = pow(NdotH, specularPower);
    return lightColor.rgb * specularColor * specular;
#else
    // 전통적인 Phong 방식: Reflection vector 기반 (더 날카로운 하이라이트)
    float3 reflectDir = reflect(-lightDir, normal);
    float RdotV = max(dot(reflectDir, viewDir), 0.0f);
    float specular = pow(RdotV, specularPower);
    return lightColor.rgb * specularColor * specular;
#endif
}

//================================================================================================
// 감쇠(Attenuation) 함수
//================================================================================================

// Unreal Engine: Inverse Square Falloff (물리 기반)
// https://docs.unrealengine.com/en-US/BuildingWorlds/LightingAndShadows/PhysicalLightUnits/
float CalculateInverseSquareFalloff(float distance, float attenuationRadius)
{
    // Unreal Engine의 역제곱 감쇠 with 부드러운 윈도우 함수
    float distanceSq = distance * distance;
    float radiusSq = attenuationRadius * attenuationRadius;

    // 기본 역제곱 법칙: I = 1 / (distance^2)
    float basicFalloff = 1.0f / max(distanceSq, 0.01f * 0.01f); // 0으로 나누기 방지

    // attenuationRadius에서 0에 도달하는 부드러운 윈도우 함수 적용
    // 반경 경계에서 급격한 차단을 방지
    float distanceRatio = saturate(distance / attenuationRadius);
    float windowAttenuation = pow(1.0f - pow(distanceRatio, 4.0f), 2.0f);

    return basicFalloff * windowAttenuation;
}

// Unreal Engine: Light Falloff Exponent (예술적 제어)
// 물리적으로 정확하지 않지만 예술적 제어 제공
float CalculateExponentFalloff(float distance, float attenuationRadius, float falloffExponent)
{
    // 정규화된 거리 (라이트 중심에서 0, 반경 경계에서 1)
    float distanceRatio = saturate(distance / attenuationRadius);

    // 단순 지수 기반 감쇠: attenuation = (1 - ratio)^exponent
    // falloffExponent가 곡선을 제어:
    // - exponent = 1: 선형 감쇠
    // - exponent > 1: 더 빠른 감쇠 (더 날카로움)
    // - exponent < 1: 더 느린 감쇠 (더 부드러움)
    float attenuation = pow(1.0f - pow(distanceRatio, 2.0f), max(falloffExponent, 0.1f));

    return attenuation;
}


//================================================================================================
// 큐브맵 UV 변환 헬퍼 함수
//================================================================================================

// 큐브맵 방향 벡터를 UV 좌표로 변환
struct CubemapUV
{
    float2 uv;        // [0,1] 범위 UV
    int faceIndex;    // 0=+X, 1=-X, 2=+Y, 3=-Y, 4=+Z, 5=-Z
};

CubemapUV DirectionToCubemapUV(float3 dir)
{
    CubemapUV result;
    
    float3 absDir = abs(dir);
    float maxAxis = max(absDir.x, max(absDir.y, absDir.z));
    
    float2 uv;
    
    if (maxAxis == absDir.x)
    {
        result.faceIndex = dir.x > 0 ? 0 : 1;
        uv = float2(-sign(dir.x) * dir.z, -dir.y) / absDir.x;
    }
    else if (maxAxis == absDir.y)
    {
        result.faceIndex = dir.y > 0 ? 2 : 3;
        uv = float2(dir.x, sign(dir.y) * dir.z) / absDir.y;
    }
    else
    {
        result.faceIndex = dir.z > 0 ? 4 : 5;
        uv = float2(sign(dir.z) * dir.x, -dir.y) / absDir.z;
    }
    
    result.uv = uv * 0.5f + 0.5f;  // [-1,1] -> [0,1]
    return result;
}

// UV 좌표를 큐브맵 방향 벡터로 변환
float3 CubemapUVToDirection(float2 uv, int faceIndex)
{
    float2 ndc = uv * 2.0f - 1.0f;  // [0,1] -> [-1,1]
    
    float3 dir;
    
    if (faceIndex == 0)       // +X
        dir = float3(1.0f, -ndc.y, -ndc.x);
    else if (faceIndex == 1)  // -X
        dir = float3(-1.0f, -ndc.y, ndc.x);
    else if (faceIndex == 2)  // +Y
        dir = float3(ndc.x, 1.0f, ndc.y);
    else if (faceIndex == 3)  // -Y
        dir = float3(ndc.x, -1.0f, -ndc.y);
    else if (faceIndex == 4)  // +Z
        dir = float3(ndc.x, -ndc.y, 1.0f);
    else                      // -Z
        dir = float3(-ndc.x, -ndc.y, -1.0f);
    
    return normalize(dir);
}

//================================================================================================
// 쉐도우 샘플링 함수
//================================================================================================

// VSM Moments 계산 (Chebyshev's inequality)
float ComputeVSMShadow(float PixelDepth, float2 Moments)
{
    // E(z), μ
    float Mean = Moments.x;
    
    // 픽셀이 평균보다 앞에 있으면 완전히 밝음
    if (PixelDepth <= Mean)
        return 1.0f;
    
    // σ² = E(z²) - E(z)²
    float Variance = Moments.y - (Moments.x * Moments.x);
    float MinVariance = 0.00001f;
    Variance = max(Variance, MinVariance);
    
    // Chebyshev's inequality
    float DepthDelta = PixelDepth - Mean;
    float LitFactor = Variance / (Variance + (DepthDelta * DepthDelta));
    
    return saturate(LitFactor);
}

// Poisson Disk 샘플 패턴 (16 samples)
static const float2 PoissonDisk[16] = {
    float2(0.540417, 0.640249), float2(0.160918, 0.899496),
    float2(-0.291771, 0.575997), float2(-0.737101, 0.504261),
    float2(-0.852436, -0.063901), float2(-0.536979, -0.580749),
    float2(-0.198121, -0.835976), float2(0.284752, -0.730335),
    float2(0.697495, -0.537658), float2(0.887834, -0.161042),
    float2(0.818454, 0.334645), float2(0.383842, 0.279867),
    float2(-0.103009, 0.134190), float2(-0.485496, 0.106886),
    float2(-0.354784, -0.320412), float2(0.166412, -0.244837)
};

//================================================================================================
// 2D Shadow Map Sampling (Directional/Spot Light)
//================================================================================================

// PCF for 2D shadow maps
float SampleShadow2D_PCF(float PixelDepth, float2 AtlasUV, int SampleCount, float2 FilterRadiusUV,
    Texture2D<float2> ShadowMap, SamplerState ShadowSampler)
{
    if (SampleCount <= 0)
    {
        float2 moments = ShadowMap.SampleLevel(ShadowSampler, AtlasUV, 0);
        return (PixelDepth <= moments.x) ? 1.0f : 0.0f;
    }
    
    float ShadowSum = 0.0f;
    
    [loop]
    for (int i = 0; i < SampleCount; i++)
    {
        float2 Offset = PoissonDisk[i] * FilterRadiusUV;
        float2 SampleUV = AtlasUV + Offset;
        
        float2 moments = ShadowMap.SampleLevel(ShadowSampler, SampleUV, 0);
        ShadowSum += (PixelDepth <= moments.x) ? 1.0f : 0.0f;
    }
    
    return ShadowSum / float(SampleCount);
}

// VSM for 2D shadow maps
float SampleShadow2D_VSM(float PixelDepth, float2 AtlasUV,
    Texture2D<float2> ShadowMap, SamplerState ShadowSampler)
{
    float2 Moments = ShadowMap.SampleLevel(ShadowSampler, AtlasUV, 0);
    return ComputeVSMShadow(PixelDepth, Moments);
}

//================================================================================================
// Cubemap Shadow Map Sampling (Point Light)
//================================================================================================

// PCF for cubemap shadow maps
float SampleShadowCube_PCF(float PixelDepth, float3 CubemapDir, uint LightIndex,
    int SampleCount, float FilterRadiusTexel,
    TextureCubeArray<float2> ShadowMapCube, SamplerState ShadowSampler)
{
    if (SampleCount <= 0)
    {
        float2 moments = ShadowMapCube.SampleLevel(ShadowSampler, float4(CubemapDir, LightIndex), 0);
        return (PixelDepth <= moments.x) ? 1.0f : 0.0f;
    }
    
    float ShadowSum = 0.0f;
    CubemapUV baseUV = DirectionToCubemapUV(CubemapDir);
    
    [loop]
    for (int i = 0; i < SampleCount; i++)
    {
        float2 offsetUV = baseUV.uv + PoissonDisk[i] * FilterRadiusTexel;
        float3 sampleDir = CubemapUVToDirection(offsetUV, baseUV.faceIndex);
        
        float2 moments = ShadowMapCube.SampleLevel(ShadowSampler, float4(sampleDir, LightIndex), 0);
        ShadowSum += (PixelDepth <= moments.x) ? 1.0f : 0.0f;
    }
    
    return ShadowSum / float(SampleCount);
}

// VSM for cubemap shadow maps
float SampleShadowCube_VSM(float PixelDepth, float3 CubemapDir, uint LightIndex,
    TextureCubeArray<float2> ShadowMapCube, SamplerState ShadowSampler)
{
    float2 Moments = ShadowMapCube.SampleLevel(ShadowSampler, float4(CubemapDir, LightIndex), 0);
    return ComputeVSMShadow(PixelDepth, Moments);
}

//================================================================================================
// Spot Light Shadow (PCF)
//================================================================================================
float CalculateSpotLightShadowFactor(
    float3 WorldPos, FShadowMapData ShadowMapData, Texture2D<float2> ShadowMap, SamplerState ShadowSampler)
{
    float4 ShadowTexCoord = mul(float4(WorldPos, 1.0f), ShadowMapData.ShadowViewProjMatrix);
    ShadowTexCoord.xyz /= ShadowTexCoord.w;
    float2 AtlasUV = ShadowTexCoord.xy;

    if (saturate(AtlasUV.x) == AtlasUV.x && saturate(AtlasUV.y) == AtlasUV.y)
    {
        AtlasUV = AtlasUV * ShadowMapData.AtlasScaleOffset.xy + ShadowMapData.AtlasScaleOffset.zw;
        
        // World space distance 기반 깊이
        float distance = length(WorldPos - ShadowMapData.WorldPos);
        float PixelDepth = saturate(distance / ShadowMapData.Radius) - 0.001f;
        
        float Width, Height;
        ShadowMap.GetDimensions(Width, Height);
        float2 FilterRadiusUV = 1.5f / float2(Width, Height);
        
        return SampleShadow2D_PCF(PixelDepth, AtlasUV, ShadowMapData.SampleCount, FilterRadiusUV, ShadowMap, ShadowSampler);
    }

    return 1.0f;
}

//================================================================================================
// Spot Light Shadow (VSM)
//================================================================================================
float CalculateSpotLightShadowFactorVSM(
    float3 WorldPos, FShadowMapData ShadowMapData, Texture2D<float2> VShadowMap, SamplerState ShadowSampler)
{
    float4 ShadowTexCoord = mul(float4(WorldPos, 1.0f), ShadowMapData.ShadowViewProjMatrix);
    ShadowTexCoord.xyz /= ShadowTexCoord.w;
    float2 AtlasUV = ShadowTexCoord.xy;

    if (saturate(AtlasUV.x) == AtlasUV.x && saturate(AtlasUV.y) == AtlasUV.y)
    {
        AtlasUV = AtlasUV * ShadowMapData.AtlasScaleOffset.xy + ShadowMapData.AtlasScaleOffset.zw;
        
        // World space distance 기반 깊이
        float distance = length(WorldPos - ShadowMapData.WorldPos);
        float PixelDepth = saturate(distance / ShadowMapData.Radius) - 0.001f;
        
        return SampleShadow2D_VSM(PixelDepth, AtlasUV, VShadowMap, ShadowSampler);
    }

    return 1.0f;
}

//================================================================================================
// Point Light Shadow (PCF)
//================================================================================================
float CalculatePointLightShadowFactor(
    float3 WorldPos, float3 LightPos, float FarPlane, uint LightIndex, int SampleCount,
    TextureCubeArray<float2> ShadowMapCube, SamplerState ShadowSampler)
{
    float3 lightToPixel = WorldPos - LightPos;
    float distance = length(lightToPixel);
    float PixelDepth = saturate(distance / FarPlane) - 0.001f;
    
    // 좌표계 변환
    float3 cubemapDir = float3(lightToPixel.y, lightToPixel.z, lightToPixel.x);
    
    float Width, Height, Elements;
    ShadowMapCube.GetDimensions(Width, Height, Elements);
    float filterRadiusTexel = 1.5f / Width;
    
    return SampleShadowCube_PCF(PixelDepth, cubemapDir, LightIndex, SampleCount, filterRadiusTexel, ShadowMapCube, ShadowSampler);
}

//================================================================================================
// Point Light Shadow (VSM)
//================================================================================================
float CalculatePointLightShadowFactorVSM(
    float3 WorldPos, float3 LightPos, float FarPlane, uint LightIndex,
    TextureCubeArray<float2> ShadowMapCube, SamplerState ShadowSampler)
{
    float3 lightToPixel = WorldPos - LightPos;
    float distance = length(lightToPixel);
    float PixelDepth = saturate(distance / FarPlane) - 0.001f;
    
    // 좌표계 변환
    float3 cubemapDir = float3(lightToPixel.y, lightToPixel.z, lightToPixel.x);
    
    return SampleShadowCube_VSM(PixelDepth, cubemapDir, LightIndex, ShadowMapCube, ShadowSampler);
}

//================================================================================================
// Cascaded Shadow Maps (Directional Light) - PCF
//================================================================================================
float GetCascadedShadowAtt(float3 WorldPos, float3 ViewPos, Texture2D<float2> ShadowMap, SamplerState ShadowSampler)
{
    int CurIdx = 0;
    bool bNeedLerp = false;
    float LerpValue = 0.0f;
    
    float Width, Height;
    ShadowMap.GetDimensions(Width, Height);
    float2 FilterRadiusUV = 1.5f / float2(Width, Height);
    
    // 카메라 위치 추출
    float3 CameraPos = float3(InverseViewMatrix[3][0], InverseViewMatrix[3][1], InverseViewMatrix[3][2]);
    float DistanceFromCamera = length(WorldPos - CameraPos);
    
    // Debug mode: 특정 cascade만 렌더링
    if (DirectionalLight.CascadedAreaShadowDebugValue != -1)
    {
        CurIdx = DirectionalLight.CascadedAreaShadowDebugValue;
        float3 CurUV = mul(float4(WorldPos, 1), DirectionalLight.Cascades[CurIdx].ShadowViewProjMatrix).xyz;
        
        if (saturate(CurUV.x) == CurUV.x && saturate(CurUV.y) == CurUV.y)
        {
            float2 CurAtlasUV = CurUV.xy * DirectionalLight.Cascades[CurIdx].AtlasScaleOffset.xy + DirectionalLight.Cascades[CurIdx].AtlasScaleOffset.zw;
            float PixelDepth = CurUV.z - 0.001f; // Orthographic projection Z는 이미 선형
            
            return SampleShadow2D_PCF(PixelDepth, CurAtlasUV, DirectionalLight.Cascades[CurIdx].SampleCount, FilterRadiusUV, ShadowMap, ShadowSampler);
        }
        return 0.0f;
    }
    
    // Cascade 선택 (world space 거리 기반)
    for (uint i = 0; i < DirectionalLight.CascadeCount; i++)
    {
        float CurFar = DirectionalLight.CascadedSliceDepth[(i + 1) / 4][(i + 1) % 4];
        if (DistanceFromCamera < CurFar)
        {
            CurIdx = i;
            break;
        }
    }
    
    // Cascade 블렌딩 체크
    float PrevFar = CurIdx == 0 ? 0 : DirectionalLight.CascadedSliceDepth[CurIdx / 4][CurIdx % 4];
    float ExtensionPrevFar = PrevFar + PrevFar * DirectionalLight.CascadedOverlapValue;
    if (CurIdx > 0 && DistanceFromCamera < ExtensionPrevFar)
    {
        bNeedLerp = true;
        LerpValue = (DistanceFromCamera - PrevFar) / (ExtensionPrevFar - PrevFar);
    }
    
    // 현재 cascade shadow 샘플링
    float3 CurUV = mul(float4(WorldPos, 1), DirectionalLight.Cascades[CurIdx].ShadowViewProjMatrix).xyz;
    float2 CurAtlasUV = CurUV.xy * DirectionalLight.Cascades[CurIdx].AtlasScaleOffset.xy + DirectionalLight.Cascades[CurIdx].AtlasScaleOffset.zw;
    float CurPixelDepth = CurUV.z - 0.001f;
    float CurShadowFactor = SampleShadow2D_PCF(CurPixelDepth, CurAtlasUV, DirectionalLight.Cascades[CurIdx].SampleCount, FilterRadiusUV, ShadowMap, ShadowSampler);
    
    // Cascade 블렌딩
    if (bNeedLerp)
    {
        int PrevIdx = CurIdx - 1;
        float3 PrevUV = mul(float4(WorldPos, 1), DirectionalLight.Cascades[PrevIdx].ShadowViewProjMatrix).xyz;
        float2 PrevAtlasUV = PrevUV.xy * DirectionalLight.Cascades[PrevIdx].AtlasScaleOffset.xy + DirectionalLight.Cascades[PrevIdx].AtlasScaleOffset.zw;
        float PrevPixelDepth = PrevUV.z - 0.001f;
        float PrevShadowFactor = SampleShadow2D_PCF(PrevPixelDepth, PrevAtlasUV, DirectionalLight.Cascades[PrevIdx].SampleCount, FilterRadiusUV, ShadowMap, ShadowSampler);
        
        return lerp(PrevShadowFactor, CurShadowFactor, LerpValue);
    }
    
    return CurShadowFactor;
}
//================================================================================================
// 통합 조명 계산 함수
//================================================================================================

// Directional Light 계산 (Diffuse + Specular)
float3 CalculateDirectionalLight
    (FDirectionalLightInfo light, float3 WorldPos, float3 ViewPos, float3 normal, float3 viewDir, float4 materialColor, bool includeSpecular, float specularPower,
 Texture2D<float2> ShadowMap, SamplerState ShadowSampler, Texture2D<float2> VShadowMap, SamplerState VShadowSampler)
{
    // Light.Direction이 영벡터인 경우, 정규화 문제 방지
    if (all(light.Direction == float3(0.0f, 0.0f, 0.0f)))
    {
        return float3(0.0f, 0.0f, 0.0f);
    }
    
    float3 lightDir = normalize(-light.Direction);

    // Diffuse (light.Color는 이미 Intensity 포함)
    float3 diffuse = CalculateDiffuse(lightDir, normal, light.Color, materialColor);

    // Specular (선택사항)
    float3 specular = float3(0.0f, 0.0f, 0.0f);
    if (includeSpecular)
    {
        specular = CalculateSpecular(lightDir, normal, viewDir, light.Color, specularPower);
    }

    // Shadow 적용
    float shadowFactor = 1.0f;
    if (light.bCastShadows)
    {
        if (light.bCascaded)
        {
            // CSM은 PCF만 지원
            shadowFactor = GetCascadedShadowAtt(WorldPos, ViewPos, ShadowMap, ShadowSampler);
        }
        else
        {
            // Non-cascaded directional light (orthographic projection)
            float3 ShadowUV = mul(float4(WorldPos, 1), light.Cascades[0].ShadowViewProjMatrix).xyz;
            float2 AtlasUV = ShadowUV.xy * light.Cascades[0].AtlasScaleOffset.xy + light.Cascades[0].AtlasScaleOffset.zw;
            float PixelDepth = ShadowUV.z - 0.001f;
            
#if SHADOW_AA_TECHNIQUE == 1
            float Width, Height;
            ShadowMap.GetDimensions(Width, Height);
            float2 FilterRadiusUV = 1.5f / float2(Width, Height);
            shadowFactor = SampleShadow2D_PCF(PixelDepth, AtlasUV, light.Cascades[0].SampleCount, FilterRadiusUV, ShadowMap, ShadowSampler);
#elif SHADOW_AA_TECHNIQUE == 2
            shadowFactor = SampleShadow2D_VSM(PixelDepth, AtlasUV, VShadowMap, VShadowSampler);
#endif
        }
    }

    return (diffuse + specular) * shadowFactor;
}

// Spot Light 계산 (Diffuse + Specular with Attenuation and Cone)
float3 CalculateSpotLight(
    FSpotLightInfo light, float3 worldPos, float3 normal, float3 viewDir, float4 materialColor, bool includeSpecular, float specularPower,
    Texture2D<float2> ShadowMap, SamplerState ShadowSampler, Texture2D<float2> VShadowMap, SamplerState VShadowSampler)
{
    float3 lightVec = light.Position - worldPos;
    float distance = length(lightVec);

    // epsilon으로 0으로 나누기 방지
    distance = max(distance, 0.0001f);
    float3 lightDir = lightVec / distance;
    float3 spotDir = normalize(light.Direction);

    // Spot 원뿔 감쇠 (각도 공간에서 보간하여 중간에서 급격하게 변함)
    float cosAngle = dot(-lightDir, spotDir);

    // cosine 공간 대신 각도 공간에서 보간 (균등한 감쇠를 위해)
    float angle = degrees(acos(saturate(cosAngle)));  // 현재 각도 (도 단위)

    // inner cone에서 1, outer cone에서 0, 중간에서 급격하게 변함
    float spotAttenuation = 1.0 - smoothstep(light.InnerConeAngle, light.OuterConeAngle, angle);

    // Falloff 모드에 따라 거리 감쇠 계산
    float distanceAttenuation;
    if (light.bUseInverseSquareFalloff)
    {
        // 물리 기반 역제곱 감쇠 (Unreal Engine 스타일)
        distanceAttenuation = CalculateInverseSquareFalloff(distance, light.AttenuationRadius);
    }
    else
    {
        // 더 큰 제어를 위한 예술적 지수 기반 감쇠
        distanceAttenuation = CalculateExponentFalloff(distance, light.AttenuationRadius, light.FalloffExponent);
    }

    // 두 감쇠를 결합
    float attenuation = distanceAttenuation * spotAttenuation;

    // Diffuse (light.Color는 이미 Intensity 포함)
    float3 diffuse = CalculateDiffuse(lightDir, normal, light.Color, materialColor) * attenuation;

    // Specular (선택사항)
    float3 specular = float3(0.0f, 0.0f, 0.0f);
    if (includeSpecular)
    {
        specular = CalculateSpecular(lightDir, normal, viewDir, light.Color, specularPower) * attenuation;
    }

    // Shadow 적용
    if (light.bCastShadows)
    {
#if SHADOW_AA_TECHNIQUE == 1
        float shadowFactor = CalculateSpotLightShadowFactor(worldPos, light.ShadowData, ShadowMap, ShadowSampler);
        diffuse *= shadowFactor;
        specular *= shadowFactor;
#elif SHADOW_AA_TECHNIQUE == 2
        float shadowFactor = CalculateSpotLightShadowFactorVSM(worldPos, light.ShadowData, VShadowMap, VShadowSampler);
        diffuse *= shadowFactor;
        specular *= shadowFactor;
#endif
    }

    return diffuse + specular;
}

// Point Light 계산 (Diffuse + Specular with Attenuation and Falloff)
float3 CalculatePointLight(
    FPointLightInfo light, float3 worldPos, float3 normal, float3 viewDir, float4 materialColor, bool includeSpecular, float specularPower,
    TextureCubeArray<float2> ShadowMapCube, SamplerState ShadowSampler, TextureCubeArray<float2> VShadowMapCube, SamplerState VShadowSampler)
{
    float3 lightVec = light.Position - worldPos;
    float distance = length(lightVec);

    // epsilon으로 0으로 나누기 방지
    distance = max(distance, 0.0001f);
    float3 lightDir = lightVec / distance;

    // Falloff 모드에 따라 감쇠 계산
    float attenuation;
    if (light.bUseInverseSquareFalloff)
    {
        // 물리 기반 역제곱 감쇠 (Unreal Engine 스타일)
        attenuation = CalculateInverseSquareFalloff(distance, light.AttenuationRadius);
    }
    else
    {
        // 더 큰 제어를 위한 예술적 지수 기반 감쇠
        attenuation = CalculateExponentFalloff(distance, light.AttenuationRadius, light.FalloffExponent);
    }

    // Diffuse (light.Color는 이미 Intensity 포함)
    float3 diffuse = CalculateDiffuse(lightDir, normal, light.Color, materialColor) * attenuation;

    // Specular (선택사항)
    float3 specular = float3(0.0f, 0.0f, 0.0f);
    if (includeSpecular)
    {
        specular = CalculateSpecular(lightDir, normal, viewDir, light.Color, specularPower) * attenuation;
    }

    // Shadow 적용
    if (light.bCastShadows)
    {
#if SHADOW_AA_TECHNIQUE == 1
        int sampleCount = 4;
        float shadowFactor = CalculatePointLightShadowFactor(
            worldPos, light.Position, light.AttenuationRadius, light.LightIndex, sampleCount,
            ShadowMapCube, ShadowSampler);
        diffuse *= shadowFactor;
        specular *= shadowFactor;
#elif SHADOW_AA_TECHNIQUE == 2
        float shadowFactor = CalculatePointLightShadowFactorVSM(
            worldPos, light.Position, light.AttenuationRadius, light.LightIndex,
            VShadowMapCube, VShadowSampler);
        diffuse *= shadowFactor;
        specular *= shadowFactor;
#endif
    }

    return diffuse + specular;
}

//================================================================================================
// 매크로 기반 조명 모델 헬퍼
//================================================================================================

// 조명 모델에 따라 적절한 파라미터 자동 설정
#ifdef LIGHTING_MODEL_GOURAUD
    #define LIGHTING_INCLUDE_SPECULAR true
    #define LIGHTING_NEED_VIEWDIR true
#elif defined(LIGHTING_MODEL_LAMBERT)
    #define LIGHTING_INCLUDE_SPECULAR false
    #define LIGHTING_NEED_VIEWDIR false
#elif defined(LIGHTING_MODEL_PHONG)
    #define LIGHTING_INCLUDE_SPECULAR true
    #define LIGHTING_NEED_VIEWDIR true
#else
    // 조명 모델 미정의 - 기본값 제공
    #define LIGHTING_INCLUDE_SPECULAR false
    #define LIGHTING_NEED_VIEWDIR false
#endif

//================================================================================================
// 타일 컬링 기반 전체 조명 계산 (매크로 자동 대응)
//================================================================================================

// 모든 라이트(Point + Spot) 계산 - 타일 컬링 지원
// 매크로에 따라 자동으로 specular on/off
// 주의: 이 헬퍼 함수는 ambient에 baseColor 사용 (Ka = Kd 가정)
//       OBJ/MTL 재질의 경우, Material.AmbientColor로 CalculateAmbientLight를 별도로 호출할 것
float3 CalculateAllLights(
    float3 worldPos,
    float3 viewPos,
    float3 normal,
    float3 viewDir,
    float4 baseColor,
    float specularPower,
    float4 screenPos,
    SamplerState ShadowSampler,
    Texture2D<float2> ShadowMap2D,
    TextureCubeArray<float2> ShadowMapCube,
    TextureCubeArray<float2> VShadowMapCube,
    Texture2D<float2> VSMShadowMap,
    SamplerState VSMSampler
    )
{
    float3 litColor = float3(0, 0, 0);

    // Ambient (비재질 오브젝트는 Ka = Kd 가정)
    litColor += CalculateAmbientLight(AmbientLight, baseColor.rgb);

    litColor += CalculateDirectionalLight(
        DirectionalLight,
        worldPos,
        viewPos,
        normal,
        viewDir,
        baseColor,
        LIGHTING_INCLUDE_SPECULAR,
        specularPower,
        ShadowMap2D, ShadowSampler,
        VSMShadowMap, VSMSampler
    );

    // Point + Spot with 타일 컬링
    if (bUseTileCulling)
    {
        uint tileIndex = CalculateTileIndex(screenPos, ViewportStartX, ViewportStartY);
        uint tileDataOffset = GetTileDataOffset(tileIndex);
        uint lightCount = g_TileLightIndices[tileDataOffset];

        for (uint i = 0; i < lightCount; i++)
        {
            uint packedIndex = g_TileLightIndices[tileDataOffset + 1 + i];
            uint lightType = (packedIndex >> 16) & 0xFFFF;
            uint lightIdx = packedIndex & 0xFFFF;

            if (lightType == 0)  // Point Light
            {
                litColor += CalculatePointLight(
                    g_PointLightList[lightIdx],
                    worldPos,
                    normal,
                    viewDir,
                    baseColor,
                    LIGHTING_INCLUDE_SPECULAR,
                    specularPower,
                    ShadowMapCube, ShadowSampler,
                    VShadowMapCube, VSMSampler
                );
            }
            else if (lightType == 1)  // Spot Light
            {
                litColor += CalculateSpotLight(
                    g_SpotLightList[lightIdx],
                    worldPos,
                    normal,
                    viewDir,
                    baseColor,
                    LIGHTING_INCLUDE_SPECULAR,  // 매크로 자동 대응
                    specularPower,
                    ShadowMap2D, ShadowSampler,
                    VSMShadowMap, VSMSampler
                );
            }
        }
    }
    else
    {
        // 모든 라이트 순회 (타일 컴링 비활성화)
        for (uint i = 0; i < PointLightCount; i++)
        {
            litColor += CalculatePointLight(
                g_PointLightList[i],
                worldPos,
                normal,
                viewDir,
                baseColor,
                LIGHTING_INCLUDE_SPECULAR,
                specularPower,
                ShadowMapCube, ShadowSampler,
                VShadowMapCube, VSMSampler
            );
        }

        for (uint j = 0; j < SpotLightCount; j++)
        {
            litColor += CalculateSpotLight(
                    g_SpotLightList[j],
                    worldPos,
                    normal,
                    viewDir,
                    baseColor,
                    LIGHTING_INCLUDE_SPECULAR,  // 매크로 자동 대응
                    specularPower,
                    ShadowMap2D, ShadowSampler,
                    VSMShadowMap, VSMSampler
                );
        }
    }

    return litColor;
}