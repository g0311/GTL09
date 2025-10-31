#pragma once
#include "Object.h"

class UShapeComponent;
class UPrimitiveComponent;
class UWorld;
class AActor;
class FShapeBVH;

// Centralized collision/overlap manager for UShapeComponent pairs.
// Computes begin/end overlap diffs and updates components' OverlapInfos.
class UCollisionManager : public UObject
{
public:
    DECLARE_CLASS(UCollisionManager, UObject)

    UCollisionManager();
    ~UCollisionManager() override;

    void Register(UShapeComponent* Shape);
    void Unregister(UShapeComponent* Shape);

    // Mark a shape as needing overlap recomputation.
    void MarkDirty(UShapeComponent* Shape);

    // Process dirty shapes; budget limits number processed per frame.
    void Update(float DeltaTime, uint32 Budget = 256);

private:
    // Active shape components in the world
    TSet<UShapeComponent*> ActiveShapes;

    // Dirty queue
    TQueue<UShapeComponent*> DirtyQueue;
    TSet<UShapeComponent*>   DirtySet;

    // Internal helpers
    void ProcessShape(UShapeComponent* Shape);

    // Broadphase
    FShapeBVH* ShapeBVH = nullptr;
};
