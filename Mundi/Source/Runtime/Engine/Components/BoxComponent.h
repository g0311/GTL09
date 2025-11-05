#pragma once
#include "ShapeComponent.h"

/**
 * A shape component representing an axis-aligned box used for collision and physics.
 *
 * Provides storage for the box's half-extents and routines to query its world-space
 * bounds, perform overlap tests, and draw debug visualization.
 */

/**
 * Default-construct a box component with default extent values.
 */

/**
 * Destroy the box component and release any component-specific resources.
 */

/**
 * Half-dimensions of the box along the X, Y and Z axes (extent = half-size).
 *
 * Measured in local component space units.
 */

/**
 * Draw a debug visualization of the box component.
 *
 * Visualization is intended for debugging and editor use and does not affect physics.
 */

/**
 * Retrieve the box's oriented bounding box (OBB) in world space.
 *
 * @returns The world-space oriented bounding box that encloses this box component.
 */

/**
 * Determine whether this box overlaps another shape component.
 *
 * @param Other Pointer to the other shape component to test against.
 * @param OutContactInfo Optional output pointer that, if provided and an overlap occurs,
 *        will be populated with contact information describing the overlap.
 * @returns `true` if this box overlaps `Other`, `false` otherwise.
 */

/**
 * Compute the axis-aligned bounding box (AABB) used for broadphase collision checks.
 *
 * @returns The axis-aligned bounding box in world space that encloses this box component.
 */
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