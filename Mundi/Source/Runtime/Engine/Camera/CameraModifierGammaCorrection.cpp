#include "pch.h"
#include "CameraModifierGammaCorrection.h"
#include "PostProcessSettings.h"

IMPLEMENT_CLASS(UGammaCorrectionModifier)
BEGIN_PROPERTIES(UGammaCorrectionModifier)
END_PROPERTIES()

UGammaCorrectionModifier::UGammaCorrectionModifier()
	: Gamma(2.2f)
{
	// GammaCorrection은 일반 포스트 프로세스 우선순위
	Priority = 110;
}

UGammaCorrectionModifier::~UGammaCorrectionModifier()
{
}

void UGammaCorrectionModifier::ModifyPostProcess(FPostProcessSettings& OutSettings)
{
	// Alpha 값은 감마 보정에서는 직접 사용하지 않음 (On/Off만)
	if (Alpha > 0.0f)
	{
		OutSettings.bEnableGammaCorrection = true;
		OutSettings.Gamma = Gamma;
	}
}
