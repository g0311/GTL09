#include "pch.h"
#include "BoxComponent.h"
#include "RenderManager.h"
#include "AABB.h"
#include "OBB.h"

IMPLEMENT_CLASS(UBoxComponent)

BEGIN_PROPERTIES(UBoxComponent)
    MARK_AS_COMPONENT("박스 컴포넌트", "박스 컴포넌트입니다.")
END_PROPERTIES()

UBoxComponent::UBoxComponent()
{
    BoxExtent = FVector(10.0f, 10.0f, 10.0f);
}

UBoxComponent::~UBoxComponent()
{
    
}

void UBoxComponent::DebugDraw() const
{
    URenderer* Renderer = URenderManager::GetInstance().GetRenderer();
    if (!Renderer)
        return;

    const FVector4 Color = FLinearColor(ShapeColor).ToFVector4();

    // Build OBB from local AABB and world matrix
    const FVector Ext = BoxExtent; // half extents in local space
    const FAABB LocalAABB(FVector(-Ext.X, -Ext.Y, -Ext.Z), FVector(Ext.X, Ext.Y, Ext.Z));
    const FOBB OBB(LocalAABB, GetWorldMatrix());

    // Edge list from corners (same as in UDecalComponent)
    static const int Edges[12][2] = {
        {6, 4}, {7, 5}, {6, 7}, {4, 5},
        {4, 0}, {5, 1}, {6, 2}, {7, 3},
        {0, 2}, {1, 3}, {0, 1}, {2, 3}
    };

    const TArray<FVector> Corners = OBB.GetCorners();

    TArray<FVector> StartPoints;
    TArray<FVector> EndPoints;
    TArray<FVector4> Colors;
    StartPoints.reserve(12);
    EndPoints.reserve(12);
    Colors.reserve(12);

    for (int i = 0; i < 12; ++i)
    {
        const FVector& S = Corners[Edges[i][0]];
        const FVector& E = Corners[Edges[i][1]];
        StartPoints.Add(S);
        EndPoints.Add(E);
        Colors.Add(Color);
    }

    Renderer->AddLines(StartPoints, EndPoints, Colors);
}

