#pragma once
#include "Object.h"
#include "Vector.h"
#include "CameraShakeBase.h" // for FCameraShakeUpdateParams/Result

class APlayerCameraManager;

class UCameraShakePattern : public UObject
{
public:
    DECLARE_CLASS(UCameraShakePattern, UObject)
    GENERATED_REFLECTION_BODY()

    UCameraShakePattern();
    virtual ~UCameraShakePattern() override = default;

    // Lifecycle managed by the owner (e.g., UCameraShakeBase)
    virtual void StartShake(APlayerCameraManager* InCameraManager, float InScale = 1.0f, float InDuration = 0.0f);
    virtual void StopShake(bool bImmediately = false);

    // Per-frame update; returns the offsets to apply
    FCameraShakeUpdateResult UpdateShake(float DeltaTime);

    // State queries
    bool IsActive() const { return bIsActive; }
    bool IsFinished() const { return bIsFinished; }
    float GetElapsedTime() const { return ElapsedTime; }
    float GetPlayScale()  const { return PlayScale; }

    // Config (editable defaults)
    void SetDuration(float InSeconds) { Duration = InSeconds; }
    void SetBlendInTime(float InSeconds) { BlendInTime = InSeconds; }
    void SetBlendOutTime(float InSeconds) { BlendOutTime = InSeconds; }
    void SetPlayScale(float InScale) { PlayScale = InScale; }

protected:
    // Overridable core: generate raw offsets for this frame before blending/scaling
    virtual void OnStart() {}
    virtual void OnStop(bool /*bImmediately*/) {}
    virtual void OnUpdate(const FCameraShakeUpdateParams& /*Params*/, FCameraShakeUpdateResult& OutResult) { OutResult = FCameraShakeUpdateResult(); }

    // Compute current blend alpha (0..1) based on elapsed, blend-in/out and duration
    float ComputeBlendAlpha(float Elapsed, float InDuration) const;

protected:
    // Runtime state
    APlayerCameraManager* CameraManager = nullptr; // not owned
    bool  bIsActive = false;
    bool  bIsFinished = false;
    float ElapsedTime = 0.0f;        // time since StartShake

    // Play parameters
    float Duration = 0.0f;           // <=0 means indefinite until StopShake
    float BlendInTime = 0.1f;        // seconds
    float BlendOutTime = 0.2f;       // seconds
    float PlayScale = 1.0f;          // global scale
};

