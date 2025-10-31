#include "pch.h"
#include "DelegateTestListener.h"
#include "DelegateTestButton.h"
#include "SceneComponent.h"
#include "World.h"
#include <cmath>

IMPLEMENT_CLASS(ADelegateTestListener)

BEGIN_PROPERTIES(ADelegateTestListener)
	MARK_AS_SPAWNABLE("테스트리스너", "델리게이트 테스트용 리스너 액터 - 버튼 이벤트를 받아서 점프")
END_PROPERTIES()

ADelegateTestListener::ADelegateTestListener()
{
	Name = "DelegateTestListener";
}

void ADelegateTestListener::BeginPlay()
{
	Super::BeginPlay();

	// 버튼 액터가 설정되지 않았다면 월드에서 자동으로 찾기
	if (ButtonActor == nullptr)
	{
		FindAndBindToButton();
	}
	else
	{
		// 이미 설정된 버튼에 바인딩
		ButtonEventHandle = ButtonActor->OnButtonPressed.AddDynamic(
			this,
			&ADelegateTestListener::OnButtonEventReceived
		);
		UE_LOG("[DelegateTestListener] Successfully bound to button event!");
	}

	// 원래 위치 저장
	if (RootComponent)
	{
		OriginalPosition = RootComponent->GetRelativeLocation();
	}
}

void ADelegateTestListener::FindAndBindToButton()
{
	UWorld* MyWorld = GetWorld();
	if (!MyWorld)
	{
		UE_LOG("[DelegateTestListener] Warning: No world found!");
		return;
	}

	// 월드의 모든 액터를 순회하면서 DelegateTestButton 찾기
	const TArray<AActor*>& AllActors = MyWorld->GetActors();
	for (AActor* Actor : AllActors)
	{
		if (Actor && Actor->IsA(ADelegateTestButton::StaticClass()))
		{
			ButtonActor = Cast<ADelegateTestButton>(Actor);
			if (ButtonActor)
			{
				// 델리게이트 바인딩
				ButtonEventHandle = ButtonActor->OnButtonPressed.AddDynamic(
					this,
					&ADelegateTestListener::OnButtonEventReceived
				);
				UE_LOG("[DelegateTestListener] Auto-found and bound to DelegateTestButton!");
				return;
			}
		}
	}

	UE_LOG("[DelegateTestListener] Warning: No DelegateTestButton found in world!");
}

void ADelegateTestListener::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 애니메이션 중이라면 점프 모션 재생
	if (bIsAnimating)
	{
		AnimationTimer += DeltaSeconds;

		// 1초 동안 위아래로 점프하는 애니메이션
		const float Duration = 1.0f;
		const float JumpHeight = 100.0f;

		if (AnimationTimer < Duration)
		{
			// 사인 곡선으로 부드러운 점프
			float Progress = AnimationTimer / Duration;
			float Height = sin(Progress * 3.14159f) * JumpHeight;

			if (RootComponent)
			{
				FVector NewPos = OriginalPosition;
				NewPos.Z += Height;
				RootComponent->SetRelativeLocation(NewPos);
			}
		}
		else
		{
			// 애니메이션 종료 - 원래 위치로 복귀
			if (RootComponent)
			{
				RootComponent->SetRelativeLocation(OriginalPosition);
			}
			bIsAnimating = false;
			AnimationTimer = 0.0f;
		}
	}
}

void ADelegateTestListener::OnButtonEventReceived()
{
	UE_LOG("[DelegateTestListener] Button event received! Starting jump animation...");

	// 애니메이션 시작
	bIsAnimating = true;
	AnimationTimer = 0.0f;

	if (RootComponent)
	{
		OriginalPosition = RootComponent->GetRelativeLocation();
	}
}

void ADelegateTestListener::SetButtonActor(ADelegateTestButton* Button)
{
	// 기존 바인딩 제거
	if (ButtonActor != nullptr && ButtonEventHandle != 0)
	{
		ButtonActor->OnButtonPressed.Remove(ButtonEventHandle);
	}

	// 새 버튼 설정
	ButtonActor = Button;

	// 새 델리게이트 바인딩
	if (ButtonActor != nullptr)
	{
		ButtonEventHandle = ButtonActor->OnButtonPressed.AddDynamic(
			this,
			&ADelegateTestListener::OnButtonEventReceived
		);
		UE_LOG("[DelegateTestListener] Button actor updated and rebound!");
	}
}

void ADelegateTestListener::DuplicateSubObjects()
{
	Super::DuplicateSubObjects();
}

void ADelegateTestListener::OnSerialized()
{
	Super::OnSerialized();
}
