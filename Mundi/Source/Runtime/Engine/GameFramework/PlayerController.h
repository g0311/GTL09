#pragma once
#include "Actor.h"

class UCameraComponent;
class FViewport;
class UInputMappingContext;
class APlayerCameraManager;

/**
 * @brief 플레이어 입력을 처리하고 카메라를 제어하는 컨트롤러 클래스
 *
 * PlayerController는 플레이어의 시점을 담당하며, Pawn을 빙의(Possess)하거나
 * ViewTarget을 설정하여 특정 액터의 카메라 컴포넌트 시점으로 렌더링
 */
class APlayerController : public AActor
{
public:
    DECLARE_CLASS(APlayerController, AActor)
    GENERATED_REFLECTION_BODY()

    APlayerController();

protected:
    virtual ~APlayerController() override;

public:
    void BeginPlay() override;
    void Tick(float DeltaSeconds) override;
    void EndPlay(EEndPlayReason Reason) override;
    
    /**
     * @brief 특정 액터에 빙의 (해당 액터의 카메라 컴포넌트 사용)
     * @param InPawn 빙의할 액터
     */
    void Possess(AActor* InPawn);

    /**
     * @brief 현재 빙의 중인 액터를 해제
     */
    void UnPossess();

    /**
     * @brief 현재 ViewTarget을 설정
     * @param NewViewTarget 새로운 ViewTarget 액터
     * @param BlendTime 블렌드 시간 (PlayerCameraManager에 전달)
     */
    void SetViewTarget(AActor* NewViewTarget, float BlendTime = 0.0f);

    /**
     * @brief 뷰타겟을 블렌드 파라미터와 함께 설정
     */
    void SetViewTargetWithBlend(AActor* NewViewTarget, float BlendTime, ECameraBlendType BlendFunc = ECameraBlendType::Linear);

    /**
     * @brief 현재 ViewTarget을 반환
     * @return 현재 ViewTarget 액터 포인터 (PlayerCameraManager에서 가져옴)
     */
    AActor* GetViewTarget() const;

    /**
     * @brief 현재 빙의 중인 Pawn을 반환
     * @return 빙의 중인 Pawn 포인터
     */
    AActor* GetPawn() const { return PossessedPawn; }

    /**
     * @brief PlayerCameraManager를 반환
     * @return PlayerCameraManager 포인터
     */
    APlayerCameraManager* GetPlayerCameraManager() const { return PlayerCameraManager; }

    /**
     * @brief 현재 사용 중인 카메라 컴포넌트를 반환 (PlayerCameraManager를 통해)
     * @return 카메라 컴포넌트 포인터 (없으면 nullptr)
     */
    UCameraComponent* GetActiveCameraComponent() const;

    FMatrix GetViewMatrix() const;

    FMatrix GetProjectionMatrix() const;

    FMatrix GetProjectionMatrix(float ViewportAspectRatio) const;

    FMatrix GetProjectionMatrix(float ViewportAspectRatio, FViewport* Viewport) const;

    FVector GetCameraLocation() const;

    FQuat GetCameraRotation() const;

    /**
     * @brief Fade Out 시작 (PlayerCameraManager에 위임)
     */
    void StartCameraFade(float Duration, FLinearColor ToColor = FLinearColor(0, 0, 0, 1), bool bFadeOut = true);

    // ───── 입력 관련 ────────────────────────────
    /**
     * @brief 입력 컨텍스트를 반환 (Lua 스크립트에서 입력 매핑 설정용)
     * @return 입력 컨텍스트 포인터
     */
    UInputMappingContext* GetInputContext() const { return InputContext; }

    /**
     * @brief 입력 컨텍스트를 초기화하고 InputMappingSubsystem에 등록
     */
    void SetupInputContext();

    // ───── 직렬화 관련 ────────────────────────────
    void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

private:
    /** 현재 빙의 중인 Pawn */
    AActor* PossessedPawn = nullptr;

    /** 플레이어 카메라 매니저 (카메라 효과, Fade, Modifier 관리) */
    APlayerCameraManager* PlayerCameraManager = nullptr;

    /** 플레이어 입력 컨텍스트 (Lua 스크립트에서 입력 매핑 설정) */
    UInputMappingContext* InputContext = nullptr;
};
