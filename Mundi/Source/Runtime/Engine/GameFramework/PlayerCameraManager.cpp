#include "pch.h"
#include "PlayerCameraManager.h"
#include "CameraComponent.h"
#include "CameraModifier.h"
#include "PostProcessSettings.h"
#include "World.h"

IMPLEMENT_CLASS(APlayerCameraManager)

BEGIN_PROPERTIES(APlayerCameraManager)
	// Phase 1: 에디터에서 스폰 가능하도록 설정하지 않음 (자동 생성됨)
END_PROPERTIES()

APlayerCameraManager::APlayerCameraManager()
	: FadeColor(0, 0, 0, 1)
	, FadeAmount(0.0f)
	, FadeTime(0.0f)
	, FadeTimeRemaining(0.0f)
	, bIsFading(false)
	, bIsFadingOut(false)
{
	Name = "PlayerCameraManager";

	// PlayerCameraManager는 일반 Actor Tick 루프에서 제외
	// World::Tick에서 모든 Actor 업데이트 후 명시적으로 호출됨
	bTickInEditor = false;
}

APlayerCameraManager::~APlayerCameraManager()
{
	for (UCameraModifier*& Modifier : ModifierList)
	{
		delete Modifier;
		Modifier = nullptr;
	}

	ModifierList.clear();
}

void APlayerCameraManager::BeginPlay()
{
	Super::BeginPlay();
}

void APlayerCameraManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 카메라 상태 업데이트
	UpdateCamera(DeltaSeconds);

	// Fade 업데이트
	UpdateFade(DeltaSeconds);

	// Blend 업데이트
	// UpdateBlend(DeltaSeconds);
}

void APlayerCameraManager::EndPlay(EEndPlayReason Reason)
{
	Super::EndPlay(Reason);
}

void APlayerCameraManager::StartFadeOut(float Duration, FLinearColor ToColor)
{
	if (Duration <= 0.0f)
	{
		// 즉시 Fade
		FadeAmount = 1.0f;
		FadeColor = ToColor;
		bIsFading = false;
		return;
	}

	FadeColor = ToColor;
	FadeAmount = 0.0f;
	FadeTime = Duration;
	FadeTimeRemaining = Duration;
	bIsFading = true;
	bIsFadingOut = true;

	UE_LOG("APlayerCameraManager - Fade Out started: Duration={0}s", Duration);
}

void APlayerCameraManager::StartFadeIn(float Duration, FLinearColor FromColor)
{
	if (Duration <= 0.0f)
	{
		// 즉시 Fade
		FadeAmount = 0.0f;
		bIsFading = false;
		return;
	}

	FadeColor = FromColor;
	FadeAmount = 1.0f;
	FadeTime = Duration;
	FadeTimeRemaining = Duration;
	bIsFading = true;
	bIsFadingOut = false;

	UE_LOG("APlayerCameraManager - Fade In started: Duration={0}s", Duration);
}

UCameraModifier* APlayerCameraManager::AddCameraModifier(UCameraModifier* Modifier)
{
	// 0. nullptr 검사를 진행
	if (!Modifier) { return nullptr; }

	// 0. 중복 검사를 진행
	if (ModifierList.Contains(Modifier)) { return Modifier; }

	// 1. 모디파이어 리스트에 추가
	ModifierList.Add(Modifier);

	// 2. 우선 순위 정렬
	ModifierList.Sort([](const UCameraModifier* A, const UCameraModifier* B) {
		return A->Priority < B->Priority;
		});

	// 3. 소유자 설정
	Modifier->CameraOwner = this;

	UE_LOG("APlayerCameraManager - CameraModifier added: {0}", Modifier->GetName());

	return Modifier;
}

void APlayerCameraManager::RemoveCameraModifier(UCameraModifier* Modifier)
{
	if (!Modifier) { return; }

	ModifierList.Remove(Modifier);
	UE_LOG("APlayerCameraManager - CameraModifier removed: {0}", Modifier->GetName());
}

