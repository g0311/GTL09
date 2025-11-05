# 포스트 프로세싱 효과 기술 문서

### 비네팅

- NDC 중점과의 거리 비교를 통해 계산
- Intensity 보다
    - 가깝다 → 0
    - 멀다 → 1
    - 그 사이다 → S-커브 보간을 통해 수행 (Smoothness 곱해서 적용)
- S-커브 공식
    - `t = (x - min) / (max - min)`
    - `result = t * t * (3 - 2 * t)`
- 핵심 코드
```hlsl
float2 localUV = (input.position.xy - ViewportRect.xy) / ViewportRect.zw;
float2 centeredUV = localUV * 2.0 - 1.0;
float distance = length(centeredUV);
float normalizedDist = distance / sqrt(2.0);

float vignette = smoothstep(
    1.0 - Intensity,
    1.0 - Intensity * Smoothness,
    normalizedDist
);

float vignetteMultiplier = 1.0 - vignette;
float3 finalColor = sceneColor * vignetteMultiplier;
```

### 감마 보정

- Linear 색상 공간 → sRGB 색상 공간 변환
- 각 픽셀의 RGB 값에 대해 감마 보정 적용
- 공식
    - `correctedColor = pow(sceneColor, 1.0 / gamma)`
- 일반적으로 gamma 값은 2.2 사용 (sRGB 표준)
- 핵심 코드
```hlsl
float3 sceneColor = SceneColorTexture.Sample(LinearSampler, input.texCoord).rgb;
float3 correctedColor = pow(sceneColor, 1.0 / GammaValue);
return float4(correctedColor, 1.0);
```

### 레터박스

- 뷰포트 로컬 UV의 Y 좌표를 기준으로 상단/하단 영역 판별
- LetterboxHeight 값과 비교
    - `localUV.y < LetterboxHeight` → 상단 레터박스 영역 (LetterboxColor 출력)
    - `localUV.y > (1.0 - LetterboxHeight)` → 하단 레터박스 영역 (LetterboxColor 출력)
    - 그 외 → 원본 씬 컬러 출력
- 레터박스 영역은 LetterboxColor로 출력 (기본: 검정, RGB)
- 핵심 코드
```hlsl
float3 sceneColor = SceneColorTexture.Sample(LinearSampler, input.texCoord).rgb;

// 뷰포트 로컬 UV 계산 [0, 1]
float2 localUV = (input.position.xy - ViewportRect.xy) / ViewportRect.zw;

bool isLetterbox = (localUV.y < LetterboxHeight) ||
                   (localUV.y > (1.0 - LetterboxHeight));

if (isLetterbox)
{
    return float4(LetterboxColor, 1.0);
}
else
{
    return float4(sceneColor, 1.0);
}
```
