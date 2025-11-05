#include "pch.h"
#include "BoxComponent.h"
#include "RenderManager.h"
#include "AABB.h"
#include "BoundingSphere.h"
#include "OBB.h"
#include "CollisionQueries.h"
#include "SphereComponent.h"
#include "CapsuleComponent.h"

IMPLEMENT_CLASS(UBoxComponent)

BEGIN_PROPERTIES(UBoxComponent)
    MARK_AS_COMPONENT("박스 컴포넌트", "박스 컴포넌트입니다.")
    ADD_PROPERTY_RANGE(FVector, BoxExtent, "Box", 0.1f, 50.0f, true, "Box Extent");
END_PROPERTIES()

UBoxComponent::UBoxComponent()
{
    BoxExtent = FVector(3.0f, 3.0f, 3.0f);
    CollisionShape = ECollisionShapeType::OBB;
}

UBoxComponent::~UBoxComponent()
{
    
}

void UBoxComponent::DebugDraw() const
{
    URenderer* Renderer = URenderManager::GetInstance().GetRenderer();
    if (!Renderer)
        return;

    const FVector4 Color = FLinearColor(ShapeColor).ToFVector4();

    // Build OBB from local AABB and world matrix
    const FVector Ext = BoxExtent; // half extents in local space
    const FAABB LocalAABB(FVector(-Ext.X, -Ext.Y, -Ext.Z), FVector(Ext.X, Ext.Y, Ext.Z));
    const FOBB OBB(LocalAABB, GetWorldMatrix());

    // Edge list from corners (same as in UDecalComponent)
    static const int Edges[12][2] = {
        {6, 4}, {7, 5}, {6, 7}, {4, 5},
        {4, 0}, {5, 1}, {6, 2}, {7, 3},
        {0, 2}, {1, 3}, {0, 1}, {2, 3}
    };

    const TArray<FVector> Corners = OBB.GetCorners();

    TArray<FVector> StartPoints;
    TArray<FVector> EndPoints;
    TArray<FVector4> Colors;
    StartPoints.reserve(12);
    EndPoints.reserve(12);
    Colors.reserve(12);

    for (int i = 0; i < 12; ++i)
    {
        const FVector& S = Corners[Edges[i][0]];
        const FVector& E = Corners[Edges[i][1]];
        StartPoints.Add(S);
        EndPoints.Add(E);
        Colors.Add(Color);
    }

    Renderer->AddLines(StartPoints, EndPoints, Colors);
}

/**
 * @brief Constructs the oriented bounding box representing this component in world space.
 *
 * Uses the component's half extents (BoxExtent) as a local axis-aligned box and applies the component's world transform to produce a world-space OBB.
 *
 * @return FOBB Oriented bounding box in world space.
 */
FOBB UBoxComponent::GetWorldOBB() const
{
    const FVector Ext = BoxExtent; // half extents in local space
    const FAABB LocalAABB(FVector(-Ext.X, -Ext.Y, -Ext.Z), FVector(Ext.X, Ext.Y, Ext.Z));
    return FOBB(LocalAABB, GetWorldMatrix());
}

/**
 * @brief Tests whether this box component overlaps another collision shape.
 *
 * Performs an overlap test between this component's world-oriented bounding box and the provided shape.
 *
 * @param Other The other shape component to test against.
 * @param OutContactInfo Optional output; if non-null and an overlap is detected, receives contact information describing the overlap.
 * @return `true` if this box overlaps `Other`, `false` otherwise.
 */
bool UBoxComponent::Overlaps(const UShapeComponent* Other, FContactInfo* OutContactInfo) const
{
    // Respect collision/overlap flags on both shapes
    if (!IsCollisionEnabled() || !GetGenerateOverlapEvents())
        return false;
    if (!Other || !Other->IsCollisionEnabled() || !Other->GetGenerateOverlapEvents())
        return false;
    const FOBB This = GetWorldOBB();
    switch (Other->GetCollisionShapeType())
    {
    case ECollisionShapeType::Sphere:
        if (const USphereComponent* S = Cast<USphereComponent>(Other))
            return Collision::OverlapOBBSphere(This, S->GetWorldSphere(), OutContactInfo);
        break;
    case ECollisionShapeType::OBB:
        if (const UBoxComponent* B = Cast<UBoxComponent>(Other))
            return Collision::OverlapOBBOBB(This, B->GetWorldOBB(), OutContactInfo);
        break;
    case ECollisionShapeType::Capsule:
        if (const UCapsuleComponent* C = Cast<UCapsuleComponent>(Other))
            return Collision::OverlapOBBCapsule(This, C->GetWorldCapsule(), OutContactInfo);
        break;
    default:
        break;
    }
    return false;
}

FAABB UBoxComponent::GetBroadphaseAABB() const
{
    // If disabled or not generating overlaps, return a degenerate AABB
    if (!IsCollisionEnabled() || !GetGenerateOverlapEvents())
    {
        const FVector P = GetWorldLocation();
        return FAABB(P, P);
    }
    const FOBB This = GetWorldOBB();
    const TArray<FVector> Corners = This.GetCorners();
    if (Corners.IsEmpty())
    {
        const FVector C = This.Center;
        return FAABB(C, C);
    }

    FVector MinV = Corners[0];
    FVector MaxV = Corners[0];
    for (int i = 1; i < Corners.Num(); ++i)
    {
        const FVector& P = Corners[i];
        MinV = MinV.ComponentMin(P);
        MaxV = MaxV.ComponentMax(P);
    }
    return FAABB(MinV, MaxV);
}