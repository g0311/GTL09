#include "pch.h"
#include "PrimitiveComponent.h"
#include "SceneComponent.h"
#include "World.h"
#include "Actor.h"
#include "CollisionQueries.h"
#include "SphereComponent.h"
#include "BoxComponent.h"
#include "CapsuleComponent.h"
#include "StaticMeshComponent.h"
#include "OBB.h"
#include "AABB.h"
#include "BoundingSphere.h"

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

TArray<AActor*> UPrimitiveComponent::GetOverlappingActors(TArray<AActor*>& OutActors, uint32 mask) const
{
    OutActors.Empty();

    if (!bIsCollisionEnabled || !bGenerateOverlapEvents)
    {
        return OutActors;
    }

    for (const FOverlapInfo& Info : OverlapInfos)
    {
        AActor* OtherActor = Info.OtherActor;
        if (!OtherActor)
            continue;

        // If a mask is provided, check the other component's collision layer
        if (mask != ~0u)
        {
            if (Info.OtherComp)
            {
                if ( (Info.OtherComp->GetCollisionLayer() & mask) == 0 )
                {
                    continue; // filtered out
                }
            }
            // If there is no component info, conservatively include
        }

        OutActors.AddUnique(OtherActor);
    }

    return OutActors;
}

void UPrimitiveComponent::RefreshOverlapInfos(uint32 mask)
{
    OverlapInfos.Empty();

    if (!bIsCollisionEnabled || !bGenerateOverlapEvents)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    AActor* MyOwner = GetOwner();
    UShapeComponent* ThisShape = Cast<UShapeComponent>(this);
    if (!ThisShape)
    {
        return;
    }

    auto AddOverlap = [&](AActor* OtherActor, UPrimitiveComponent* OtherComp)
    {
        if (!OtherActor || OtherActor == MyOwner)
            return;
        if (mask != ~0u && OtherComp)
        {
            if ((OtherComp->GetCollisionLayer() & mask) == 0)
                return;
        }
        FOverlapInfo Info;
        Info.OtherActor = OtherActor;
        Info.OtherComp = OtherComp;
        if (!OverlapInfos.Contains(Info))
            OverlapInfos.Add(Info);
    };
    
    for (AActor* Actor : World->GetActors())
    {
        if (!Actor || Actor == MyOwner)
            continue;

        for (USceneComponent* Comp : Actor->GetSceneComponents())
        {
            UShapeComponent* OtherShape = Cast<UShapeComponent>(Comp);
            if (!OtherShape || OtherShape == ThisShape)
                continue;
            if (!OtherShape->IsCollisionEnabled() || !OtherShape->GetGenerateOverlapEvents())
                continue;
            
            if (ThisShape->Overlaps(OtherShape))
            {
                AddOverlap(Actor, OtherShape);
            }
        }
    }
}
