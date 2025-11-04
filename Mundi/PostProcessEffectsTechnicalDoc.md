# 포스트 프로세싱 효과 기술 문서

카메라 컴포넌트에서 사용 가능한 포스트 프로세싱 효과들에 대한 기술 문서입니다.

---

## 1. 비네팅 (Vignetting)

### 개요
화면 가장자리를 어둡게 만들어 중심부에 시선을 집중시키는 효과입니다.

### 알고리즘
- NDC 중점(0, 0)으로부터의 거리를 계산
- Intensity 기준으로 거리 비교:
    - `distance < (1.0 - Intensity)` → 0 (어두워지지 않음)
    - `distance > (1.0 - Intensity * Smoothness)` → 1 (완전히 어두움)
    - 그 사이 → S-커브 보간을 통해 부드럽게 전환 (Smoothness 값으로 제어)

### S-커브 보간 공식
```hlsl
// smoothstep 함수 사용
// t = (x - min) / (max - min)
// result = t * t * (3 - 2 * t)
float vignette = smoothstep(
    1.0 - Intensity,              // min
    1.0 - Intensity * Smoothness, // max
    normalizedDist                // value
);
```

### 셰이더 코드 (Vignetting_PS.hlsl)
```hlsl
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
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

float4 mainPS(PS_INPUT input) : SV_TARGET
{
    float3 sceneColor = SceneColorTexture.Sample(LinearSampler, input.texCoord).rgb;

    // 뷰포트 로컬 UV 계산 [0, 1]
    float2 localUV = (input.position.xy - ViewportRect.xy) / ViewportRect.zw;
    // 중심 기준 UV [-1, 1]
    float2 centeredUV = localUV * 2.0 - 1.0;

    // 중점부터의 거리
    float distance = length(centeredUV);

    // 0~1로 정규화 (대각선 길이 sqrt(2) 사용)
    float normalizedDist = distance / sqrt(2.0);

    // smoothstep을 사용한 S-커브 보간
    float vignette = smoothstep(
        1.0 - Intensity,
        1.0 - Intensity * Smoothness,
        normalizedDist
    );

    float vignetteMultiplier = 1.0 - vignette;
    float3 finalColor = sceneColor * vignetteMultiplier;

    return float4(finalColor, 1.0);
}
```

### 사용 방법
```cpp
UCameraComponent* Camera = GetCameraComponent();

// 비네팅 활성화
Camera->SetVignettingEnabled(true);

// 강도 설정 (0.0 ~ 1.0)
Camera->SetVignettingIntensity(0.5f);

// 부드러움 설정 (0.0 ~ 1.0)
Camera->SetVignettingSmoothness(0.5f);
```

---

## 2. 감마 보정 (Gamma Correction)

### 개요
색상 공간을 보정하여 모니터에서 올바른 밝기로 표시되도록 하는 효과입니다.

### 알고리즘
- 각 픽셀의 RGB 값에 대해 감마 보정 적용
- 공식: `correctedColor = pow(sceneColor, 1.0 / gamma)`
- 일반적으로 gamma 값은 2.2를 사용 (sRGB 표준)

### 감마 보정 공식
```hlsl
// Linear → sRGB 변환
float3 correctedColor = pow(sceneColor, 1.0 / GammaValue);
```

### 셰이더 코드 (GammaCorrection_PS.hlsl)
```hlsl
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
    float3 correctedColor = pow(sceneColor, 1.0 / GammaValue);

    return float4(correctedColor, 1.0);
}
```

### 사용 방법
```cpp
UCameraComponent* Camera = GetCameraComponent();

// 감마 보정 활성화
Camera->SetGammaCorrectionEnabled(true);

// 감마 값 설정 (1.0 ~ 3.0, 기본: 2.2)
Camera->SetGammaValue(2.2f);
```

---

## 3. 레터박스 (Letterbox)

### 개요
화면 상단과 하단에 검은색 띠를 추가하여 영화 같은 화면 비율을 연출하는 효과입니다.

