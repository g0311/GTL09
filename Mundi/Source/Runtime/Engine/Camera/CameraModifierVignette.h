#pragma once
#include "CameraModifier.h"

/**
 * @brief 화면 가장자리를 어둡게 만드는 비네팅 효과 모디파이어
 *
 * 기술:
 * - NDC 중점과의 거리를 통해 계산
 * - Intensity보다 가까우면 0, 멀면 1, 그 사이는 S-커브 보간
 * - Smoothness로 전환 부드러움 조절
 */
class UVignetteModifier : public UCameraModifier
{
public:
	DECLARE_CLASS(UVignetteModifier, UCameraModifier)
	GENERATED_REFLECTION_BODY()

	UVignetteModifier();

protected:
	virtual ~UVignetteModifier() override;

public:
	virtual void ModifyPostProcess(FPostProcessSettings& OutSettings) override;

	void SetIntensity(float InIntensity) { Intensity = FMath::Clamp(InIntensity, 0.0f, 1.0f); }
	void SetSmoothness(float InSmoothness) { Smoothness = FMath::Clamp(InSmoothness, 0.0f, 1.0f); }

	float GetIntensity() const { return Intensity; }
	float GetSmoothness() const { return Smoothness; }

private:
	float Intensity = 0.5f;     // 비네팅 강도 (0.0 ~ 1.0)
	float Smoothness = 0.5f;    // 비네팅 부드러움 (0.0 ~ 1.0)
};
