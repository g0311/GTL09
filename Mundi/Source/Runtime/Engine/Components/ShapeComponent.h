#pragma once
#include "Color.h"
#include "PrimitiveComponent.h"

struct FAABB;

/**
 * Test whether this shape overlaps another shape and optionally produce contact details.
 *
 * @param Other The other shape component to test against.
 * @param OutContactInfo Optional pointer to receive contact information when an overlap is detected; may be null.
 * @returns `true` if the two shapes overlap, `false` otherwise.
 */
class UShapeComponent : public UPrimitiveComponent
{
public:
    DECLARE_CLASS(UShapeComponent, UPrimitiveComponent)
    GENERATED_REFLECTION_BODY()
    DECLARE_DUPLICATE(UShapeComponent)
    
    UShapeComponent();

    ECollisionShapeType CollisionShape = ECollisionShapeType::Sphere;
    FLinearColor ShapeColor = FLinearColor::Green;
    bool bDrawOnlyIfSelected = false;

    virtual void DebugDraw() const {};

    // Hook into debug rendering pass to call shape DebugDraw
    virtual void RenderDebugVolume(class URenderer* Renderer) const override;

    // Virtual shape-vs-shape overlap dispatch
    virtual bool Overlaps(const UShapeComponent* Other, FContactInfo* OutContactInfo = nullptr) const { return false; }
    ECollisionShapeType GetCollisionShapeType() const { return CollisionShape; }

    // Mark overlaps dirty when transform changes (central manager will recompute)
    void OnTransformUpdated() override;
    void OnRegister(UWorld* InWorld) override;
    void OnUnregister() override;

    // Broadphase AABB used by collision manager for candidate queries
    virtual FAABB GetBroadphaseAABB() const;
};