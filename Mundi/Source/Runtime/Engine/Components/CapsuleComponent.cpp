#include "pch.h"
#include "CapsuleComponent.h"

#include "BoundingSphere.h"
#include "RenderManager.h"
#include "Vector.h"
#include "CollisionQueries.h"
#include "SphereComponent.h"
#include "BoxComponent.h"
#include "OBB.h"
#include "AABB.h"

IMPLEMENT_CLASS(UCapsuleComponent)

BEGIN_PROPERTIES(UCapsuleComponent)
    MARK_AS_COMPONENT("캡슐 컴포넌트", "캡슐 컴포넌트입니다.")
    ADD_PROPERTY_RANGE(float, CapsuleRadius, "Capsule", 0.1f, 50.0f, true, "Capsule Radius");
    ADD_PROPERTY_RANGE(float, CapsuleHalfHeight, "Capsule", 0.1f, 50.0f, true, "Capsule Half Height");
END_PROPERTIES()

UCapsuleComponent::UCapsuleComponent()
{
    CapsuleRadius = 3.0f;
    CapsuleHalfHeight = 5.0f;
    CollisionShape = ECollisionShapeType::Capsule;
}

UCapsuleComponent::~UCapsuleComponent()
{
    
}

FCapsule UCapsuleComponent::GetWorldCapsule() const
{
    const FTransform& WorldTransform = GetWorldTransform();

    // The capsule's local axis (Z axis convention for capsule orientation)
    const FVector CapsuleAxisWorld = WorldTransform.GetUnitAxis(EAxis::Z);

    // World-space center of the capsule component
    const FVector CapsuleCenterWorld = WorldTransform.TransformPosition(FVector(0, 0, 0));

    // Absolute world scale values
    const FVector WorldScale = GetWorldScale();

    // Scaling factors (simple conservative approach — same scale for radius and half-height)
    const float RadiusScale = std::max({ WorldScale.X, WorldScale.Y, WorldScale.Z });
    const float HalfHeightScale = RadiusScale; // conservative; can be made more accurate if projecting scale along axis

    // Final scaled radius & half-height
    const float CapsuleRadiusWorld = CapsuleRadius * RadiusScale;
    const float CapsuleHalfHeightWorld = CapsuleHalfHeight * HalfHeightScale;

    // Compute world-space capsule end points
    const FVector CapsuleEndPointTop    = CapsuleCenterWorld + CapsuleAxisWorld * CapsuleHalfHeightWorld;
    const FVector CapsuleEndPointBottom = CapsuleCenterWorld - CapsuleAxisWorld * CapsuleHalfHeightWorld;

    return FCapsule(CapsuleEndPointTop, CapsuleEndPointBottom, CapsuleRadiusWorld);
}

