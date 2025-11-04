#pragma once
#include "Object.h"

class APlayerCameraManager;
struct FPostProcessSettings;

/**
 * @brief 카메라 효과 수정자 베이스 클래스
 *
 * 역할:
 * - 카메라 최종 Transform/FOV를 수정
 * - 화면 흔들림, 줌, 회전 등의 효과 구현
 *
 * Phase 1: 인터페이스만 정의 (실제 동작 없음)
 * Phase 2: 구체 클래스 구현 (CameraShake, CameraZoom 등)
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
	 * @brief 카메라 정보를 수정하는 메인 함수
	 * @param DeltaTime 프레임 시간
	 * @param InOutLocation 카메라 위치 (수정 가능)
	 * @param InOutRotation 카메라 회전 (수정 가능)
	 * @param InOutFOV 카메라 FOV (수정 가능)
	 * @return true면 카메라 정보를 수정함
	 */
	virtual bool ModifyCamera(float DeltaTime, FVector& InOutLocation, FQuat& InOutRotation, float& InOutFOV);

	/**
	 * @brief 후처리 효과에 기여 (서브클래스에서 오버라이드)
	 * @param OutSettings 후처리 설정 구조체 (여러 Modifier가 순차적으로 수정)
	 *
	 * Priority 순서대로 호출되므로, 각 Modifier는 OutSettings를 읽고 수정할 수 있습니다.
	 * Alpha를 활용하여 효과의 강도를 조절할 수 있습니다.
	 */
	virtual void ModifyPostProcess(FPostProcessSettings& OutSettings)
	{
		// 기본 구현: 아무것도 하지 않음
		// 서브클래스에서 필요한 경우 오버라이드
	}

	/**
	 * @param bImmediate true면 즉시 활성화 (Alpha = 1.0), false면 서서히 페이드 인
	 */
	void EnableModifier(bool bImmediate = false);

	/**
	 * @param bImmediate true면 즉시 비활성화 (Alpha = 0.0), false면 서서히 페이드 아웃
	 */
	void DisableModifier(bool bImmediate = false);

	bool IsEnabled() const { return !bDisabled; }

	float GetAlpha() const { return Alpha; }

	/**
	 * @brief 매 프레임 Alpha 업데이트 (페이드 인/아웃)
	 * @note PlayerCameraManager에서 자동 호출됨
	 */
	void UpdateAlpha(float DeltaTime);

protected:
	APlayerCameraManager* CameraOwner = nullptr;

	// Fade In & Out 관련 변수
	float Alpha = 1.0f;			// 현재 Alpha 값 (0.0 = 비활성화, 1.0 = 활성화) 
	float TargetAlpha = 1.0f;	// 목표 Alpha 값 (페이드 인/아웃 시 사용) 
	float AlphaInTime = 0.2f;   // 페이드 인에 걸리는 시간 (초)
	float AlphaOutTime = 0.2f;  // 페이드 아웃에 걸리는 시간 (초)

	bool bDisabled = false; // 모디파이어 비활성화 여부

	uint8 Priority = 128; // 모디파이어 우선순위 (높을수록 나중에 적용, 카메라 효과 순서를 정하기 위함)

	friend class APlayerCameraManager;
};
