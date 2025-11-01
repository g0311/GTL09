#include "pch.h"
#include "PlayerController.h"
#include "Actor.h"
#include "CameraComponent.h"
#include "ActorComponent.h"

IMPLEMENT_CLASS(APlayerController)

BEGIN_PROPERTIES(APlayerController)
END_PROPERTIES()

APlayerController::APlayerController()
    : PossessedPawn(nullptr)
    , ViewTarget(nullptr)
{
    Name = "PlayerController";
    bTickInEditor = false; // 게임 중에만 틱
}

APlayerController::~APlayerController()
{
}

void APlayerController::Possess(AActor* InPawn)
{
    if (PossessedPawn == InPawn)
    {
        return;
    }

    // 이전 Pawn 해제
    UnPossess();

    // 새 Pawn 빙의
    PossessedPawn = InPawn;

    // ViewTarget을 자동으로 새 Pawn으로 설정
    SetViewTarget(InPawn);
}

void APlayerController::UnPossess()
{
    if (PossessedPawn)
    {
        PossessedPawn = nullptr;
    }

    // ViewTarget도 해제
    if (ViewTarget == PossessedPawn)
    {
        ViewTarget = nullptr;
    }
}

void APlayerController::SetViewTarget(AActor* NewViewTarget, float BlendTime)
{
    if (ViewTarget == NewViewTarget)
    {
        return;
    }

    ViewTarget = NewViewTarget;

    // TODO: BlendTime을 사용한 부드러운 카메라 전환 구현
}

UCameraComponent* APlayerController::GetActiveCameraComponent() const
{
    // 1. ViewTarget이 있으면 ViewTarget의 카메라 컴포넌트 찾기
    AActor* TargetActor = ViewTarget ? ViewTarget : PossessedPawn;

    if (!TargetActor)
    {
        return nullptr;
    }

    // 2. 액터의 모든 컴포넌트를 순회하며 카메라 컴포넌트 찾기
    const TSet<UActorComponent*>& Components = TargetActor->GetOwnedComponents();
    for (UActorComponent* Component : Components)
    {
        if (UCameraComponent* CameraComp = Cast<UCameraComponent>(Component))
        {
            return CameraComp;
        }
    }

    return nullptr;
}

void APlayerController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    // 여기서 입력 처리 등을 할 수 있습니다
}

FMatrix APlayerController::GetViewMatrix() const
{
    UCameraComponent* CameraComp = GetActiveCameraComponent();
    if (CameraComp)
    {
        return CameraComp->GetViewMatrix();
    }

    // 카메라가 없으면 Identity 반환
    return FMatrix::Identity();
}

FMatrix APlayerController::GetProjectionMatrix() const
{
    UCameraComponent* CameraComp = GetActiveCameraComponent();
    if (CameraComp)
    {
        return CameraComp->GetProjectionMatrix();
    }

    return FMatrix::Identity();
}

FMatrix APlayerController::GetProjectionMatrix(float ViewportAspectRatio) const
{
    UCameraComponent* CameraComp = GetActiveCameraComponent();
    if (CameraComp)
    {
        return CameraComp->GetProjectionMatrix(ViewportAspectRatio);
    }

    return FMatrix::Identity();
}

FMatrix APlayerController::GetProjectionMatrix(float ViewportAspectRatio, FViewport* Viewport) const
{
    UCameraComponent* CameraComp = GetActiveCameraComponent();
    if (CameraComp)
    {
        return CameraComp->GetProjectionMatrix(ViewportAspectRatio, Viewport);
    }

    return FMatrix::Identity();
}

FVector APlayerController::GetCameraLocation() const
{
    UCameraComponent* CameraComp = GetActiveCameraComponent();
    if (CameraComp)
    {
        return CameraComp->GetWorldTransform().Translation;
    }

    // 카메라가 없으면 ViewTarget 또는 Pawn의 위치 반환
    AActor* TargetActor = ViewTarget ? ViewTarget : PossessedPawn;
    if (TargetActor)
    {
        return TargetActor->GetActorLocation();
    }

    return FVector::Zero();
}

FQuat APlayerController::GetCameraRotation() const
{
    UCameraComponent* CameraComp = GetActiveCameraComponent();
    if (CameraComp)
    {
        return CameraComp->GetWorldTransform().Rotation;
    }

    // 카메라가 없으면 ViewTarget 또는 Pawn의 회전 반환
    AActor* TargetActor = ViewTarget ? ViewTarget : PossessedPawn;
    if (TargetActor)
    {
        return TargetActor->GetActorRotation();
    }

    return FQuat::Identity();
}

void APlayerController::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    Super::Serialize(bInIsLoading, InOutHandle);

    // TODO: PossessedPawn, ViewTarget 직렬화
    // 액터 참조는 나중에 GUID나 이름으로 저장/로드 필요
}