void UCapsuleComponent::DebugDraw() const
{
    URenderer* Renderer = URenderManager::GetInstance().GetRenderer();
    if (!Renderer)
        return;

    const FVector4 Color = FLinearColor(ShapeColor).ToFVector4();

    const FCapsule Cap = GetWorldCapsule();
    const FVector C1 = Cap.Center1;
    const FVector C2 = Cap.Center2;
    const float R = Cap.Radius;

    // Axis from C1 to C2
    FVector Axis = (C2 - C1);
    float AxisLen = Axis.Size();
    if (AxisLen <= KINDA_SMALL_NUMBER)
        return;
    Axis = Axis * (1.0f / AxisLen);

    // Build orthonormal basis (U, V) perpendicular to Axis
    FVector Temp = (std::fabs(Axis.Z) < 0.99f) ? FVector(0, 0, 1) : FVector(0, 1, 0);
    FVector U = FVector::Cross(Axis, Temp).GetNormalized();
    FVector V = FVector::Cross(Axis, U).GetNormalized();

    // Rings at both ends + hemisphere arcs
    const int Segments = 24;
    const float DeltaFull = TWO_PI / static_cast<float>(Segments);
    const float DeltaHalf = PI / static_cast<float>(Segments);

    TArray<FVector> Starts;
    TArray<FVector> Ends;
    TArray<FVector4> Colors;
    Starts.reserve(Segments * 3 + 8);
    Ends.reserve(Segments * 3 + 8);
    Colors.reserve(Segments * 3 + 8);

    auto AddRing = [&](const FVector& Center)
    {
        for (int i = 0; i < Segments; ++i)
        {
            const float a0 = static_cast<float>(i) * DeltaFull;
            const float a1 = (static_cast<float>(i) + 1.0f) * DeltaFull;
            const FVector P0 = Center + (U * std::cos(a0) + V * std::sin(a0)) * R;
            const FVector P1 = Center + (U * std::cos(a1) + V * std::sin(a1)) * R;
            Starts.Add(P0);
            Ends.Add(P1);
            Colors.Add(Color);
        }
    };

    AddRing(C1);
    AddRing(C2);

    // Longitudinal lines along U and V directions
    const FVector Udir = U * R;
    const FVector Vdir = V * R;

    Starts.Add(C1 + Udir); Ends.Add(C2 + Udir); Colors.Add(Color);
    Starts.Add(C1 - Udir); Ends.Add(C2 - Udir); Colors.Add(Color);
    Starts.Add(C1 + Vdir); Ends.Add(C2 + Vdir); Colors.Add(Color);
    Starts.Add(C1 - Vdir); Ends.Add(C2 - Vdir); Colors.Add(Color);

    // Hemisphere arcs on both caps in planes (U,Axis) and (V,Axis)
    auto AddHemisphere = [&](const FVector& Center, const FVector& AxisDir)
    {
        for (int i = 0; i < Segments; ++i)
        {
            const float t0 = static_cast<float>(i) * DeltaHalf;
            const float t1 = (static_cast<float>(i) + 1.0f) * DeltaHalf;
            // Plane (U, Axis)
            {
                const FVector P0 = Center + (U * std::cos(t0) + AxisDir * std::sin(t0)) * R;
                const FVector P1 = Center + (U * std::cos(t1) + AxisDir * std::sin(t1)) * R;
                Starts.Add(P0); Ends.Add(P1); Colors.Add(Color);
            }
            // Plane (V, Axis)
            {
                const FVector P0 = Center + (V * std::cos(t0) + AxisDir * std::sin(t0)) * R;
                const FVector P1 = Center + (V * std::cos(t1) + AxisDir * std::sin(t1)) * R;
                Starts.Add(P0); Ends.Add(P1); Colors.Add(Color);
            }
        }
    };

    AddHemisphere(C2, Axis); // top cap
    AddHemisphere(C1, -Axis); // bottom cap

    Renderer->AddLines(Starts, Ends, Colors);
}

bool UCapsuleComponent::Overlaps(const UShapeComponent* Other) const
{
    if (!Other) return false;
    const FCapsule This = GetWorldCapsule();
    switch (Other->GetCollisionShapeType())
    {
    case ECollisionShapeType::Sphere:
        if (const USphereComponent* S = Cast<USphereComponent>(Other))
            return Collision::OverlapCapsuleSphere(This, S->GetWorldSphere());
        break;
    case ECollisionShapeType::OBB:
        if (const UBoxComponent* B = Cast<UBoxComponent>(Other))
            return Collision::OverlapOBBCapsule(B->GetWorldOBB(), This);
        break;
    case ECollisionShapeType::Capsule:
        if (const UCapsuleComponent* C = Cast<UCapsuleComponent>(Other))
            return Collision::OverlapCapsuleCapsule(This, C->GetWorldCapsule());
        break;
    default:
        break;
    }
    return false;
}

FAABB UCapsuleComponent::GetBroadphaseAABB() const
{
    const FCapsule Cap = GetWorldCapsule();
    FVector MinV = Cap.Center1.ComponentMin(Cap.Center2);
    FVector MaxV = Cap.Center1.ComponentMax(Cap.Center2);
    const FVector r(Cap.Radius, Cap.Radius, Cap.Radius);
    MinV = MinV - r;
    MaxV = MaxV + r;
    return FAABB(MinV, MaxV);
}
