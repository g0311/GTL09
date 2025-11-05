#include "pch.h"
#include "PlayerCameraManager.h"
#include "CameraComponent.h"
#include "CameraModifier.h"
#include "CameraModifierFade.h"
#include "CameraModifierVignette.h"
#include "CameraModifierGammaCorrection.h"
#include "CameraModifierLetterbox.h"
#include "PostProcessSettings.h"
#include "CameraShakeBase.h"
#include "CameraShakePattern.h"

IMPLEMENT_CLASS(APlayerCameraManager)

BEGIN_PROPERTIES(APlayerCameraManager)
	// Phase 1: 에디터에서 스폰 가능하도록 설정하지 않음 (자동 생성됨)
END_PROPERTIES()

APlayerCameraManager::APlayerCameraManager()
{
	Name = "PlayerCameraManager";

	// PlayerCameraManager는 일반 Actor Tick 루프에서 제외
	// World::Tick에서 모든 Actor 업데이트 후 명시적으로 호출됨
	bTickInEditor = false;

	InitializeModifiers();
}

APlayerCameraManager::~APlayerCameraManager()
{
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
}

void APlayerCameraManager::EndPlay(EEndPlayReason Reason)
{
	Super::EndPlay(Reason);
}

void APlayerCameraManager::StartFadeOut(float Duration, FLinearColor ToColor)
{
	// UCameraModifier_Fade를 찾아서 위임
	if (UFadeModifier* FadeModifier = FindModifier<UFadeModifier>())
	{
		FadeModifier->StartFadeOut(Duration, ToColor);
	}
	else
	{
		UE_LOG("APlayerCameraManager::StartFadeOut - UCameraModifier_Fade not found! Add one first.");
	}
}

void APlayerCameraManager::StartFadeIn(float Duration, FLinearColor FromColor)
{
	// UCameraModifier_Fade를 찾아서 위임
	if (UFadeModifier* FadeModifier = FindModifier<UFadeModifier>())
	{
		FadeModifier->StartFadeIn(Duration, FromColor);
	}
	else
	{
		UE_LOG("APlayerCameraManager::StartFadeIn - UCameraModifier_Fade not found! Add one first.");
	}
}

void APlayerCameraManager::EnableVignette(float Intensity, float Smoothness, bool bImmediate)
{
	// UVignetteModifier를 찾아서 위임
	if (UVignetteModifier* VignetteModifier = FindModifier<UVignetteModifier>())
	{
		VignetteModifier->SetIntensity(Intensity);
		VignetteModifier->SetSmoothness(Smoothness);
		VignetteModifier->EnableModifier(bImmediate);
	}
	else
	{
		UE_LOG("APlayerCameraManager::EnableVignette - UVignetteModifier not found! Add one first.");
	}
}

void APlayerCameraManager::DisableVignette(bool bImmediate)
{
	// UVignetteModifier를 찾아서 위임
	if (UVignetteModifier* VignetteModifier = FindModifier<UVignetteModifier>())
	{
		VignetteModifier->DisableModifier(bImmediate);
	}
	else
	{
		UE_LOG("APlayerCameraManager::DisableVignette - UVignetteModifier not found! Add one first.");
	}
}

void APlayerCameraManager::SetVignetteIntensity(float Intensity)
{
	if (UVignetteModifier* VignetteModifier = FindModifier<UVignetteModifier>())
	{
		VignetteModifier->SetIntensity(Intensity);
	}
}

void APlayerCameraManager::SetVignetteSmoothness(float Smoothness)
{
	if (UVignetteModifier* VignetteModifier = FindModifier<UVignetteModifier>())
	{
		VignetteModifier->SetSmoothness(Smoothness);
	}
}

void APlayerCameraManager::EnableGammaCorrection(float Gamma, bool bImmediate)
{
	if (UGammaCorrectionModifier* GammaModifier = FindModifier<UGammaCorrectionModifier>())
	{
		GammaModifier->SetGamma(Gamma);
		GammaModifier->EnableModifier(bImmediate);
	}
	else
	{
		UE_LOG("APlayerCameraManager::EnableGammaCorrection - UGammaCorrectionModifier not found! Add one first.");
	}
}