void APlayerCameraManager::SetViewTarget(AActor* NewViewTarget, float BlendTime, ECameraBlendType BlendFunc)
{
	if (!NewViewTarget)
	{
		UE_LOG("APlayerCameraManager::SetViewTarget - NewViewTarget is null");
		return;
	}

	ViewTarget.Target = NewViewTarget;
	UE_LOG("APlayerCameraManager - ViewTarget set to: {0}", NewViewTarget->GetName());
}

UCameraComponent* APlayerCameraManager::GetViewTargetCameraComponent() const
{
	if (!ViewTarget.Target) { return nullptr; }

	return ViewTarget.Target->GetComponent<UCameraComponent>();
}

FPostProcessSettings APlayerCameraManager::GetPostProcessSettings() const
{
	FPostProcessSettings Settings;

	// 1. 기본 Fade 설정 (PlayerCameraManager가 직접 관리)
	Settings.FadeColor = FadeColor;
	Settings.FadeAmount = FadeAmount;

	// 2. 모든 CameraModifier에게 후처리 기여 요청
	for (UCameraModifier* Modifier : ModifierList)
	{
		if (Modifier && Modifier->IsEnabled())
		{
			Modifier->ModifyPostProcess(Settings);
		}
	}

	return Settings;
}

FMatrix APlayerCameraManager::GetViewMatrix() const
{
	// ViewCache에 저장된 최종 위치와 회전을 사용하여 View 행렬 생성
	return FMatrix::LookAtLH(ViewCache.Location,
		ViewCache.Location + ViewCache.Rotation.GetForwardVector(),
		ViewCache.Rotation.GetUpVector());
}

FMatrix APlayerCameraManager::GetProjectionMatrix(FViewport* Viewport) const
{
	// Projection 행렬은 일반적으로 모디파이어의 영향을 받지 않으므로 원본 사용
	if (UCameraComponent* CameraComp = GetViewTargetCameraComponent())
	{
		return CameraComp->GetProjectionMatrix(0.0f, Viewport);
	}

	return FMatrix::Identity();
}

void APlayerCameraManager::OnSerialized()
{
	Super::OnSerialized();
}

void APlayerCameraManager::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);
}

void APlayerCameraManager::UpdateFade(float DeltaTime)
{
	if (!bIsFading) { return; }

	FadeTimeRemaining -= DeltaTime;

	if (FadeTimeRemaining <= 0.0f)
	{
		// Fade 완료
		FadeAmount = bIsFadingOut ? 1.0f : 0.0f;
		bIsFading = false;
		UE_LOG("APlayerCameraManager - Fade completed");
		return;
	}

	// 선형 보간 (Phase 1: Linear만)
	float Alpha = 1.0f - (FadeTimeRemaining / FadeTime);
	FadeAmount = bIsFadingOut ? Alpha : (1.0f - Alpha);
}

void APlayerCameraManager::UpdateCamera(float DeltaTime)
{
	// 1. ViewTarget의 카메라 컴포넌트에서 기본 값 가져오기
	UCameraComponent* CameraComp = GetViewTargetCameraComponent();
	if (!CameraComp) { return; }

	ViewCache.Location = CameraComp->GetWorldLocation();
	ViewCache.Rotation = CameraComp->GetWorldRotation();
	ViewCache.FOV = CameraComp->GetFOV();

	// 2. 모든 모디파이어의 Alpha 업데이트 및 적용
	for (UCameraModifier* Modifier : ModifierList)
	{
		if (Modifier)
		{
			// Alpha 페이드 인/아웃 업데이트
			Modifier->UpdateAlpha(DeltaTime);

			// 활성화된 모디파이어만 카메라에 적용
			if (Modifier->IsEnabled())
			{
				Modifier->ModifyCamera(DeltaTime, ViewCache.Location, ViewCache.Rotation, ViewCache.FOV);
			}
		}
	}
}
