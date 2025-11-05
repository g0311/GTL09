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
	// Alpha 값을 사용하여 선형(1.0)과 설정 감마 사이를 보간
	OutSettings.bEnableGammaCorrection = true;
	// Alpha가 1.0이면 설정 감마, 0.0이면 선형 감마(1.0 = 보정 없음)로 보간
	float LinearGamma = 1.0f;  // 감마 보정 없음 (disable 상태와 동일)
	OutSettings.Gamma = LinearGamma + (Gamma - LinearGamma) * Alpha;
}
