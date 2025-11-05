#include "pch.h"
#include "CameraModifierFade.h"
#include "PostProcessSettings.h"

IMPLEMENT_CLASS(UFadeModifier)
BEGIN_PROPERTIES(UFadeModifier)
	/* 필요하다면 에디터 노출 프로퍼티 추가 */
END_PROPERTIES()

UFadeModifier::UFadeModifier()
	: FadeColor(0, 0, 0, 1)
	, FadeAmount(0.0f)
	, FadeTime(0.0f)
	, FadeTimeRemaining(0.0f)
	, bIsFading(false)
	, bIsFadingOut(false)
{
	// Fade는 가장 마지막에 적용되어야 함 (최상위 오버레이)
	Priority = 200;

	// Fade는 기본적으로 비활성화 상태로 시작
	Alpha = 0.0f;
	TargetAlpha = 0.0f;
	bDisabled = true;
}

UFadeModifier::~UFadeModifier()
{
}

void UFadeModifier::ModifyPostProcess(FPostProcessSettings& OutSettings)
{
	// Alpha 값을 곱하여 효과 강도 조절
	OutSettings.FadeColor = FadeColor;
	OutSettings.FadeAmount = FadeAmount * Alpha;
}

void UFadeModifier::StartFadeOut(float Duration, FLinearColor ToColor)
{
	// 모디파이어 활성화
	EnableModifier(true);

	if (Duration <= 0.0f)
	{
		// 즉시 Fade
		FadeAmount = 1.0f;
		FadeColor = ToColor;
		bIsFading = false;
		return;
	}

	FadeColor = ToColor;
	FadeAmount = 0.0f;
	FadeTime = Duration;
	FadeTimeRemaining = Duration;
	bIsFading = true;
	bIsFadingOut = true;

	UE_LOG("UCameraModifier_Fade - Fade Out started: Duration={0}s", Duration);
}

void UFadeModifier::StartFadeIn(float Duration, FLinearColor FromColor)
{
	// 모디파이어 활성화
	EnableModifier(true);

	if (Duration <= 0.0f)
	{
		// 즉시 Fade 제거
		FadeAmount = 0.0f;
		bIsFading = false;

		// Fade가 끝났으므로 모디파이어 비활성화
		DisableModifier(true);
		return;
	}

	FadeColor = FromColor;
	FadeAmount = 1.0f;
	FadeTime = Duration;
	FadeTimeRemaining = Duration;
	bIsFading = true;
	bIsFadingOut = false;

	UE_LOG("UCameraModifier_Fade - Fade In started: Duration={0}s", Duration);
}

void UFadeModifier::UpdateFade(float DeltaTime)
{
	if (!bIsFading) { return; }

	FadeTimeRemaining -= DeltaTime;

	if (FadeTimeRemaining <= 0.0f)
	{
		// Fade 완료
		FadeAmount = bIsFadingOut ? 1.0f : 0.0f;
		bIsFading = false;

		// Fade In이 완료되었으면 모디파이어 비활성화
		if (!bIsFadingOut)
		{
			DisableModifier(true);
		}

		UE_LOG("UCameraModifier_Fade - Fade completed");
		return;
	}

	// 선형 보간
	float Alpha = 1.0f - (FadeTimeRemaining / FadeTime);
	FadeAmount = bIsFadingOut ? Alpha : (1.0f - Alpha);
}
