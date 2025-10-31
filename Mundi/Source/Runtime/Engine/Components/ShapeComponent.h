#pragma once
#include "Color.h"
#include "PrimitiveComponent.h"

class UShapeComponent : public UPrimitiveComponent
{
public:
    DECLARE_CLASS(UShapeComponent, UPrimitiveComponent)
    GENERATED_REFLECTION_BODY()
    
    UShapeComponent();
    
    FLinearColor ShapeColor = FLinearColor::Green;
    bool bDrawOnlyIfSelected = false;

    virtual void DebugDraw() const {};

    // Hook into debug rendering pass to call shape DebugDraw
    virtual void RenderDebugVolume(class URenderer* Renderer) const override { DebugDraw(); }
};
