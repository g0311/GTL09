#pragma once
#include "ShapeComponent.h"

class USphereComponent : public UShapeComponent
{
public:
    DECLARE_CLASS(USphereComponent, UShapeComponent)
    GENERATED_REFLECTION_BODY()
    DECLARE_DUPLICATE(USphereComponent)

    USphereComponent();
    virtual ~USphereComponent() override;
    
    void DebugDraw() const override;
    struct FBoundingSphere GetWorldSphere() const;
    bool Overlaps(const UShapeComponent* Other, FContactInfo* OutContactInfo = nullptr) const override;
    struct FAABB GetBroadphaseAABB() const override;

private:
    float SphereRadius;
};
