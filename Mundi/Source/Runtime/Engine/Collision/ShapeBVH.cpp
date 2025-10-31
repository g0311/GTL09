#include "pch.h"
#include "ShapeBVH.h"
#include "SceneComponent.h"
#include <algorithm>

void FShapeBVH::Clear()
{
    Entries.Empty();
    EntryMap.clear();
    Nodes.Empty();
    Root = -1;
    bDirty = false;
}

void FShapeBVH::Insert(UShapeComponent* Shape, const FAABB& Bounds)
{
    if (!Shape) return;
    int32 idx;
    if (EntryMap.Contains(Shape))
    {
        idx = EntryMap[Shape];
        Entries[idx].Bounds = Bounds;
    }
    else
    {
        idx = Entries.Num();
        FEntry E; E.Shape = Shape; E.Bounds = Bounds;
        Entries.Add(E);
        EntryMap[Shape] = idx;
    }
    bDirty = true;
}

void FShapeBVH::Update(UShapeComponent* Shape, const FAABB& Bounds)
{
    Insert(Shape, Bounds);
}

void FShapeBVH::Remove(UShapeComponent* Shape)
{
    if (!Shape) return;
    auto* Found = EntryMap.Find(Shape);
    if (!Found) return;
    int32 idx = *Found;
    // swap-erase
    int32 last = Entries.Num() - 1;
    if (idx != last)
    {
        Entries[idx] = Entries[last];
        EntryMap[Entries[idx].Shape] = idx;
    }
    Entries.RemoveAt(last);
    EntryMap.erase(Shape);
    bDirty = true;
}

void FShapeBVH::Query(const FAABB& QueryBox, TArray<UShapeComponent*>& OutCandidates)
{
    OutCandidates.Empty();
    if (bDirty) Build();
    if (Root < 0 || Nodes.IsEmpty()) return;

    TArray<int32> Stack;
    Stack.Add(Root);
    while (!Stack.IsEmpty())
    {
        int32 n = Stack.Pop();
        const FNode& Node = Nodes[n];
        if (!Node.Bounds.Intersects(QueryBox))
            continue;
        if (Node.IsLeaf())
        {
            const FEntry& E = Entries[Node.EntryIndex];
            OutCandidates.Add(E.Shape);
        }
        else
        {
            if (Node.Left >= 0) Stack.Add(Node.Left);
            if (Node.Right >= 0) Stack.Add(Node.Right);
        }
    }
}

void FShapeBVH::FlushRebuild()
{
    if (bDirty) Build();
}

void FShapeBVH::Build()
{
    Nodes.Empty();
    Root = -1;
    if (Entries.IsEmpty()) { bDirty = false; return; }
    // Build a temporary index list
    TArray<int32> Idx;
    Idx.Reserve(Entries.Num());
    for (int32 i = 0; i < Entries.Num(); ++i) Idx.Add(i);

    Root = BuildRange(Idx, 0, Idx.Num());
    bDirty = false;
}

int32 FShapeBVH::BuildRange(TArray<int32>& Indices, int32 Begin, int32 End)
{
    const int32 Count = End - Begin;
    if (Count <= 0) return -1;

    if (Count == 1)
    {
        const int32 EntryIndex = Indices[Begin];
        FNode Leaf;
        Leaf.EntryIndex = EntryIndex;
        Leaf.Bounds = Entries[EntryIndex].Bounds;
        const int32 NodeIndex = Nodes.Num();
        Nodes.Add(Leaf);
        return NodeIndex;
    }

    // Compute bounds and choose split axis by largest extent
    FAABB Bounds = Entries[Indices[Begin]].Bounds;
    for (int32 i = Begin + 1; i < End; ++i)
    {
        Bounds = FAABB::Union(Bounds, Entries[Indices[i]].Bounds);
    }
    const FVector Ext = Bounds.GetHalfExtent() * 2.0f;
    int Axis = 0;
    if (Ext.Y > Ext.X && Ext.Y >= Ext.Z) Axis = 1; else if (Ext.Z > Ext.X && Ext.Z >= Ext.Y) Axis = 2;

    // Sort by centroid along chosen axis
    std::sort(Indices.begin() + Begin, Indices.begin() + End, [&](int a, int b)
    {
        const FVector Ca = Entries[a].Bounds.GetCenter();
        const FVector Cb = Entries[b].Bounds.GetCenter();
        if (Axis == 0) return Ca.X < Cb.X; if (Axis == 1) return Ca.Y < Cb.Y; return Ca.Z < Cb.Z;
    });

    const int32 Mid = Begin + Count / 2;
    const int32 Left = BuildRange(Indices, Begin, Mid);
    const int32 Right = BuildRange(Indices, Mid, End);

    FNode Node;
    Node.Left = Left; Node.Right = Right;
    Node.EntryIndex = -1;
    Node.Bounds = Bounds;
    const int32 NodeIndex = Nodes.Num();
    Nodes.Add(Node);

    if (Left >= 0) Nodes[Left].Parent = NodeIndex;
    if (Right >= 0) Nodes[Right].Parent = NodeIndex;
    return NodeIndex;
}