void APlayerCameraManager::DisableGammaCorrection(bool bImmediate)
{
	if (UGammaCorrectionModifier* GammaModifier = FindModifier<UGammaCorrectionModifier>())
	{
		GammaModifier->DisableModifier(bImmediate);
	}
	else
	{
		UE_LOG("APlayerCameraManager::DisableGammaCorrection - UGammaCorrectionModifier not found! Add one first.");
	}
}

void APlayerCameraManager::SetGamma(float Gamma)
{
	if (UGammaCorrectionModifier* GammaModifier = FindModifier<UGammaCorrectionModifier>())
	{
		GammaModifier->SetGamma(Gamma);
	}
}

void APlayerCameraManager::EnableLetterbox(float Height, FLinearColor Color, bool bImmediate)
{
	if (ULetterboxModifier* LetterboxModifier = FindModifier<ULetterboxModifier>())
	{
		LetterboxModifier->SetHeight(Height);
		LetterboxModifier->SetColor(Color);
		LetterboxModifier->EnableModifier(bImmediate);
	}
	else
	{
		UE_LOG("APlayerCameraManager::EnableLetterbox - ULetterboxModifier not found! Add one first.");
	}
}

void APlayerCameraManager::DisableLetterbox(bool bImmediate)
{
	if (ULetterboxModifier* LetterboxModifier = FindModifier<ULetterboxModifier>())
	{
		LetterboxModifier->DisableModifier(bImmediate);
	}
	else
	{
		UE_LOG("APlayerCameraManager::DisableLetterbox - ULetterboxModifier not found! Add one first.");
	}
}

void APlayerCameraManager::SetLetterboxHeight(float Height)
{
	if (ULetterboxModifier* LetterboxModifier = FindModifier<ULetterboxModifier>())
	{
		LetterboxModifier->SetHeight(Height);
	}
}

void APlayerCameraManager::SetLetterboxColor(const FLinearColor& Color)
{
	if (ULetterboxModifier* LetterboxModifier = FindModifier<ULetterboxModifier>())
	{
		LetterboxModifier->SetColor(Color);
	}
}

void APlayerCameraManager::SetLetterboxFadeTime(float InTime, float OutTime)
{
	if (ULetterboxModifier* LetterboxModifier = FindModifier<ULetterboxModifier>())
	{
		LetterboxModifier->SetAlphaInTime(InTime);
		LetterboxModifier->SetAlphaOutTime(OutTime);
	}
}

void APlayerCameraManager::SetVignetteFadeTime(float InTime, float OutTime)
{
	if (UVignetteModifier* VignetteModifier = FindModifier<UVignetteModifier>())
	{
		VignetteModifier->SetAlphaInTime(InTime);
		VignetteModifier->SetAlphaOutTime(OutTime);
	}
}

void APlayerCameraManager::SetGammaFadeTime(float InTime, float OutTime)
{
	if (UGammaCorrectionModifier* GammaModifier = FindModifier<UGammaCorrectionModifier>())
	{
		GammaModifier->SetAlphaInTime(InTime);
		GammaModifier->SetAlphaOutTime(OutTime);
	}
}

UCameraShakeBase* APlayerCameraManager::StartCameraShake(UCameraShakeBase* Shake, float Scale, float Duration)
{
	if (!Shake) return nullptr;
	if (ActiveShakes.Contains(Shake)) return Shake;
	Shake->StartShake(this, Scale, Duration);
	ActiveShakes.Add(Shake);
	UE_LOG("APlayerCameraManager - CameraShakeBase added: {0}", Shake->GetName());
	return Shake;
}

