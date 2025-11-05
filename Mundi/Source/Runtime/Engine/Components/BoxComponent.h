#pragma once
#include "ShapeComponent.h"

class UBoxComponent : public UShapeComponent
{
public:
    DECLARE_CLASS(UBoxComponent, UShapeComponent)
    GENERATED_REFLECTION_BODY()
    DECLARE_DUPLICATE(UBoxComponent)

    UBoxComponent();
    virtual ~UBoxComponent() override;
    
    FVector BoxExtent;

    void DebugDraw() const override;
    struct FOBB GetWorldOBB() const;
    bool Overlaps(const UShapeComponent* Other, FContactInfo* OutContactInfo = nullptr) const override;
    struct FAABB GetBroadphaseAABB() const override;
};
