#include "pch.h"
#include "DelegateTestButton.h"
#include "InputManager.h"

IMPLEMENT_CLASS(ADelegateTestButton)

BEGIN_PROPERTIES(ADelegateTestButton)
	MARK_AS_SPAWNABLE("테스트버튼", "델리게이트 테스트용 버튼 액터 - B키를 누르면 이벤트 발생")
END_PROPERTIES()

ADelegateTestButton::ADelegateTestButton()
{
	Name = "DelegateTestButton";
}

void ADelegateTestButton::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// B 키가 눌렸는지 확인
	bool bIsKeyDown = INPUT.IsKeyDown('B');

	// 키가 이번 프레임에 눌렸고, 이전 프레임에는 눌리지 않았다면 (중복 방지)
	if (bIsKeyDown && !bWasKeyPressed)
	{
		// 델리게이트가 바인딩되어 있다면 브로드캐스트
		if (OnButtonPressed.IsBound())
		{
			UE_LOG("[DelegateTestButton] B key pressed! Broadcasting to %d listener(s)...", OnButtonPressed.Num());
			OnButtonPressed.Broadcast();
		}
		else
		{
			UE_LOG("[DelegateTestButton] B key pressed but no listeners bound.");
		}
	}

	bWasKeyPressed = bIsKeyDown;
}

void ADelegateTestButton::DuplicateSubObjects()
{
	Super::DuplicateSubObjects();
}

void ADelegateTestButton::OnSerialized()
{
	Super::OnSerialized();
}
