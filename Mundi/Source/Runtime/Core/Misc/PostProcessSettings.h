#pragma once

#include "Vector.h"                                                                                                           
#include "Color.h"

/**
* @brief 후처리 효과 설정을 모아둔 구조체
*/
struct FPostProcessSettings
{
	FPostProcessSettings() = default;

	// Fade
	FLinearColor FadeColor = FLinearColor(0, 0, 0, 1);
	float FadeAmount = 0.0f;

	// Vignette
	bool bEnableVignette = false;
	float VignetteIntensity = 0.5f;
	float VignetteSmoothness = 0.5f;

	// Gamma Correction
	bool bEnableGammaCorrection = false;
	float Gamma = 2.2f;

	// Letterbox
	bool bEnableLetterbox = false;
	float LetterboxHeight = 0.1f;
	FLinearColor LetterboxColor = FLinearColor(0, 0, 0, 1);

	bool IsEmpty() const
	{
		return FadeAmount <= 0.0f
			&& !bEnableLetterbox
			&& !bEnableVignette
			&& !bEnableGammaCorrection;
	}
};