### 알고리즘
- UV의 Y 좌표를 기준으로 상단/하단 영역 판별
- LetterboxHeight 값과 비교:
    - `Y < LetterboxHeight` → 상단 레터박스 영역 (LetterboxColor 출력)
    - `Y > (1.0 - LetterboxHeight)` → 하단 레터박스 영역 (LetterboxColor 출력)
    - 그 외 → 원본 씬 컬러 출력
- 레터박스 영역은 LetterboxColor로 출력 (기본: 검정, RGB)

### 레터박스 판별 공식
```hlsl
// 상단/하단 레터박스 영역 판별
bool isLetterbox = (uv.y < LetterboxHeight) || (uv.y > (1.0 - LetterboxHeight));

if (isLetterbox)
{
    return float4(LetterboxColor, 1.0);
}
else
{
    return float4(sceneColor, 1.0);
}
```

### 셰이더 코드 (Letterbox_PS.hlsl)
```hlsl
Texture2D SceneColorTexture : register(t0);
SamplerState LinearSampler : register(s0);

cbuffer LetterboxCB : register(b0)
{
    float LetterboxHeight;  // 0.0 ~ 0.5
    float3 LetterboxColor;  // RGB
}

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

float4 mainPS(PS_INPUT input) : SV_TARGET
{
    float3 sceneColor = SceneColorTexture.Sample(LinearSampler, input.texCoord).rgb;

    // Y 좌표가 상단 또는 하단 레터박스 영역에 있는지 확인
    bool isLetterbox = (input.texCoord.y < LetterboxHeight) ||
                       (input.texCoord.y > (1.0 - LetterboxHeight));

    if (isLetterbox)
    {
        return float4(LetterboxColor, 1.0);
    }
    else
    {
        return float4(sceneColor, 1.0);
    }
}
```

### 사용 방법
```cpp
UCameraComponent* Camera = GetCameraComponent();

// 레터박스 활성화
Camera->SetLetterboxEnabled(true);

// 레터박스 높이 설정 (0.0 ~ 0.5)
// 0.1 = 화면 상단/하단 각 10%
Camera->SetLetterboxHeight(0.1f);

// 레터박스 색상 설정 (RGB)
Camera->SetLetterboxColor(FVector(0.0f, 0.0f, 0.0f)); // 검정
```

---

## 렌더링 순서

포스트 프로세싱 효과들은 다음 순서로 적용됩니다:

1. **Fog Pass** - 높이 기반 안개 효과
2. **Vignetting Pass** - 비네팅 효과
3. **FXAA Pass** - 안티앨리어싱
4. **Gamma Correction Pass** - 감마 보정
5. **Letterbox Pass** - 레터박스 효과

```cpp
void FSceneRenderer::RenderPostProcessingPasses()
{
    RenderFogPass();
    RenderVignettingPass();
    RenderFXAAPass();
    RenderGammaCorrectionPass();
    // TODO: RenderLetterboxPass();
}
```

---

## 상수 버퍼 정의

### FVignetteBufferType
```cpp
struct FVignetteBufferType // b0
{
    float Intensity;
    float Smoothness;
    float Padding[2];
};
```

### FGammaCorrectionBufferType
```cpp
struct FGammaCorrectionBufferType // b0
{
    float GammaValue;
    float Padding[3];
};
```

### FLetterboxBufferType (TODO)
```cpp
struct FLetterboxBufferType // b0
{
    float LetterboxHeight;
    FVector LetterboxColor;
};
```

---

## 참고 사항

- 모든 포스트 프로세싱 패스는 `FSwapGuard`를 사용하여 렌더 타겟을 자동으로 스왑하고 SRV를 정리합니다.
- `FullScreenTriangle_VS.hlsl`을 버텍스 셰이더로 사용하여 전체 화면 쿼드를 그립니다.
- 멀티 뷰포트 환경에서는 `ViewportConstants` (b10)를 사용하여 뷰포트 정보를 셰이더에 전달합니다.
- Depth Test/Write는 모두 OFF 상태로 설정됩니다 (`EComparisonFunc::Always`).
