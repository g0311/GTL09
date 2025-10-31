#pragma once
#include "Color.h"
#include "PrimitiveComponent.h"

class UShapeComponent : public UPrimitiveComponent
{
public:
    DECLARE_CLASS(UShapeComponent, UPrimitiveComponent)
    GENERATED_REFLECTION_BODY()
    
    UShapeComponent();

    ECollisionShapeType CollisionShape = ECollisionShapeType::Sphere;
    FLinearColor ShapeColor = FLinearColor::Green;
    bool bDrawOnlyIfSelected = false;

    virtual void DebugDraw() const {};

    // Hook into debug rendering pass to call shape DebugDraw
    virtual void RenderDebugVolume(class URenderer* Renderer) const override { DebugDraw(); }

    // Virtual shape-vs-shape overlap dispatch
    virtual bool Overlaps(const UShapeComponent* Other) const { return false; }
    ECollisionShapeType GetCollisionShapeType() const { return CollisionShape; }

    // Mark overlaps dirty when transform changes (central manager will recompute)
    void OnTransformUpdated() override;
    void OnRegister(UWorld* InWorld) override;
    void OnUnregister() override;
};
