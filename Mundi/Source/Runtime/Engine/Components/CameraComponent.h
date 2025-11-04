#pragma once
#include "SceneComponent.h"

enum class ECameraProjectionMode
{
    Perspective,
    Orthographic
};

class UCameraComponent : public USceneComponent
{
public:
    DECLARE_CLASS(UCameraComponent, USceneComponent)
    GENERATED_REFLECTION_BODY()

    UCameraComponent();

protected:
    ~UCameraComponent() override;

public:
    // Projection settings
    void SetFOV(float NewFOV) { FieldOfView = NewFOV; }
    void SetAspectRatio(float NewAspect) { AspectRatio = NewAspect; }
    void SetClipPlanes(float NewNear, float NewFar) { NearClip = NewNear; FarClip = NewFar; }
    void SetNearClipPlane(float NewNear) { NearClip = NewNear; }
    void SetFarClipPlane(float NewFar) { FarClip = NewFar; }
    void SetProjectionMode(ECameraProjectionMode Mode) { ProjectionMode = Mode; }
    void SetZoomFactor(float InZoomFactor) { ZooMFactor = InZoomFactor; };
    
    float GetFOV() const { return FieldOfView; }
    float GetAspectRatio() const { return AspectRatio; }
    float GetNearClip() const { return NearClip; }
    float GetFarClip() const { return FarClip; }
    float GetZoomFactor()const { return ZooMFactor; };
    ECameraProjectionMode GetProjectionMode() const { return ProjectionMode; }

    // Post-process effects
    // Vignetting
    void SetVignettingEnabled(bool bEnabled) { bEnableVignetting = bEnabled; }
    void SetVignettingIntensity(float Intensity) { VignettingIntensity = Intensity; }
    void SetVignettingSmoothness(float Smoothness) { VignettingSmoothness = Smoothness; }
    bool IsVignettingEnabled() const { return bEnableVignetting; }
    float GetVignettingIntensity() const { return VignettingIntensity; }
    float GetVignettingSmoothness() const { return VignettingSmoothness; }

    // Gamma Correction
    void SetGammaCorrectionEnabled(bool bEnabled) { bEnableGammaCorrection = bEnabled; }
    void SetGammaValue(float Gamma) { GammaValue = Gamma; }
    bool IsGammaCorrectionEnabled() const { return bEnableGammaCorrection; }
    float GetGammaValue() const { return GammaValue; }

    // Letterbox
    void SetLetterboxEnabled(bool bEnabled) { bEnableLetterbox = bEnabled; }
    void SetLetterboxHeight(float Height) { LetterboxHeight = Height; }
    void SetLetterboxColor(const FVector& Color) { LetterboxColor = Color; }
    bool IsLetterboxEnabled() const { return bEnableLetterbox; }
    float GetLetterboxHeight() const { return LetterboxHeight; }
    FVector GetLetterboxColor() const { return LetterboxColor; }


    // Matrices
    FMatrix GetViewMatrix() const;
    FMatrix GetProjectionMatrix() const;
    FMatrix GetProjectionMatrix(float ViewportAspectRatio) const; //사용 x
    FMatrix GetProjectionMatrix(float ViewportAspectRatio, FViewport* Viewport) const; //ViewportAspectRatio는 Viewport에서 얻어올 수 있음

    TArray<FVector> GetFrustumVertices(FViewport* Viewport) const;
    TArray<FVector> GetFrustumVerticesCascaded(FViewport* Viewport, const float Near, const float Far) const;
    TArray<float> GetCascadedSliceDepth(int CascadedCount, float LinearBlending = 0.5f) const;

    void SetViewGizmo()
    {
        bSetViewGizmo = true;
    }
    const TArray<FVector>& GetViewGizmo() const
    {
        return ViewGizmo;
    }
    // Directions in world space
    FVector GetForward() const;
    FVector GetRight() const;
    FVector GetUp() const;

    // ───── 복사 관련 ────────────────────────────
    void DuplicateSubObjects() override;
    DECLARE_DUPLICATE(UCameraComponent)

    // Serialization
    virtual void OnSerialized() override;
    virtual void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

private:
    float FieldOfView;   // degrees
    float AspectRatio;  //사용x
    float NearClip;
    float FarClip;

    float ZooMFactor;

    ECameraProjectionMode ProjectionMode;

    bool bSetViewGizmo;
    TArray<FVector> ViewGizmo;

    // Post-process effect properties
    // Vignetting
    bool bEnableVignetting{ false };
    float VignettingIntensity{ 0.5f };     // 0.0 ~ 1.0
    float VignettingSmoothness{ 0.5f };    // 0.0 ~ 1.0

    // Gamma Correction
    bool bEnableGammaCorrection{ false };
    float GammaValue{ 2.2f };              // Standard gamma value

    // Letterbox
    bool bEnableLetterbox{ false };
    float LetterboxHeight{ 0.1f };         // 0.0 ~ 1.0 (비율)
    FVector LetterboxColor{ 0.0f, 0.0f, 0.0f };  // RGB (기본: 검정)
};

