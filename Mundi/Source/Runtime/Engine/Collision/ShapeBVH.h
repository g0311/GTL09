#pragma once
#include "UEContainer.h"
#include "AABB.h"

class UShapeComponent;

// Simple BVH for shape-component broadphase queries.
class FShapeBVH
{
public:
    FShapeBVH() = default;
    ~FShapeBVH() = default;

    void Clear();

    void Insert(UShapeComponent* Shape, const FAABB& Bounds);
    void Update(UShapeComponent* Shape, const FAABB& Bounds);
    void Remove(UShapeComponent* Shape);

    // Query candidates intersecting query box
    void Query(const FAABB& QueryBox, TArray<UShapeComponent*>& OutCandidates);

    // Rebuild when dirty (called automatically by Query/Update as needed)
    void FlushRebuild();

private:
    struct FEntry
    {
        UShapeComponent* Shape = nullptr;
        FAABB Bounds;
    };

    struct FNode
    {
        FAABB Bounds;
        int32 Left = -1;
        int32 Right = -1;
        int32 Parent = -1;
        int32 EntryIndex = -1; // leaf has entry index >= 0
        bool IsLeaf() const { return EntryIndex >= 0; }
    };

    TArray<FEntry> Entries;
    TMap<UShapeComponent*, int32> EntryMap;

    TArray<FNode> Nodes;
    int32 Root = -1;
    bool bDirty = false;

private:
    void Build();
    int32 BuildRange(TArray<int32>& Indices, int32 Begin, int32 End);
};

