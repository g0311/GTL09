#include "pch.h"
#include "CameraModifierVignette.h"
#include "PostProcessSettings.h"

IMPLEMENT_CLASS(UVignetteModifier)
BEGIN_PROPERTIES(UVignetteModifier)
END_PROPERTIES()

UVignetteModifier::UVignetteModifier()
	: Intensity(0.5f)
	, Smoothness(0.5f)
{
	// Vignette는 일반 포스트 프로세스 우선순위
	Priority = 100;
}

UVignetteModifier::~UVignetteModifier()
{
}

void UVignetteModifier::ModifyPostProcess(FPostProcessSettings& OutSettings)
{
	// Alpha 값을 곱하여 효과 강도 조절
	OutSettings.bEnableVignette = true;
	OutSettings.VignetteIntensity = Intensity * Alpha;
	OutSettings.VignetteSmoothness = Smoothness;
}
