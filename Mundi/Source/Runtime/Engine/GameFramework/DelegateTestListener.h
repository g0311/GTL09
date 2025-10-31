#pragma once
#include "Actor.h"

// Forward Declaration
class ADelegateTestButton;

/**
 * ADelegateTestListener
 * 테스트용 리스너 액터 - 버튼의 이벤트를 받아서 반응
 */
class ADelegateTestListener : public AActor
{
public:
	DECLARE_CLASS(ADelegateTestListener, AActor)
	GENERATED_REFLECTION_BODY()

	ADelegateTestListener();
	~ADelegateTestListener() override = default;

	void BeginPlay() override;
	void Tick(float DeltaSeconds) override;

	void DuplicateSubObjects() override;
	DECLARE_DUPLICATE(ADelegateTestListener)

	void OnSerialized() override;

	// 버튼 이벤트에 반응하는 함수
	void OnButtonEventReceived();

	// 버튼 액터 설정 (코드에서 사용)
	void SetButtonActor(ADelegateTestButton* Button);

private:
	// 월드에서 첫 번째 DelegateTestButton을 찾아서 연결
	void FindAndBindToButton();

private:
	ADelegateTestButton* ButtonActor = nullptr; // 구독할 버튼
	FDelegateHandle ButtonEventHandle; // 델리게이트 핸들 저장

	float AnimationTimer = 0.0f; // 애니메이션용 타이머
	FVector OriginalPosition; // 원래 위치
	bool bIsAnimating = false; // 애니메이션 중인지
};
