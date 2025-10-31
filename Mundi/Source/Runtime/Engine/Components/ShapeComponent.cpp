#include "pch.h"
#include "ShapeComponent.h"
#include "CollisionManager.h"
#include "AABB.h"
#include "World.h"
#include "SelectionManager.h"

IMPLEMENT_CLASS(UShapeComponent)

BEGIN_PROPERTIES(UShapeComponent)
    ADD_PROPERTY_RANGE(bool, bDrawOnlyIfSelected, "Shape", 0, 1, true, "Draw Only If Selected");
    ADD_PROPERTY_RANGE(FLinearColor, ShapeColor, "Shape", 0.0f, 1.0f, true, "Debug draw color")
END_PROPERTIES()

UShapeComponent::UShapeComponent()
{
    SetCollisionEnabled(true);
    SetGenerateOverlapEvents(true);
    ShapeColor = FLinearColor::Green;
}

void UShapeComponent::RenderDebugVolume(URenderer* Renderer) const
{
    // If drawing only when selected, check selection state
    if (bDrawOnlyIfSelected)
    {
        if (UWorld* W = const_cast<UShapeComponent*>(this)->GetWorld())
        {
            if (USelectionManager* Sel = W->GetSelectionManager())
            {
                if (!Sel->IsActorSelected(GetOwner()))
                {
                    return; // not selected and draw-only mode is on
                }
            }
        }
    }
    DebugDraw();
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
