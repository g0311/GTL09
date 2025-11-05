#include "pch.h"
#include "CollisionManager.h"
#include "ShapeComponent.h"
#include "PrimitiveComponent.h"
#include "Actor.h"
#include "ShapeBVH.h"
#include "AABB.h"

IMPLEMENT_CLASS(UCollisionManager)

UCollisionManager::UCollisionManager() { ShapeBVH = new FShapeBVH(); }
UCollisionManager::~UCollisionManager() { if (ShapeBVH) { delete ShapeBVH; ShapeBVH = nullptr; } }

void UCollisionManager::Register(UShapeComponent* Shape)
{
    if (!Shape) return;
    ActiveShapes.insert(Shape);
    if (ShapeBVH)
    {
        ShapeBVH->Insert(Shape, Shape->GetBroadphaseAABB());
    }
}

void UCollisionManager::Unregister(UShapeComponent* Shape)
{
    if (!Shape) return;
    ActiveShapes.erase(Shape);
    // Also clear any pending dirty state
    DirtySet.erase(Shape);
    if (ShapeBVH)
    {
        ShapeBVH->Remove(Shape);
    }
}

void UCollisionManager::MarkDirty(UShapeComponent* Shape)
{
    if (!Shape) return;
    if (DirtySet.insert(Shape).second)
    {
        DirtyQueue.push(Shape);
    }
}

void UCollisionManager::Update(float /*DeltaTime*/, uint32 Budget)
{
    uint32 processed = 0;
    while (processed < Budget)
    {
        UShapeComponent* Shape = nullptr;
        if (!DirtyQueue.Dequeue(Shape))
            break;

        if (DirtySet.erase(Shape) == 0)
            continue; // already processed/removed

        if (Shape)
        {
            ProcessShape(Shape);
            ++processed;
        }
    }
    //if (ShapeBVH)
    //{
    //    ShapeBVH->FlushRebuild();
    //}
}

void UCollisionManager::ProcessShape(UShapeComponent* Shape)
{
    if (!Shape) return;

    if (!Shape->IsCollisionEnabled() || !Shape->GetGenerateOverlapEvents())
    {
        // If disabled, remove all current overlaps and fire end events.
        const TArray<FOverlapInfo>& OldInfos = Shape->GetOverlapInfos();
        for (const FOverlapInfo& Info : OldInfos)
        {
            if (UPrimitiveComponent* OtherComp = Info.OtherComp)
            {
                // Remove from other side
                TArray<FOverlapInfo>& OtherList = const_cast<TArray<FOverlapInfo>&>(OtherComp->GetOverlapInfos());
                OtherList.Remove(Info);
                // Fire end events on both sides
                Shape->BroadcastEndOverlap(Info.OtherActor, OtherComp);
                OtherComp->BroadcastEndOverlap(Shape->GetOwner(), Shape);
            }
        }
        // Clear own list
        TArray<FOverlapInfo>& SelfList = const_cast<TArray<FOverlapInfo>&>(Shape->GetOverlapInfos());
        SelfList.Empty();
        return;
    }

    // Update BVH entry to current bounds
    //if (ShapeBVH)
    //{
    //    ShapeBVH->Update(Shape, Shape->GetBroadphaseAABB());
    //}

    // Build set from current overlaps
    TSet<UPrimitiveComponent*> OldSet;
    for (const FOverlapInfo& Info : Shape->GetOverlapInfos())
    {
        if (Info.OtherComp)
            OldSet.insert(Info.OtherComp);
    }

    // Compute new overlaps via BVH candidates
    TSet<UShapeComponent*> NewSet;
    TArray<UShapeComponent*> Candidates;
    //if (ShapeBVH)
    //{
    //    const FAABB QueryBox = Shape->GetBroadphaseAABB();
    //    ShapeBVH->Query(QueryBox, Candidates);
    //}
    //else
    {
        // Fallback: iterate all active shapes
        for (UShapeComponent* S : ActiveShapes) Candidates.Add(S);
    }
    for (UShapeComponent* Other : Candidates)
    {
        if (!Other || Other == Shape) continue;
        if (!Other->IsCollisionEnabled() || !Other->GetGenerateOverlapEvents()) continue;
        if (Shape->Overlaps(Other)) NewSet.insert(Other);
    }

    // Compute begins (New - Old)
    for (UShapeComponent* Other : NewSet)
    {
        if (OldSet.count(Other) == 0)
        {
            // Add to both components' overlap lists
            FOverlapInfo InfoAB; InfoAB.OtherActor = Other->GetOwner(); InfoAB.OtherComp = Other;
            FOverlapInfo InfoBA; InfoBA.OtherActor = Shape->GetOwner(); InfoBA.OtherComp = Shape;

            TArray<FOverlapInfo>& AList = const_cast<TArray<FOverlapInfo>&>(Shape->GetOverlapInfos());
            if (!AList.Contains(InfoAB)) AList.Add(InfoAB);

            TArray<FOverlapInfo>& BList = const_cast<TArray<FOverlapInfo>&>(Other->GetOverlapInfos());
            if (!BList.Contains(InfoBA)) BList.Add(InfoBA);

            // Fire begin events on both sides
            Shape->BroadcastBeginOverlap(Other->GetOwner(), Other);
            Other->BroadcastBeginOverlap(Shape->GetOwner(), Shape);
        }
    }

    // Compute ends (Old - New)
    for (UPrimitiveComponent* OldOther : OldSet)
    {
        UShapeComponent* OtherShape = Cast<UShapeComponent>(OldOther);
        if (!OtherShape) continue;
        if (NewSet.count(OtherShape) == 0)
        {
            FOverlapInfo InfoAB; InfoAB.OtherActor = OtherShape->GetOwner(); InfoAB.OtherComp = OtherShape;
            FOverlapInfo InfoBA; InfoBA.OtherActor = Shape->GetOwner(); InfoBA.OtherComp = Shape;

            TArray<FOverlapInfo>& AList = const_cast<TArray<FOverlapInfo>&>(Shape->GetOverlapInfos());
            AList.Remove(InfoAB);

            TArray<FOverlapInfo>& BList = const_cast<TArray<FOverlapInfo>&>(OtherShape->GetOverlapInfos());
            BList.Remove(InfoBA);

            // Fire end events on both sides
            Shape->BroadcastEndOverlap(OtherShape->GetOwner(), OtherShape);
            OtherShape->BroadcastEndOverlap(Shape->GetOwner(), Shape);
        }
    }
}

void UCollisionManager::QueryAABB(const FAABB& QueryBox, TArray<UShapeComponent*>& OutCandidates)
{
    OutCandidates.Empty();
    //if (ShapeBVH)
    //{
    //    ShapeBVH->Query(QueryBox, OutCandidates);
    //}
    //else
    {
        for (UShapeComponent* S : ActiveShapes) OutCandidates.Add(S);
    }
}
