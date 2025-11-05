#include "pch.h"
#include "SpringArmComponent.h"
#include "CameraComponent.h"
#include "World.h"
#include "Actor.h"
#include "CollisionQueries.h"
#include "BoundingSphere.h"
#include "SphereComponent.h"
#include "BoxComponent.h"
#include "CapsuleComponent.h"
#include "OBB.h"
#include "AABB.h"
#include "CollisionManager.h"

IMPLEMENT_CLASS(USpringArmComponent)

BEGIN_PROPERTIES(USpringArmComponent)
    MARK_AS_COMPONENT("스프링암 컴포넌트", "부모 기준으로 카메라를 지연/오프셋 배치합니다.")
    ADD_PROPERTY_RANGE(float, TargetArmLength, "SpringArm", 0.0f, 100000.0f, true, "암 길이입니다.(-X 방향)")
    ADD_PROPERTY(FVector, TargetOffset, "SpringArm", true, "암 끝의 로컬 오프셋입니다.")
    ADD_PROPERTY(bool, bInheritPitch, "SpringArm", true, "Pitch 상속 여부")
    ADD_PROPERTY(bool, bInheritYaw,   "SpringArm", true, "Yaw 상속 여부")
    ADD_PROPERTY(bool, bInheritRoll,  "SpringArm", true, "Roll 상속 여부")
    ADD_PROPERTY_RANGE(float, ProbeSize, "SpringArm", 0.0f, 200.0f, true, "충돌 탐색용 프로브 반경")
    ADD_PROPERTY(bool, bEnableCameraLag,         "Lag", true, "카메라 위치 랙 사용")
    ADD_PROPERTY(bool, bEnableCameraRotationLag, "Lag", true, "카메라 회전 랙 사용")
    ADD_PROPERTY_RANGE(float, CameraLagSpeed,           "Lag", 0.0f, 1000.0f, true, "위치 랙 속도")
    ADD_PROPERTY_RANGE(float, CameraRotationLagSpeed,   "Lag", 0.0f, 1000.0f, true, "회전 랙 속도")
    ADD_PROPERTY_RANGE(float, CameraLagMaxDistance,     "Lag", 0.0f, 100000.0f, true, "프레임당 최대 이동량(0=무제한)")
END_PROPERTIES()

USpringArmComponent::USpringArmComponent()
{
    bInheritPitch = true;
    bInheritYaw = true;
    bInheritRoll = true;

    TargetArmLength = 1.0f;
    TargetOffset = FVector(0, 0, 0);
    ProbeSize = 12.0f;

    bEnableCameraLag = true;
    bEnableCameraRotationLag = true;
    CameraLagSpeed = 10.f;
    CameraRotationLagSpeed = 10.f;
    CameraLagMaxDistance = 0.f;

    UnfixedCameraPosition = FVector(0, 0, 0);

    SetCanEverTick(true);
    SetTickEnabled(true);
}

FVector USpringArmComponent::GetDesiredRotation() const
{
    return GetWorldRotation().ToEulerZYXDeg();
}

FVector USpringArmComponent::GetTargetRotation() const
{
    return SmoothedRotation.ToEulerZYXDeg();
}

FVector USpringArmComponent::GetUnfixedCameraPosition() const
{
    return UnfixedCameraPosition;
}

bool USpringArmComponent::IsCollisionFixApplied() const
{
    return bIsCameraFixed;
}

void USpringArmComponent::OnRegister(UWorld* InWorld)
{
    // ensure tick
    SetCanEverTick(true);
    SetTickEnabled(true);
}

void USpringArmComponent::TickComponent(float DeltaTime)
{
    Super::TickComponent(DeltaTime);

    const bool bDoTrace = false; // no line trace in this engine for now
    const bool bDoLocationLag = bEnableCameraLag;
    const bool bDoRotationLag = bEnableCameraRotationLag;

    UpdateDesiredArmLocation(bDoTrace, bDoLocationLag, bDoRotationLag, DeltaTime);
}

