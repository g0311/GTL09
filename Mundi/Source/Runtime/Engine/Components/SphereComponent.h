#pragma once
#include "ShapeComponent.h"

class USphereComponent : public UShapeComponent
{
public:
    DECLARE_CLASS(USphereComponent, UShapeComponent)
    GENERATED_REFLECTION_BODY()

    USphereComponent();
    virtual ~USphereComponent() override;
    
    void DebugDraw() const override;
    struct FBoundingSphere GetWorldSphere() const;

private:
    float SphereRadius;
};
