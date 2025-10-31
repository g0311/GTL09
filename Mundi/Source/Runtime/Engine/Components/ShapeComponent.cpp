#include "pch.h"
#include "ShapeComponent.h"

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

    // Sync overlaps when a shape moves
    const TArray<FOverlapInfo> Old = OverlapInfos;
    RefreshOverlapInfos();

    TSet<UPrimitiveComponent*> Peers;
    for (const FOverlapInfo& E : Old) if (E.OtherComp) Peers.insert(E.OtherComp);
    for (const FOverlapInfo& E : OverlapInfos) if (E.OtherComp) Peers.insert(E.OtherComp);

    for (UPrimitiveComponent* Other : Peers)
    {
        if (Other && Other != this)
        {
            Other->RefreshOverlapInfos();
        }
    }
}
