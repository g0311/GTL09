#pragma once
#include "ShapeComponent.h"
#include "Capsule.h"

/**
 * Capsule half-height measured along the capsule's local up axis.
 * Represents half the distance between the spherical caps in local units.
 */

/**
 * Capsule radius measured in local units.
 */

/**
 * Get the capsule transformed into world space.
 *
 * @returns FCapsule representing this component's capsule in world coordinates.
 */

/**
 * Render a debug visualization of the capsule.
 */

/**
 * Determine whether this capsule overlaps another shape component.
 *
 * @param Other Shape component to test against.
 * @param OutContactInfo If non-null and an overlap occurs, receives contact details.
 * @returns `true` if the capsule overlaps the other shape, `false` otherwise.
 */

/**
 * Compute the axis-aligned bounding box used for broadphase collision checks.
 *
 * @returns FAABB axis-aligned bounding box that encloses this capsule in world space.
 */
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