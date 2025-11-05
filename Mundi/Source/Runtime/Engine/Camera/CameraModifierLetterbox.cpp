#include "pch.h"
#include "CameraModifierLetterbox.h"
#include "PostProcessSettings.h"

IMPLEMENT_CLASS(ULetterboxModifier)
BEGIN_PROPERTIES(ULetterboxModifier)
END_PROPERTIES()

ULetterboxModifier::ULetterboxModifier()
	: Height(0.1f)
	, Color(0, 0, 0, 1)
{
	// Letterbox는 일반 포스트 프로세스 우선순위
	Priority = 120;
}

ULetterboxModifier::~ULetterboxModifier()
{
}

void ULetterboxModifier::ModifyPostProcess(FPostProcessSettings& OutSettings)
{
	// Alpha 값은 레터박스 높이에 곱해서 서서히 나타나게 함
	OutSettings.bEnableLetterbox = true;
	OutSettings.LetterboxHeight = Height * Alpha;
	OutSettings.LetterboxColor = Color;
}
