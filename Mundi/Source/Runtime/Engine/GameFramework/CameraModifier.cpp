#include "pch.h"
#include "CameraModifier.h"
#include "PlayerCameraManager.h"

IMPLEMENT_CLASS(UCameraModifier)
BEGIN_PROPERTIES(UCameraModifier)
	/* 필요하다면 추가할 것 */
END_PROPERTIES()

UCameraModifier::UCameraModifier()
	: CameraOwner(nullptr)
	, Alpha(1.0f)
	, TargetAlpha(1.0f)
	, AlphaInTime(0.2f)
	, AlphaOutTime(0.2f)
	, bDisabled(false)
	, Priority(128)
{
}

UCameraModifier::~UCameraModifier()
{
}

bool UCameraModifier::ModifyCamera(float DeltaTime, FVector& InOutLocation, FQuat& InOutRotation, float& InOutFOV)
{
	/* 필요하다면 추가할 것 
	 * CameraShake, CameraZoom 등
	*/

	return false;  // 카메라를 수정하지 않습니다
}

void UCameraModifier::EnableModifier(bool bImmediate)
{
	bDisabled = false;
	TargetAlpha = 1.0f;

	if (bImmediate)
	{
		Alpha = 1.0f;
	}
	// else: Alpha는 UpdateAlpha()에서 서서히 증가
}

void UCameraModifier::DisableModifier(bool bImmediate)
{
	TargetAlpha = 0.0f;

	if (bImmediate)
	{
		Alpha = 0.0f;
		bDisabled = true;
	}
	// else: Alpha는 UpdateAlpha()에서 서서히 감소하고, 0에 도달하면 bDisabled = true
}

void UCameraModifier::UpdateAlpha(float DeltaTime)
{
	// Alpha가 이미 목표값에 도달했으면 아무 것도 안 함
	if (Alpha == TargetAlpha)
	{
		return;
	}

	// Alpha를 TargetAlpha로 보간
	if (Alpha < TargetAlpha)
	{
		// Fade In (Alpha 증가)
		float AlphaSpeed = (AlphaInTime > 0.0f) ? (1.0f / AlphaInTime) : 999.0f;
		Alpha += AlphaSpeed * DeltaTime;

		if (Alpha >= TargetAlpha)
		{
			Alpha = TargetAlpha;
		}
	}
	else
	{
		// Fade Out (Alpha 감소)
		float AlphaSpeed = (AlphaOutTime > 0.0f) ? (1.0f / AlphaOutTime) : 999.0f;
		Alpha -= AlphaSpeed * DeltaTime;

		if (Alpha <= TargetAlpha)
		{
			Alpha = TargetAlpha;

			// Alpha가 0에 도달하면 완전히 비활성화
			if (Alpha <= 0.0f)
			{
				bDisabled = true;
			}
		}
	}
}
