#pragma once
#include "Object.h"
#include "Vector.h"

class APlayerCameraManager;
class UCameraShakePattern;

// Parameters provided to a camera shake on update
struct FCameraShakeUpdateParams
{
    float DeltaTime = 0.0f;
    float ElapsedTime = 0.0f;     // time since StartShake
    float Duration = 0.0f;        // nominal play time (not including blend-out)
    float PlayScale = 1.0f;       // overall scaling factor
};

// Offsets produced by a camera shake for this frame
struct FCameraShakeUpdateResult
{
    FVector LocationOffset{ 0,0,0 };
    FQuat   RotationOffset{ 0,0,0,1 };
    float   FOVOffset = 0.0f;     // in degrees
};

// Parameters to start a camera shake (creation/start overrides)
struct FCameraShakeStartParams
{
    float Scale = 1.0f;
    float Duration = 0.0f;    // 0 = use shake default (or infinite)
    float BlendInTime = 0.1f;
    float BlendOutTime = 0.2f;
};

// Base class for camera shakes (pattern-driven in derived classes)
class UCameraShakeBase : public UObject
{
public:
    DECLARE_CLASS(UCameraShakeBase, UObject)
    GENERATED_REFLECTION_BODY()

    UCameraShakeBase();
    ~UCameraShakeBase();

    // Lifecycle
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
    // Overridable core: generate offsets for this frame before blending/scaling
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

    UCameraShakePattern* RootPattern = nullptr; // may be owned
    bool bOwnsPattern = false;

public:
    void SetRootPattern(UCameraShakePattern* InPattern, bool bTakeOwnership = false)
    {
        if (RootPattern && bOwnsPattern && RootPattern != InPattern)
        {
            delete RootPattern;
        }
        RootPattern = InPattern;
        bOwnsPattern = bTakeOwnership;
    }
    UCameraShakePattern* GetRootPattern() const { return RootPattern; }
};
