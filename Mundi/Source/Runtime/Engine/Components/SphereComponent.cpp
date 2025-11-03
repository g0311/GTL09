#include "pch.h"
#include "SphereComponent.h"
#include "RenderManager.h"
#include "Vector.h"
#include "BoundingSphere.h"
#include "CollisionQueries.h"
#include "BoxComponent.h"
#include "CapsuleComponent.h"
#include "OBB.h"
#include "AABB.h"

IMPLEMENT_CLASS(USphereComponent)

BEGIN_PROPERTIES(USphereComponent)
    MARK_AS_COMPONENT("구 컴포넌트", "구 컴포넌트입니다.")
    ADD_PROPERTY_RANGE(float, SphereRadius, "Sphere", 0.1f, 50.0f, true, "Sphere Radius");
END_PROPERTIES()

USphereComponent::USphereComponent()
{
    SphereRadius = 3.0f;
    CollisionShape = ECollisionShapeType::Sphere;
}

USphereComponent::~USphereComponent()
{
    
}

void USphereComponent::DebugDraw() const
{
    URenderer* Renderer = URenderManager::GetInstance().GetRenderer();
    if (!Renderer)
        return;

    const FVector4 Color = FLinearColor(ShapeColor).ToFVector4();

    // World transform data
    const FVector Center = GetWorldLocation();
    const FVector Scale = GetWorldScale();
    const float RadiusWS = SphereRadius * std::max({ Scale.X, Scale.Y, Scale.Z });
    const FQuat Rot = GetWorldRotation();

    // Local basis rotated to world
    const FVector AxisX = Rot.RotateVector(FVector(1, 0, 0));
    const FVector AxisY = Rot.RotateVector(FVector(0, 1, 0));
    const FVector AxisZ = Rot.RotateVector(FVector(0, 0, 1));

    // Draw 3 great circles (XY, YZ, ZX in local space)
    const int Segments = 32;
    const float Delta = TWO_PI / static_cast<float>(Segments);

    auto AddCircle = [&](const FVector& U, const FVector& V)
    {
        TArray<FVector> Start, End;
        TArray<FVector4> Colors;
        Start.reserve(Segments);
        End.reserve(Segments);
        Colors.reserve(Segments);

        for (int i = 0; i < Segments; ++i)
        {
            const float a0 = static_cast<float>(i) * Delta;
            const float a1 = (static_cast<float>(i) + 1.0f) * Delta;
            const FVector P0 = Center + (U * std::cos(a0) + V * std::sin(a0)) * RadiusWS;
            const FVector P1 = Center + (U * std::cos(a1) + V * std::sin(a1)) * RadiusWS;
            Start.Add(P0);
            End.Add(P1);
            Colors.Add(Color);
        }
        Renderer->AddLines(Start, End, Colors);
    };

    AddCircle(AxisX, AxisY);
    AddCircle(AxisY, AxisZ);
    AddCircle(AxisZ, AxisX);
}

FBoundingSphere USphereComponent::GetWorldSphere() const
{
    const FVector Center = GetWorldLocation();
    const FVector Scale = GetWorldScale();
    const float RadiusWS = SphereRadius * std::max({ Scale.X, Scale.Y, Scale.Z });
    return FBoundingSphere(Center, RadiusWS);
}

bool USphereComponent::Overlaps(const UShapeComponent* Other) const
{
    // Respect collision/overlap flags on both shapes
    if (!IsCollisionEnabled() || !GetGenerateOverlapEvents())
        return false;
    if (!Other || !Other->IsCollisionEnabled() || !Other->GetGenerateOverlapEvents())
        return false;
    const FBoundingSphere This = GetWorldSphere();
    switch (Other->GetCollisionShapeType())
    {
    case ECollisionShapeType::Sphere:
        if (const USphereComponent* S = Cast<USphereComponent>(Other))
            return Collision::OverlapSphereSphere(This, S->GetWorldSphere());
        break;
    case ECollisionShapeType::OBB:
        if (const UBoxComponent* B = Cast<UBoxComponent>(Other))
            return Collision::OverlapOBBSphere(B->GetWorldOBB(), This);
        break;
    case ECollisionShapeType::Capsule:
        if (const UCapsuleComponent* C = Cast<UCapsuleComponent>(Other))
            return Collision::OverlapCapsuleSphere(C->GetWorldCapsule(), This);
        break;
    default:
        break;
    }
    return false;
}

FAABB USphereComponent::GetBroadphaseAABB() const
{
    // If disabled or not generating overlaps, return a degenerate AABB
    if (!IsCollisionEnabled() || !GetGenerateOverlapEvents())
    {
        const FVector P = GetWorldLocation();
        return FAABB(P, P);
    }
    const FBoundingSphere S = GetWorldSphere();
    const FVector r(S.Radius, S.Radius, S.Radius);
    return FAABB(S.Center - r, S.Center + r);
}
