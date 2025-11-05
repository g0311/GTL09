#include "pch.h"
#include "CameraShakeBase.h"
#include "CameraShakePattern.h"
#include "PlayerCameraManager.h"

IMPLEMENT_CLASS(UCameraShakeBase)

BEGIN_PROPERTIES(UCameraShakeBase)
    ADD_PROPERTY_RANGE(float, Duration,    "CameraShake", 0.0f, 1000.0f, true, "Play duration (<=0 = infinite)")
    ADD_PROPERTY_RANGE(float, BlendInTime, "CameraShake", 0.0f,  10.0f,  true, "Blend-in time")
    ADD_PROPERTY_RANGE(float, BlendOutTime,"CameraShake", 0.0f,  10.0f,  true, "Blend-out time")
    ADD_PROPERTY_RANGE(float, PlayScale,   "CameraShake", 0.0f, 100.0f,  true, "Play scale")
END_PROPERTIES()

UCameraShakeBase::UCameraShakeBase()
{
    Duration = 0.0f;
    BlendInTime = 0.1f;
    BlendOutTime = 0.2f;
    PlayScale = 1.0f;
}

UCameraShakeBase::~UCameraShakeBase()
{
    if (RootPattern && bOwnsPattern)
    {
        DeleteObject(RootPattern);
        RootPattern = nullptr;
        bOwnsPattern = false;
    }
}

void UCameraShakeBase::StartShake(APlayerCameraManager* InCameraManager, float InScale, float InDuration)
{
    CameraManager = InCameraManager;
    PlayScale = InScale;
    if (InDuration > 0.0f)
    {
        Duration = InDuration;
    }
    ElapsedTime = 0.0f;
    bIsFinished = false;
    bIsActive = true;
    OnStart();
    if (RootPattern)
    {
        RootPattern->StartShake(InCameraManager, InScale, InDuration > 0.0f ? InDuration : Duration);
    }
}

void UCameraShakeBase::StopShake(bool bImmediately)
{
    if (!bIsActive) return;
    if (bImmediately)
    {
        bIsActive = false;
        bIsFinished = true;
        OnStop(true);
        if (RootPattern)
        {
            RootPattern->StopShake(true);
        }
        return;
    }
    // Trigger blend-out from current time by clamping duration to now
    Duration = ElapsedTime;
    OnStop(false);
    if (RootPattern)
    {
        RootPattern->StopShake(false);
    }
}

FCameraShakeUpdateResult UCameraShakeBase::UpdateShake(float DeltaTime)
{
    FCameraShakeUpdateResult Result; // defaults to zero offsets
    if (!bIsActive || bIsFinished)
    {
        return Result;
    }

    ElapsedTime += DeltaTime;

    // Prepare params for child update
    FCameraShakeUpdateParams Params;
    Params.DeltaTime = DeltaTime;
    Params.ElapsedTime = ElapsedTime;
    Params.Duration = Duration;
    Params.PlayScale = PlayScale;

    // Ask root pattern (if any) or derived class to provide raw offsets
    if (RootPattern)
    {
        Result = RootPattern->UpdateShake(DeltaTime);
    }
    else
    {
        OnUpdate(Params, Result);
    }

    // Compute blend alpha (handles in/out)
    const float Alpha = ComputeBlendAlpha(ElapsedTime, Duration);

    // Apply blend alpha and scale
    Result.LocationOffset = Result.LocationOffset * (Alpha * PlayScale);
    // Scale FOV (degrees)
    Result.FOVOffset *= (Alpha * PlayScale);
    // Slerp rotation offset towards identity by (1-Alpha*PlayScale) is not ideal; instead scale by Alpha*Scale for small angles.
    // Approximate by nlerp between identity and delta rotation.
    const float rotScale = std::clamp(Alpha * PlayScale, 0.0f, 1.0f);
    if (rotScale < 1.0f)
    {
        Result.RotationOffset = (FQuat(0,0,0,1) * (1.0f - rotScale) + Result.RotationOffset.GetNormalized() * rotScale).GetNormalized();
    }
    else
    {
        Result.RotationOffset = Result.RotationOffset.GetNormalized();
    }

    // Finish condition: if we had a finite duration and finished blend-out
    if (Duration > 0.0f)
    {
        const float outAlpha = (BlendOutTime > 0.0f) ? std::max(0.0f, 1.0f - (ElapsedTime - Duration) / BlendOutTime) : (ElapsedTime <= Duration ? 1.0f : 0.0f);
        if (ElapsedTime >= (Duration + BlendOutTime) || outAlpha <= 0.0f)
        {
            bIsActive = false;
            bIsFinished = true;
        }
    }

    return Result;
}

float UCameraShakeBase::ComputeBlendAlpha(float Elapsed, float InDuration) const
{
    // Blend-in
    float alphaIn = 1.0f;
    if (BlendInTime > 0.0f)
    {
        alphaIn = std::clamp(Elapsed / BlendInTime, 0.0f, 1.0f);
    }

    // Blend-out (finite only)
    float alphaOut = 1.0f;
    if (InDuration > 0.0f && BlendOutTime > 0.0f)
    {
        const float tOut = (Elapsed > InDuration) ? (Elapsed - InDuration) / BlendOutTime : 0.0f;
        alphaOut = std::clamp(1.0f - tOut, 0.0f, 1.0f);
    }
    else if (InDuration > 0.0f && BlendOutTime <= 0.0f)
    {
        alphaOut = (Elapsed <= InDuration) ? 1.0f : 0.0f;
    }

    return std::min(alphaIn, alphaOut);
}
