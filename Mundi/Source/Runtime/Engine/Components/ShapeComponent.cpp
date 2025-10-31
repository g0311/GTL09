#include "pch.h"
#include "ShapeComponent.h"
#include "CollisionManager.h"
#include "AABB.h"

IMPLEMENT_CLASS(UShapeComponent)

BEGIN_PROPERTIES(UShapeComponent)
    MARK_AS_COMPONENT("모양 컴포넌트", "모양 컴포넌트입니다.")
END_PROPERTIES()

UShapeComponent::UShapeComponent()
{
    SetCollisionEnabled(true);
    SetGenerateOverlapEvents(true);
}

void UShapeComponent::OnTransformUpdated()
{
    Super::OnTransformUpdated();
    // Defer overlap recomputation to the central collision manager
    if (UWorld* W = GetWorld())
    {
        if (auto* CM = W->GetCollisionManager())
        {
            CM->MarkDirty(this);
        }
    }
}

void UShapeComponent::OnRegister(UWorld* InWorld)
{
    Super::OnRegister(InWorld);
    // Register with collision manager
    if (InWorld)
    {
        // UWorld exposes getter; include in ShapeComponent.cpp we can reach via Owner->GetWorld()
        if (UWorld* W = GetWorld())
        {
            if (auto* CM = W->GetCollisionManager())
            {
                CM->Register(this);
                CM->MarkDirty(this);
            }
        }
    }
}

void UShapeComponent::OnUnregister()
{
    Super::OnUnregister();
    // Unregister from collision manager
    if (UWorld* W = GetWorld())
    {
        if (auto* CM = W->GetCollisionManager())
        {
            CM->Unregister(this);
        }
    }
}

FAABB UShapeComponent::GetBroadphaseAABB() const
{
    // Default: empty AABB at world location
    const FVector P = GetWorldLocation();
    return FAABB(P, P);
}
