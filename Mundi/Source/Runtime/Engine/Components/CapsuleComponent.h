#pragma once
#include "ShapeComponent.h"
#include "Capsule.h"

class UCapsuleComponent : public UShapeComponent
{
public:
    DECLARE_CLASS(UCapsuleComponent, UShapeComponent)
    GENERATED_REFLECTION_BODY()
    DECLARE_DUPLICATE(UCapsuleComponent)

    UCapsuleComponent();
    virtual ~UCapsuleComponent() override;
    
    float CapsuleHalfHeight;
    float CapsuleRadius;

    FCapsule GetWorldCapsule() const;
    void DebugDraw() const override;
    bool Overlaps(const UShapeComponent* Other, FContactInfo* OutContactInfo = nullptr) const override;
    struct FAABB GetBroadphaseAABB() const override;
};