void USpringArmComponent::UpdateDesiredArmLocation(bool /*bDoTrace*/, bool bDoLocationLag, bool bDoRotationLag, float DeltaTime)
{
    // Arm origin is this component's world location
    const FVector ArmOrigin = GetWorldLocation();

    // Base desired rotation from this component
    FQuat DesiredRot = GetWorldRotation();

    // Apply inherit toggles by zeroing selected euler components
    if (!(bInheritPitch && bInheritYaw && bInheritRoll))
    {
        FVector e = DesiredRot.ToEulerZYXDeg();
        // Note: ToEulerZYXDeg returns FVector(Roll(X), Pitch(Y), Yaw(Z)) per Vector.h comments
        if (!bInheritPitch) e.X = 0.0f; // Roll
        if (!bInheritYaw)   e.Y = 0.0f; // Pitch
        if (!bInheritRoll)  e.Z = 0.0f; // Yaw
        DesiredRot = FQuat::MakeFromEulerZYX(e).GetNormalized();
    }

    // Rotation lag (slerp-like using normalized lerp)
    if (bFirstTick)
    {
        SmoothedRotation = DesiredRot;
        PreviousDesiredRotation = DesiredRot;
    }
    else if (bDoRotationLag)
    {
        const float alphaRot = (CameraRotationLagSpeed <= 0.0f) ? 1.0f : (1.0f - std::exp(-CameraRotationLagSpeed * DeltaTime));
        FQuat from = SmoothedRotation.GetNormalized();
        FQuat to = DesiredRot.GetNormalized();
        float dot = FQuat::Dot(from, to);
        if (dot < 0.0f) { to = to * -1.0f; dot = -dot; }
        FQuat result = (from * (1.0f - alphaRot) + to * alphaRot).GetNormalized();
        SmoothedRotation = result;
    }
    else
    {
        SmoothedRotation = DesiredRot;
    }

    // Desired end (camera) position in world
    const FVector LocalEnd = FVector(-TargetArmLength, 0, 0) + TargetOffset;
    const FVector DesiredPos = ArmOrigin + SmoothedRotation.RotateVector(LocalEnd);

    // Store pre-collision (unfixed)
    UnfixedCameraPosition = DesiredPos;
    bIsCameraFixed = false;

    FVector ClampedPos = DesiredPos;
    const FVector Segment = DesiredPos - ArmOrigin;
    const float SegLen = Segment.Size();
    if (SegLen > 1e-3f && ProbeSize > 0.0f)
    {
        const FVector Dir = Segment / SegLen;
        const int Steps = 16; // coarse steps
        const float StepLen = SegLen / Steps;

        // Use a temporary sphere component as a probe and rely on its Overlaps implementation
        USphereComponent ProbeComp;
        {
            const float baseRadius = ProbeComp.GetWorldSphere().Radius; // default radius with scale 1
            const float scaleK = (baseRadius > 0.0f) ? (ProbeSize / baseRadius) : 1.0f;
            ProbeComp.SetWorldScale(FVector(scaleK, scaleK, scaleK));
        }

        auto OverlapsAt = [&](const FVector& Pos) -> bool
        {
            UWorld* W = GetWorld();
            if (!W) return false;

            ProbeComp.SetWorldLocation(Pos);

            // Use collision manager's ShapeBVH to gather candidates quickly
            TArray<UShapeComponent*> Candidates;
            if (UCollisionManager* CM = W->GetCollisionManager())
            {
                CM->QueryAABB(ProbeComp.GetBroadphaseAABB(), Candidates);
            }
            else
            {
                // Fallback to iterating all actors/components
                for (AActor* Actor : W->GetActors())
                {
                    if (!Actor) continue;
                    for (USceneComponent* Comp : Actor->GetSceneComponents())
                    {
                        if (UShapeComponent* Shape = Cast<UShapeComponent>(Comp))
                            Candidates.Add(Shape);
                    }
                }
            }

            for (UShapeComponent* Shape : Candidates)
            {
                if (!Shape) continue;
                if (Shape->GetOwner() == GetOwner()) continue; // ignore self owner
                // Rely on Overlaps to respect collision and overlap flags
                if (ProbeComp.Overlaps(Shape))
                    return true;
            }
            return false;
        };

        float hitT = 1.0f;
        bool bHit = false;
        for (int i = 1; i <= Steps; ++i)
        {
            const float t = (float)i / (float)Steps;
            const FVector P = ArmOrigin + Dir * (t * SegLen);
            if (OverlapsAt(P))
            {
                // back off to previous sample and refine via binary search
                float t0 = (float)(i - 1) / (float)Steps;
                float t1 = t;
                for (int it = 0; it < 3; ++it) // 3 iterations are enough
                {
                    float mid = 0.5f * (t0 + t1);
                    FVector Pm = ArmOrigin + Dir * (mid * SegLen);
                    if (OverlapsAt(Pm))
                        t1 = mid;
                    else
                        t0 = mid;
                }
                hitT = t0;
                bHit = true;
                break;
            }
        }
        if (bHit)
        {
            ClampedPos = ArmOrigin + Dir * (hitT * SegLen);
            bIsCameraFixed = true;
        }
    }

    // Location lag
    FVector FinalPos = ClampedPos;
    if (bFirstTick)
    {
        PreviousDesiredLocation = FinalPos;
        PreviousArmOrigin = ArmOrigin;
        bFirstTick = false;
    }
    else if (bDoLocationLag)
    {
        FinalPos = BlendLocations(PreviousDesiredLocation, ClampedPos, DeltaTime, CameraLagSpeed, CameraLagMaxDistance);
    }

    // Cache for next tick
    PreviousDesiredLocation = FinalPos;
    PreviousArmOrigin = ArmOrigin;
    PreviousDesiredRotation = DesiredRot;

    // Apply to attached children (acts as socket at arm end)
    const TArray<USceneComponent*>& Children = GetAttachChildren();
    for (USceneComponent* Child : Children)
    {
        if (!Child) continue;
        Child->SetWorldLocationAndRotation(FinalPos, SmoothedRotation);
    }
}

FVector USpringArmComponent::BlendLocations(const FVector& From, const FVector& To, float DeltaTime, float LagSpeed, float MaxDistance)
{
    if (LagSpeed <= 0.0f)
    {
        return To;
    }
    const float alpha = 1.0f - std::exp(-LagSpeed * DeltaTime);
    FVector blended = From + (To - From) * alpha;
    if (MaxDistance > 0.0f)
    {
        FVector delta = blended - From;
        const float len = delta.Size();
        if (len > MaxDistance)
        {
            blended = From + delta.GetNormalized() * MaxDistance;
        }
    }
    return blended;
}

void USpringArmComponent::DuplicateSubObjects()
{
    Super::DuplicateSubObjects();
}

void USpringArmComponent::OnSerialized()
{
    Super::OnSerialized();
}
