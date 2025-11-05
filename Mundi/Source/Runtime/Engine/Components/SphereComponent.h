#pragma once
#include "ShapeComponent.h"

/**
 * Sphere-shaped collision component that provides sphere-specific bounds and overlap queries.
 */

/**
 * Construct a USphereComponent with default parameters.
 */

/**
 * Destroy the USphereComponent and release any allocated resources.
 */

/**
 * Render a debug visualization of the sphere for debugging purposes.
 */

/**
 * Get the component's bounding sphere transformed into world space.
 * @returns The world-space bounding sphere for this component.
 */

/**
 * Determine whether this sphere overlaps another shape component.
 * @param Other The other shape component to test against.
 * @param OutContactInfo Optional output. If non-null and an overlap occurs, filled with contact information for the intersection.
 * @returns `true` if the two shapes overlap, `false` otherwise.
 */

/**
 * Get an axis-aligned bounding box that conservatively encloses this sphere for broadphase collision culling.
 * @returns The axis-aligned bounding box in world space that bounds this component.
 */
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