#pragma once
#include "ShapeComponent.h"

class UBoxComponent : public UShapeComponent
{
public:
    DECLARE_CLASS(UBoxComponent, UShapeComponent)
    GENERATED_REFLECTION_BODY()

    UBoxComponent();
    virtual ~UBoxComponent() override;
    
    FVector BoxExtent;

    void DebugDraw() const override;
};
