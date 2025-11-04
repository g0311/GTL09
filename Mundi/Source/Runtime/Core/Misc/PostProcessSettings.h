#pragma once

#include "Vector.h"                                                                                                           
#include "Color.h"

/**
* @brief 후처리 효과 설정을 모아둔 구조체
*/
struct FPostProcessSettings
{
	FPostProcessSettings() = default;

	FLinearColor FadeColor = FLinearColor(0, 0, 0, 1);
	float FadeAmount = 0.0f;

	bool bEnableLetterbox = false;
	float LetterboxAmount = 0.0f;

	bool bEnableVignette = false;
	float VignetteIntensity = 0.5f;

	float Gamma = 2.2f;

	bool IsEmpty() const
	{
		return FadeAmount <= 0.0f
			&& !bEnableLetterbox
			&& !bEnableVignette;
	}
};