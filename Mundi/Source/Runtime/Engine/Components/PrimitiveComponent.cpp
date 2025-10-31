#include "pch.h"
#include "PrimitiveComponent.h"
#include "SceneComponent.h"

IMPLEMENT_CLASS(UPrimitiveComponent)

void UPrimitiveComponent::SetMaterialByName(uint32 InElementIndex, const FString& InMaterialName)
{
    SetMaterial(InElementIndex, UResourceManager::GetInstance().Load<UMaterial>(InMaterialName));
}

void UPrimitiveComponent::DuplicateSubObjects()
{
    Super::DuplicateSubObjects();
}

void UPrimitiveComponent::OnSerialized()
{
    Super::OnSerialized();
}

bool UPrimitiveComponent::IsOverlappingActor(const AActor* Other) const
{
    if (Other == nullptr)
    {
        return false;
    }

    // If this component is not set up to generate overlaps or collision is disabled, no overlaps are valid
    if (!bIsCollisionEnabled || !bGenerateOverlapEvents)
    {
        return false;
    }

    for (const FOverlapInfo& Info : OverlapInfos)
    {
        if (Info.OtherActor == Other)
        {
            return true;
        }
    }
    return false;
}
