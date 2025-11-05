#pragma once
#include "CameraModifier.h"
#include "Color.h"

/**
 * @brief 레터박스(시네마틱 상하단 검은 바) 모디파이어
 *
 * 기술:
 * - 뷰포트 로컬 UV의 Y 좌표를 기준으로 상단/하단 영역 판별
 * - localUV.y < LetterboxHeight → 상단 레터박스 (LetterboxColor 출력)
 * - localUV.y > (1.0 - LetterboxHeight) → 하단 레터박스 (LetterboxColor 출력)
 * - 그 외 → 원본 씬 컬러 출력
 */
class ULetterboxModifier : public UCameraModifier
{
public:
	DECLARE_CLASS(ULetterboxModifier, UCameraModifier)
	GENERATED_REFLECTION_BODY()

	ULetterboxModifier();

protected:
	virtual ~ULetterboxModifier() override;

public:
	virtual void ModifyPostProcess(FPostProcessSettings& OutSettings) override;

	void SetHeight(float InHeight) { Height = FMath::Clamp(InHeight, 0.0f, 0.5f); }
	void SetColor(const FLinearColor& InColor) { Color = InColor; }

	float GetHeight() const { return Height; }
	FLinearColor GetColor() const { return Color; }

private:
	float Height = 0.1f;                           // 레터박스 높이 비율 (0.0 ~ 0.5)
	FLinearColor Color = FLinearColor(0, 0, 0, 1); // 레터박스 색상 (기본: 검정)
};
