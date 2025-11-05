#pragma once
#include "CameraModifier.h"

/**
 * @brief 감마 보정 모디파이어
 *
 * 기술:
 * - Linear 색상 공간 → sRGB 색상 공간 변환
 * - 공식: correctedColor = pow(sceneColor, 1.0 / gamma)
 * - 일반적으로 gamma 값은 2.2 사용 (sRGB 표준)
 */
class UGammaCorrectionModifier : public UCameraModifier
{
public:
	DECLARE_CLASS(UGammaCorrectionModifier, UCameraModifier)
	GENERATED_REFLECTION_BODY()

	UGammaCorrectionModifier();

protected:
	virtual ~UGammaCorrectionModifier() override;

public:
	virtual void ModifyPostProcess(FPostProcessSettings& OutSettings) override;

	void SetGamma(float InGamma) { Gamma = FMath::Max(InGamma, 0.1f); }
	float GetGamma() const { return Gamma; }

private:
	float Gamma = 2.2f;  // 감마 값 (sRGB 표준: 2.2)
};
