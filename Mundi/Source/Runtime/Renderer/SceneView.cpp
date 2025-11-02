#include "pch.h"
#include "SceneView.h"
#include "CameraActor.h"
#include "FViewport.h"
#include "Frustum.h"

FSceneView::FSceneView(UCameraComponent* InCameraComponent, FViewport* InViewport, EViewModeIndex InViewMode)
{
    if (!InCameraComponent || !InViewport)
    {
        UE_LOG("[FSceneView::FSceneView()]: CameraComponent 또는 InViewport 가 없습니다.");
        return;
    }

    // --- 이 로직이 FSceneRenderer::PrepareView()에서 이동해 옴 ---

    float AspectRatio = 1.0f;
    if (InViewport->GetSizeY() > 0)
    {
        AspectRatio = (float)InViewport->GetSizeX() / (float)InViewport->GetSizeY();
    }

    ViewMatrix = InCameraComponent->GetViewMatrix();
    ProjectionMatrix = InCameraComponent->GetProjectionMatrix(AspectRatio, InViewport);
    ViewFrustum = CreateFrustumFromCamera(*InCameraComponent, AspectRatio);
    ViewLocation = InCameraComponent->GetWorldLocation();
    ZNear = InCameraComponent->GetNearClip();
    ZFar = InCameraComponent->GetFarClip();

    ViewRect.MinX = InViewport->GetStartX();
    ViewRect.MinY = InViewport->GetStartY();
    ViewRect.MaxX = ViewRect.MinX + InViewport->GetSizeX();
    ViewRect.MaxY = ViewRect.MinY + InViewport->GetSizeY();

    // ViewMode는 World나 RenderSettings에서 가져와야 함 (임시)
    ViewMode = InViewMode;
    ProjectionMode = InCameraComponent->GetProjectionMode();

    Camera = InCameraComponent;
    Viewport = InViewport;
}