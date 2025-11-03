#pragma once
#include "Actor.h"

class UCameraComponent;
class FViewport;
class UInputMappingContext;

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
    ~APlayerController() override;

public:
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
     * @param BlendTime 블렌드 시간 (현재 미사용)
     */
    void SetViewTarget(AActor* NewViewTarget, float BlendTime = 0.0f);

    /**
     * @brief 현재 ViewTarget을 반환
     * @return 현재 ViewTarget 액터 포인터
     */
    AActor* GetViewTarget() const { return ViewTarget; }

    /**
     * @brief 현재 빙의 중인 Pawn을 반환
     * @return 빙의 중인 Pawn 포인터
     */
    AActor* GetPawn() const { return PossessedPawn; }

    /**
     * @brief 현재 사용 중인 카메라 컴포넌트를 반환
     * ViewTarget이 카메라 컴포넌트를 가지고 있으면 해당 컴포넌트를,
     * 없으면 PossessedPawn의 카메라 컴포넌트를 반환
     * @return 카메라 컴포넌트 포인터 (없으면 nullptr)
     */
    UCameraComponent* GetActiveCameraComponent() const;

    /**
     * @brief 매 프레임 호출되는 틱 함수
     */
    void Tick(float DeltaSeconds) override;

    /**
     * @brief View 행렬을 반환
     */
    FMatrix GetViewMatrix() const;

    /**
     * @brief Projection 행렬을 반환
     */
    FMatrix GetProjectionMatrix() const;

    /**
     * @brief Projection 행렬을 반환
     */
    FMatrix GetProjectionMatrix(float ViewportAspectRatio) const;

    /**
     * @brief Projection 행렬을 반환
     */
    FMatrix GetProjectionMatrix(float ViewportAspectRatio, FViewport* Viewport) const;

    /**
     * @brief 카메라 위치를 반환
     */
    FVector GetCameraLocation() const;

    /**
     * @brief 카메라 회전을 반환
     */
    FQuat GetCameraRotation() const;

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

    /**
     * @brief BeginPlay 시 호출 (입력 컨텍스트 설정)
     */
    void BeginPlay() override;

    /**
     * @brief EndPlay 시 호출 (입력 컨텍스트 정리)
     */
    void EndPlay(EEndPlayReason Reason) override;

    // ───── 직렬화 관련 ────────────────────────────
    void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

private:
    /** 현재 빙의 중인 Pawn */
    AActor* PossessedPawn = nullptr;

    /** 현재 시점을 제공하는 액터 (기본적으로 PossessedPawn과 동일) */
    AActor* ViewTarget = nullptr;

    /** 플레이어 입력 컨텍스트 (Lua 스크립트에서 입력 매핑 설정) */
    UInputMappingContext* InputContext = nullptr;
};
