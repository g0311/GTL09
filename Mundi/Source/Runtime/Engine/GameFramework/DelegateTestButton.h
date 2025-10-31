#pragma once
#include "Actor.h"

// 버튼이 눌렸을 때 발생하는 델리게이트 선언
DECLARE_DELEGATE_NoParams(FOnButtonPressed);

/**
 * ADelegateTestButton
 * 테스트용 버튼 액터 - 'B' 키를 누르면 델리게이트 이벤트 발생
 */
class ADelegateTestButton : public AActor
{
public:
	DECLARE_CLASS(ADelegateTestButton, AActor)
	GENERATED_REFLECTION_BODY()

	ADelegateTestButton();
	~ADelegateTestButton() override = default;

	void Tick(float DeltaSeconds) override;

	void DuplicateSubObjects() override;
	DECLARE_DUPLICATE(ADelegateTestButton)

	void OnSerialized() override;

	// 버튼 눌림 이벤트 델리게이트
	FOnButtonPressed OnButtonPressed;

private:
	bool bWasKeyPressed = false; // 이전 프레임 키 상태 (중복 방지용)
};
