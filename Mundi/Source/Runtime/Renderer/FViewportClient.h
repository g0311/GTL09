#pragma once
#include "Vector.h"
#include"Enums.h"

class FViewport;
class UWorld;
class UCameraComponent;


/**
 * @brief 뷰포트 클라이언트 - UE의 FViewportClient를 모방
 */
class FViewportClient
{
public:
    FViewportClient();
    virtual ~FViewportClient();

    // 렌더링
    virtual void Draw(FViewport* Viewport);
    virtual void Tick(float DeltaTime);

    // 입력 처리
    virtual void MouseWheel(float DeltaSeconds);

    // 입력 컨텍스트 관리
    void SetupInputContext();
    void OnFocusGained();
    void OnFocusLost();

    // 입력 컨텍스트 (public - USlateManager에서 포커스 관리 위해 필요)
    class UInputMappingContext* ViewportInputContext = nullptr;

    // 뷰포트 설정
    void SetViewportType(EViewportType InType) { ViewportType = InType; }
    EViewportType GetViewportType() const { return ViewportType; }

    void SetWorld(UWorld* InWorld);
    UWorld* GetWorld() const { return World; }

    void SetCamera(ACameraActor* InCamera) { Camera = InCamera; }
    ACameraActor* GetCamera() const { return Camera; }

    void SetViewport(FViewport* InViewport) { Viewport = InViewport; }
    FViewport* GetViewport() const { return Viewport; }

    // 카메라 매트릭스 계산
    FMatrix GetViewMatrix() const;


    // 뷰포트별 카메라 설정
    void SetupCameraMode();
    void SetViewModeIndex(EViewModeIndex InViewModeIndex) { ViewModeIndex = InViewModeIndex; }

    EViewModeIndex GetViewModeIndex() { return ViewModeIndex;}


protected:
    EViewportType ViewportType = EViewportType::Perspective;
    UWorld* World = nullptr;
    ACameraActor* Camera = nullptr;
    FViewport* Viewport = nullptr;
    bool bIsMouseRightButtonDown = false;
    static FVector CameraAddPosition;


    // 직교 뷰용 카메라 설정
    uint32 OrthographicAddXPosition;
    uint32  OrthographicAddYPosition;
    float OrthographicZoom = 30.0f;
    //뷰모드
    EViewModeIndex ViewModeIndex = EViewModeIndex::VMI_Lit;

    //원근 투영 기본값
    bool PerspectiveCameraInput = false;
    FVector PerspectiveCameraPosition = FVector(-5.0f, 5.0f, 5.0f);
    FVector PerspectiveCameraRotation = FVector(0.0f, 22.5f, -45.0f);
    float PerspectiveCameraFov=60;

    // PIE Eject 모드 (F8) - PIE 중 에디터 카메라 제어
    bool bPIEEjected = false;
};