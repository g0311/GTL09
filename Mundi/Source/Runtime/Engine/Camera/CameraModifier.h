#pragma once
#include "Object.h"

class APlayerCameraManager;
struct FPostProcessSettings;

/**
 * @brief 카메라 효과를 구현하는 수정자(Modifier)의 베이스 클래스.
 * @note 각 모디파이어는 고유한 'Alpha' 값을 가집니다.
 *		 'Alpha' 값으로 모디파이어의 효과 '세기'를 조절하는 데 사용됩니다.
 */
class UCameraModifier : public UObject
{
public:
	DECLARE_CLASS(UCameraModifier, UObject)
	GENERATED_REFLECTION_BODY()

	UCameraModifier();

protected:
	virtual ~UCameraModifier() override;

public:
	/**
	 * @brief 카메라 정보를 수정하는 메인 함수. 파생 클래스에서 이 함수를 오버라이드하여 실제 효과를 구현합니다.
	 * @param DeltaTime 프레임 시간
	 * @param InOutLocation 카메라 위치 (수정 가능)
	 * @param InOutRotation 카메라 회전 (수정 가능)
	 * @param InOutFOV 카메라 FOV (수정 가능)
	 * @return true면 카메라 정보를 수정했음을 의미합니다.
	 */
	virtual bool ModifyCamera(float DeltaTime, FVector& InOutLocation, FQuat& InOutRotation, float& InOutFOV) { return false; }

	/**
	 * @brief 후처리 효과에 기여 (서브클래스에서 오버라이드)
	 * @param OutSettings 후처리 설정 구조체 (여러 Modifier가 순차적으로 수정)
	 */
	virtual void ModifyPostProcess(FPostProcessSettings& OutSettings) {}

	void EnableModifier(bool bImmediate = false);
	void DisableModifier(bool bImmediate = false);

	bool IsEnabled() const { return !bDisabled; }

	void UpdateAlpha(float DeltaTime);

	float GetAlpha() const { return Alpha; }
	void SetAlpha(const float InAlpha) { Alpha = InAlpha; }

	float GetTargetAlpha() const { return TargetAlpha; }
	void SetTargetAlpha(const float InTargetAlpha) { TargetAlpha = InTargetAlpha; }

	float GetAlphaInTime() const { return AlphaInTime; }
	void SetAlphaInTime(const float InTime) { AlphaInTime = InTime; }

	float GetAlphaOutTime() const { return AlphaOutTime; }
	void SetAlphaOutTime(const float InTime) { AlphaOutTime = InTime; }


protected:
	APlayerCameraManager* CameraOwner = nullptr;

	float Alpha = 1.0f; // 현재 효과의 강도값 (ModifyCamera() 내에서 이 값을 곱하여 사용함)
	float TargetAlpha = 1.0f; // 효과의 강도 목표값 (Enable/Disable시 이 값이 1.0 또는 0.0으로 설정됨)
	float AlphaInTime = 0.2f;
	float AlphaOutTime = 0.2f;

	bool bDisabled = false; // 모디파이어 비활성화 여부

	uint8 Priority = 128; // 모디파이어 우선순위 (높을수록 나중에 적용, 카메라 효과 순서를 정하기 위함)

	friend class APlayerCameraManager;
};
