#pragma once
#include "Color.h"
#include "PrimitiveComponent.h"

class UShapeComponent : public UPrimitiveComponent
{
public:
    FLinearColor ShapeColor = FLinearColor::Green;
    bool bDrawOnlyIfSelected = false;

    virtual void DebugDraw() const {};
};
