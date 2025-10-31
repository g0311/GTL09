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

    // Helper to push a new overlap
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
        // Avoid duplicates
        bool bExists = false;
        for (const FOverlapInfo& E : OverlapInfos)
        {
            if (E == Info) { bExists = true; break; }
        }
        if (!bExists)
            OverlapInfos.Add(Info);
    };

    // Construct this component's shape info
    const USphereComponent* ThisSphere = Cast<USphereComponent>(this);
    const UBoxComponent* ThisBox = ThisSphere ? nullptr : Cast<UBoxComponent>(this);
    const UCapsuleComponent* ThisCapsule = (ThisSphere || ThisBox) ? nullptr : Cast<UCapsuleComponent>(this);
    const UStaticMeshComponent* ThisSMC = (ThisSphere || ThisBox || ThisCapsule) ? nullptr : Cast<UStaticMeshComponent>(this);

    // Precompute this shape
    FBoundingSphere ThisSphereShape;
    FOBB ThisOBBShape;
    FCapsule ThisCapsuleShape;
    FAABB ThisAABBShape;

    if (ThisSphere)
    {
        ThisSphereShape = ThisSphere->GetWorldSphere();
    }
    else if (ThisBox)
    {
        ThisOBBShape = ThisBox->GetWorldOBB();
    }
    else if (ThisCapsule)
    {
        ThisCapsuleShape = ThisCapsule->GetWorldCapsule();
    }
    else if (ThisSMC)
    {
        ThisAABBShape = ThisSMC->GetWorldAABB();
    }

    // Iterate world actors/components
    for (AActor* Actor : World->GetActors())
    {
        if (!Actor || Actor == MyOwner)
            continue;

        for (USceneComponent* Comp : Actor->GetSceneComponents())
        {
            UPrimitiveComponent* OtherPrim = Cast<UPrimitiveComponent>(Comp);
            if (!OtherPrim || OtherPrim == this)
                continue;
            if (!OtherPrim->IsCollisionEnabled() || !OtherPrim->GetGenerateOverlapEvents())
                continue;

            const USphereComponent* OtherSphere = Cast<USphereComponent>(OtherPrim);
            const UBoxComponent* OtherBox = OtherSphere ? nullptr : Cast<UBoxComponent>(OtherPrim);
            const UCapsuleComponent* OtherCapsule = (OtherSphere || OtherBox) ? nullptr : Cast<UCapsuleComponent>(OtherPrim);
            const UStaticMeshComponent* OtherSMC = (OtherSphere || OtherBox || OtherCapsule) ? nullptr : Cast<UStaticMeshComponent>(OtherPrim);

            bool bOverlap = false;

            if (ThisSphere)
            {
                if (OtherSphere)
                {
                    bOverlap = Collision::OverlapSphereSphere(ThisSphereShape, OtherSphere->GetWorldSphere());
                }
                else if (OtherBox)
                {
                    bOverlap = Collision::OverlapOBBSphere(OtherBox->GetWorldOBB(), ThisSphereShape);
                }
                else if (OtherCapsule)
                {
                    bOverlap = Collision::OverlapCapsuleSphere(OtherCapsule->GetWorldCapsule(), ThisSphereShape);
                }
                else if (OtherSMC)
                {
                    bOverlap = Collision::OverlapAABBSphere(OtherSMC->GetWorldAABB(), ThisSphereShape);
                }
            }
            else if (ThisBox)
            {
                if (OtherSphere)
                {
                    bOverlap = Collision::OverlapOBBSphere(ThisOBBShape, OtherSphere->GetWorldSphere());
                }
                else if (OtherBox)
                {
                    bOverlap = Collision::OverlapOBBOBB(ThisOBBShape, OtherBox->GetWorldOBB());
                }
                else if (OtherCapsule)
                {
                    bOverlap = Collision::OverlapOBBCapsule(ThisOBBShape, OtherCapsule->GetWorldCapsule());
                }
                else if (OtherSMC)
                {
                    bOverlap = Collision::OverlapAABBOBB(OtherSMC->GetWorldAABB(), ThisOBBShape);
                }
            }
            else if (ThisCapsule)
            {
                if (OtherSphere)
                {
                    bOverlap = Collision::OverlapCapsuleSphere(ThisCapsuleShape, OtherSphere->GetWorldSphere());
                }
                else if (OtherBox)
                {
                    bOverlap = Collision::OverlapOBBCapsule(OtherBox->GetWorldOBB(), ThisCapsuleShape);
                }
                else if (OtherCapsule)
                {
                    bOverlap = Collision::OverlapCapsuleCapsule(ThisCapsuleShape, OtherCapsule->GetWorldCapsule());
                }
                else if (OtherSMC)
                {
                    // AABB-Capsule not implemented in CollisionQueries; skip
                    bOverlap = false;
                }
            }
            else if (ThisSMC)
            {
                if (OtherSphere)
                {
                    bOverlap = Collision::OverlapAABBSphere(ThisAABBShape, OtherSphere->GetWorldSphere());
                }
                else if (OtherBox)
                {
                    bOverlap = Collision::OverlapAABBOBB(ThisAABBShape, OtherBox->GetWorldOBB());
                }
                else if (OtherCapsule)
                {
                    // AABB-Capsule not implemented
                    bOverlap = false;
                }
            }

            if (bOverlap)
            {
                AddOverlap(Actor, OtherPrim);
            }
        }
    }
}
