#pragma once
#include "CameraModifier.h"
#include "Color.h"

/**
 * @brief 화면 전체를 특정 색상으로 페이드 인/아웃하는 카메라 모디파이어
 */
class UFadeModifier : public UCameraModifier
{
public:
	DECLARE_CLASS(UFadeModifier, UCameraModifier)
	GENERATED_REFLECTION_BODY()

	UFadeModifier();

protected:
	virtual ~UFadeModifier() override;

public:
	/**
	 * @brief 후처리 설정에 Fade 효과를 기여합니다
	 * @param OutSettings 후처리 설정 구조체
	 */
	virtual void ModifyPostProcess(FPostProcessSettings& OutSettings) override;

	/**
	 * @brief 화면을 특정 색상으로 페이드 아웃합니다 (점점 불투명해짐)
	 */
	void StartFadeOut(float Duration, FLinearColor ToColor = FLinearColor(0, 0, 0, 1));

	/**
	 * @brief 화면을 페이드 인합니다 (점점 투명해짐)
	 */
	void StartFadeIn(float Duration, FLinearColor FromColor = FLinearColor(0, 0, 0, 1));

	/**
	 * @brief 매 프레임 Fade Alpha를 업데이트합니다
	 * @param DeltaTime 프레임 시간
	 * @note PlayerCameraManager가 UpdateAlpha() 호출 전에 이 메서드를 호출해야 합니다
	 */
	void UpdateFade(float DeltaTime);

	/** @brief 현재 페이드 양 (0 = 투명, 1 = 완전 불투명) */
	float GetFadeAmount() const { return FadeAmount; }

	/** @brief 현재 페이드 색상 */
	FLinearColor GetFadeColor() const { return FadeColor; }

private:
	// Fade 색상 및 강도
	FLinearColor FadeColor = FLinearColor(0, 0, 0, 1);  // 페이드 색상
	float FadeAmount = 0.0f;                            // 현재 Fade Alpha (0~1)

	// Fade 애니메이션 관련
	float FadeTime = 0.0f;                              // Fade 총 시간
	float FadeTimeRemaining = 0.0f;                     // 남은 시간
	bool bIsFading = false;                             // Fade 중인지
	bool bIsFadingOut = false;                          // true = Out, false = In
};
