#pragma once
#include "ShapeComponent.h"
#include "Capsule.h"

class UCapsuleComponent : public UShapeComponent
{
public:
    DECLARE_CLASS(UCapsuleComponent, UShapeComponent)
    GENERATED_REFLECTION_BODY()

    UCapsuleComponent();
    virtual ~UCapsuleComponent() override;
    
    float CapsuleHalfHeight;
    float CapsuleRadius;

    FCapsule GetWorldCapsule() const;
    void DebugDraw() const override;
    bool Overlaps(const UShapeComponent* Other) const override;
    struct FAABB GetBroadphaseAABB() const override;
};