void APlayerCameraManager::StopCameraShake(UCameraShakeBase* Shake, bool bImmediately)
{
	if (!Shake) return;
	auto it = std::find(ActiveShakes.begin(), ActiveShakes.end(), Shake);
	if (it == ActiveShakes.end()) return;
	Shake->StopShake(bImmediately);
	if (bImmediately)
	{
		DeleteObject(*it);
		*it = nullptr;
		ActiveShakes.erase(it);
	}
	UE_LOG("APlayerCameraManager - CameraShakeBase removed: {0}", Shake->GetName());
}

void APlayerCameraManager::StopAllCameraShakes(bool bImmediately)
{
	for (UCameraShakeBase*& Shake : ActiveShakes)
	{
		if (!Shake) continue;
		Shake->StopShake(bImmediately);
	}
	if (bImmediately)
	{
		for (UCameraShakeBase*& Shake : ActiveShakes)
		{
			DeleteObject(Shake);
			Shake = nullptr;
		}
		ActiveShakes.clear();
	}
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
    ModifierList.Sort([](const UCameraModifier* A, const UCameraModifier* B)
    {
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

    FViewTargetTransitionParams Params;
    Params.BlendTime = BlendTime;
    Params.BlendFunc = BlendFunc;
    SetViewTarget(NewViewTarget, Params);
}

void APlayerCameraManager::SetViewTarget(AActor* NewViewTarget, const FViewTargetTransitionParams& Transition)
{
    if (!NewViewTarget)
    {
        UE_LOG("APlayerCameraManager::SetViewTarget(params) - NewViewTarget is null");
        return;
    }

    // 즉시 전환
    if (Transition.BlendTime <= 0.0f || ViewTarget.Target == nullptr)
    {
        ViewTarget.Target = NewViewTarget;
        PendingViewTarget.Target = nullptr;
        bIsBlending = false;
        BlendElapsed = 0.0f;

        if (UCameraComponent* CameraComp = GetViewTargetCameraComponent())
        {
            ViewCache.Location = CameraComp->GetWorldLocation();
            ViewCache.Rotation = CameraComp->GetWorldRotation();
            ViewCache.FOV = CameraComp->GetFOV();
        }
        UE_LOG("APlayerCameraManager - ViewTarget set instantly to: {0}", NewViewTarget->GetName());
        return;
    }

    // 블렌딩 설정
    PendingViewTarget.Target = NewViewTarget;
    TransitionParams = Transition;
    BlendElapsed = 0.0f;
    bIsBlending = true;

    UE_LOG("APlayerCameraManager - ViewTarget blending to: {0}, time: {1}", NewViewTarget->GetName(),
           TransitionParams.BlendTime);
}

UCameraComponent* APlayerCameraManager::GetViewTargetCameraComponent() const
{
	// 블렌딩 중에는 도착지 타겟의 프로젝션 사용을 우선
	AActor* QueryTarget = bIsBlending && PendingViewTarget.Target ? PendingViewTarget.Target : ViewTarget.Target;

	if (!QueryTarget)
	{
		UE_LOG("APlayerCameraManager::GetViewTargetCameraComponent - ViewTarget.Target is null");
		return nullptr;
	}

	UCameraComponent* CameraComp = QueryTarget->GetComponent<UCameraComponent>();
	if (!CameraComp)
	{
		UE_LOG("APlayerCameraManager::GetViewTargetCameraComponent - CameraComponent not found on ViewTarget: %c", 
			QueryTarget->GetName());
	}
	else
	{
		UE_LOG("APlayerCameraManager::GetViewTargetCameraComponent - Found CameraComponent on ViewTarget: %c", 
			QueryTarget->GetName());
	}

    return CameraComp;
}

FPostProcessSettings APlayerCameraManager::GetPostProcessSettings() const
{
	FPostProcessSettings Settings;

	// 모든 CameraModifier에게 후처리 기여 요청
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

void APlayerCameraManager::InitializeModifiers()
{
	UE_LOG("=== PlayerCameraManager: Initializing Modifiers ===");

	// 1. Fade Modifier 추가
	UFadeModifier* FadeModifier = NewObject<UFadeModifier>();
	if (FadeModifier)
	{
		AddCameraModifier(FadeModifier);
		FadeModifier->DisableModifier(true);
		UE_LOG("  [OK] FadeModifier added");
	}

	// 2. Vignette Modifier 추가
	UVignetteModifier* VignetteModifier = NewObject<UVignetteModifier>();
	if (VignetteModifier)
	{
		AddCameraModifier(VignetteModifier);
		VignetteModifier->DisableModifier(true);
		UE_LOG("  [OK] VignetteModifier added");
	}

	// 3. Gamma Correction Modifier 추가
	UGammaCorrectionModifier* GammaModifier = NewObject<UGammaCorrectionModifier>();
	if (GammaModifier)
	{
		AddCameraModifier(GammaModifier);
		GammaModifier->DisableModifier(true);
		UE_LOG("  [OK] GammaCorrectionModifier added");
	}

	// 4. Letterbox Modifier 추가
	ULetterboxModifier* LetterboxModifier = NewObject<ULetterboxModifier>();
	if (LetterboxModifier)
	{
		AddCameraModifier(LetterboxModifier);
		LetterboxModifier->DisableModifier(true);
		UE_LOG("  [OK] LetterboxModifier added");
	}
}

void APlayerCameraManager::UpdateCamera(float DeltaTime)
{
    // 0. ViewTarget / Blending 업데이트
    UpdateViewTarget(DeltaTime);

	// 2. 모든 모디파이어의 Alpha 및 상태 업데이트
	// ViewTarget이 없어도 모디파이어 업데이트는 진행 (Fade 등)
	for (UCameraModifier* Modifier : ModifierList)
	{
		if (Modifier)
		{
			// Fade 모디파이어는 별도로 UpdateFade() 호출
			if (UFadeModifier* FadeModifier = Cast<UFadeModifier>(Modifier))
			{
				FadeModifier->UpdateFade(DeltaTime);
			}

			// Alpha 페이드 인/아웃 업데이트 (모든 모디파이어 공통)
			Modifier->UpdateAlpha(DeltaTime);

			// 활성화된 모디파이어만 카메라에 적용
			if (Modifier->IsEnabled())
			{
				Modifier->ModifyCamera(DeltaTime, ViewCache.Location, ViewCache.Rotation, ViewCache.FOV);
			}
		}
	}

	// 3. 카메라 셰이크: offset를 업데이트하고 적용
	if (!ActiveShakes.empty())
	{
		FVector AccumLoc(0,0,0);
		FQuat   AccumRot(0,0,0,1);
		float   AccumFOV = 0.0f;

		bool bHasRot = false;
		for (int i = 0; i < (int)ActiveShakes.size(); ++i)
		{
			UCameraShakeBase* Shake = ActiveShakes[i];
			if (!Shake) continue;
			FCameraShakeUpdateResult R = Shake->UpdateShake(DeltaTime);
			AccumLoc = AccumLoc + R.LocationOffset;
			AccumFOV += R.FOVOffset;
			if (R.RotationOffset.W != 1.0f || R.RotationOffset.X != 0.0f || R.RotationOffset.Y != 0.0f || R.RotationOffset.Z != 0.0f)
			{
				AccumRot = R.RotationOffset.GetNormalized() * AccumRot;
				bHasRot = true;
			}
		}

		ViewCache.Location = ViewCache.Location + AccumLoc;
		if (bHasRot)
		{
			ViewCache.Rotation = AccumRot.GetNormalized() * ViewCache.Rotation;
		}
		ViewCache.FOV += AccumFOV;

		// Remove finished shakes
		for (int i = (int)ActiveShakes.size() - 1; i >= 0; --i)
		{
			UCameraShakeBase* Shake = ActiveShakes[i];
			if (!Shake) continue;
			if (Shake->IsFinished())
			{
				DeleteObject(Shake);
				ActiveShakes.erase(ActiveShakes.begin() + i);
			}
		}
	}
}

static float EvalBlendAlpha(float T, ECameraBlendType Type)
{
    T = FMath::Clamp(T, 0.0f, 1.0f);
    switch (Type)
    {
    case ECameraBlendType::EaseIn:
        // Quadratic ease-in: t^2
        return T * T;
    case ECameraBlendType::EaseOut:
        // Quadratic ease-out: 1 - (1-t)^2
        return 1.0f - (1.0f - T) * (1.0f - T);
    case ECameraBlendType::EaseInOut:
        // Smoothstep: t^2(3 - 2t)
        return T * T * (3.0f - 2.0f * T);
    case ECameraBlendType::Linear:
    default:
        return T;
    }
}

void APlayerCameraManager::UpdateViewTarget(float DeltaTime)
{
    // 기본값: 현재 또는 보류 중인 타겟의 카메라 컴포넌트를 조회해 ViewCache 설정
    if (!bIsBlending)
    {
        if (UCameraComponent* CameraComp = GetViewTargetCameraComponent())
        {
            ViewCache.Location = CameraComp->GetWorldLocation();
            ViewCache.Rotation = CameraComp->GetWorldRotation();
            ViewCache.FOV = CameraComp->GetFOV();
        }
        return;
    }

    // 블렌딩 진행
    BlendElapsed += DeltaTime;
    float Alpha = TransitionParams.BlendTime > 0.0f ? (BlendElapsed / TransitionParams.BlendTime) : 1.0f;
    if (Alpha >= 1.0f)
    {
        // 블렌딩 완료
        ViewTarget = PendingViewTarget;
        PendingViewTarget.Target = nullptr;
        bIsBlending = false;
        BlendElapsed = 0.0f;

        if (UCameraComponent* CameraComp = GetViewTargetCameraComponent())
        {
            ViewCache.Location = CameraComp->GetWorldLocation();
            ViewCache.Rotation = CameraComp->GetWorldRotation();
            ViewCache.FOV = CameraComp->GetFOV();
        }
        return;
    }

    Alpha = EvalBlendAlpha(Alpha, TransitionParams.BlendFunc);

    // 출발/도착 카메라 컴포넌트 조회 (매 프레임 샘플링하여 움직임 반영)
    UCameraComponent* FromCam = ViewTarget.Target ? ViewTarget.Target->GetComponent<UCameraComponent>() : nullptr;
    UCameraComponent* ToCam = PendingViewTarget.Target
                                  ? PendingViewTarget.Target->GetComponent<UCameraComponent>()
                                  : nullptr;

    if (!FromCam && ToCam)
    {
        // 출발지가 없으면 도착지만 사용
        ViewCache.Location = ToCam->GetWorldLocation();
        ViewCache.Rotation = ToCam->GetWorldRotation();
        ViewCache.FOV = ToCam->GetFOV();
        return;
    }
    if (!ToCam && FromCam)
    {
        // 도착지가 없으면 출발지만 사용
        ViewCache.Location = FromCam->GetWorldLocation();
        ViewCache.Rotation = FromCam->GetWorldRotation();
        ViewCache.FOV = FromCam->GetFOV();
        return;
    }
    if (!FromCam && !ToCam)
    {
        return;
    }

    const FVector FromLoc = FromCam->GetWorldLocation();
    const FQuat FromRot = FromCam->GetWorldRotation();
    const float FromFOV = FromCam->GetFOV();

    const FVector ToLoc = ToCam->GetWorldLocation();
    const FQuat ToRot = ToCam->GetWorldRotation();
    const float ToFOV = ToCam->GetFOV();

    ViewCache.Location = FVector::Lerp(FromLoc, ToLoc, Alpha);
    ViewCache.Rotation = FQuat::Slerp(FromRot, ToRot, Alpha).GetNormalized();
    ViewCache.FOV = FMath::Lerp(FromFOV, ToFOV, Alpha);
}
