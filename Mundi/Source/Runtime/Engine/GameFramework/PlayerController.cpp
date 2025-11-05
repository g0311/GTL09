#include "pch.h"
#include "PlayerController.h"
#include "Actor.h"
#include "CameraComponent.h"
#include "ActorComponent.h"
#include "PlayerCameraManager.h"
#include "World.h"
#include "Source/Runtime/InputCore/InputMappingContext.h"
#include "Source/Runtime/InputCore/InputMappingSubsystem.h"

IMPLEMENT_CLASS(APlayerController)

BEGIN_PROPERTIES(APlayerController)
END_PROPERTIES()

APlayerController::APlayerController()
    : PossessedPawn(nullptr)
    , PlayerCameraManager(nullptr)
{
    Name = "PlayerController";
    bTickInEditor = false; // 게임 중에만 틱

    // InputContext를 생성자에서 미리 생성
    // (ScriptComponent BeginPlay가 PlayerController BeginPlay보다 먼저 호출될 수 있으므로)
    InputContext = NewObject<UInputMappingContext>();
}

APlayerController::~APlayerController()
{
    // 소멸자에서는 즉시 제거 (Dangling pointer 방지)
    if (InputContext)
    {
        UInputMappingSubsystem::Get().RemoveMappingContextImmediate(InputContext);
        InputContext = nullptr;
    }

    // PlayerCameraManager는 World의 Actor로 관리되므로 World에서 제거됨
}

void APlayerController::BeginPlay()
{
    Super::BeginPlay();

    // 입력 컨텍스트 설정
    SetupInputContext();
}

void APlayerController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    // PlayerCameraManager는 자체적으로 Tick되므로 별도 호출 불필요
}

void APlayerController::EndPlay(EEndPlayReason Reason)
{
    // 입력 컨텍스트 정리
    if (InputContext)
    {
        UInputMappingSubsystem::Get().RemoveMappingContext(InputContext);
        InputContext = nullptr;
    }

    // PlayerCameraManager는 World의 Actor로 스폰되었으므로
    // World 소멸 시 자동으로 정리됨 (이중 삭제 방지)
    // 여기서는 nullptr만 설정
    PlayerCameraManager = nullptr;

    Super::EndPlay(Reason);
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
        // ViewTarget이 PossessedPawn이면 해제
        if (PlayerCameraManager && PlayerCameraManager->GetViewTarget() == PossessedPawn)
        {
            PlayerCameraManager->SetViewTarget(nullptr);
        }

        PossessedPawn = nullptr;
    }
}

void APlayerController::SetViewTarget(AActor* NewViewTarget, float BlendTime)
{
    if (!PlayerCameraManager)
    {
        UE_LOG("APlayerController::SetViewTarget - PlayerCameraManager is null");
        return;
    }

    // PlayerCameraManager에 위임
    PlayerCameraManager->SetViewTarget(NewViewTarget, BlendTime);
}

AActor* APlayerController::GetViewTarget() const
{
    if (!PlayerCameraManager)
    {
        return nullptr;
    }

    return PlayerCameraManager->GetViewTarget();
}

UCameraComponent* APlayerController::GetActiveCameraComponent() const
{
    if (!PlayerCameraManager)
    {
        return nullptr;
    }

    return PlayerCameraManager->GetViewTargetCameraComponent();
}

FMatrix APlayerController::GetViewMatrix() const
{
    if (!PlayerCameraManager)
    {
        return FMatrix::Identity();
    }

    return PlayerCameraManager->GetViewMatrix();
}

FMatrix APlayerController::GetProjectionMatrix() const
{
    if (!PlayerCameraManager)
    {
        return FMatrix::Identity();
    }

    // ViewportAspectRatio는 CameraComponent가 자동 계산
    return PlayerCameraManager->GetProjectionMatrix(nullptr);
}

FMatrix APlayerController::GetProjectionMatrix(float ViewportAspectRatio) const
{
    if (!PlayerCameraManager)
    {
        return FMatrix::Identity();
    }

    return PlayerCameraManager->GetProjectionMatrix(nullptr);
}

FMatrix APlayerController::GetProjectionMatrix(float ViewportAspectRatio, FViewport* Viewport) const
{
    if (!PlayerCameraManager)
    {
        return FMatrix::Identity();
    }

    return PlayerCameraManager->GetProjectionMatrix(Viewport);
}

FVector APlayerController::GetCameraLocation() const
{
    if (!PlayerCameraManager)
    {
        return FVector::Zero();
    }

    return PlayerCameraManager->GetCameraLocation();
}

FQuat APlayerController::GetCameraRotation() const
{
    if (!PlayerCameraManager)
    {
        return FQuat::Identity();
    }

    return PlayerCameraManager->GetCameraRotation();
}

void APlayerController::StartCameraFade(float Duration, FLinearColor ToColor, bool bFadeOut)
{
    if (!PlayerCameraManager)
    {
        UE_LOG("APlayerController::StartCameraFade - PlayerCameraManager is null");
        return;
    }

    if (bFadeOut)
    {
        PlayerCameraManager->StartFadeOut(Duration, ToColor);
    }
    else
    {
        PlayerCameraManager->StartFadeIn(Duration, ToColor);
    }
}

void APlayerController::SetupInputContext()
{
    // InputContext는 생성자에서 이미 생성됨
    // 여기서는 InputMappingSubsystem에 등록만 함
    if (InputContext)
    {
        UInputMappingSubsystem::Get().AddMappingContext(InputContext, 0);
    }
}

void APlayerController::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    Super::Serialize(bInIsLoading, InOutHandle);

    // TODO: PossessedPawn, ViewTarget 직렬화
    // 액터 참조는 나중에 GUID나 이름으로 저장/로드 필요
}
