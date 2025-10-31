#include "pch.h"
#include "CapsuleComponent.h"

FCapsule UCapsuleComponent::GetWorldCapsule() const
{
    const FTransform& WorldTransform = GetWorldTransform();

    // The capsule's local axis (Z axis convention for capsule orientation)
    const FVector CapsuleAxisWorld = WorldTransform.GetUnitAxis(EAxis::Z);

    // World-space center of the capsule component
    const FVector CapsuleCenterWorld = WorldTransform.TransformPosition(FVector(0, 0, 0));

    // Absolute world scale values
    const FVector WorldScale = GetWorldScale();

    // Scaling factors (simple conservative approach — same scale for radius and half-height)
    const float RadiusScale = std::max({ WorldScale.X, WorldScale.Y, WorldScale.Z });
    const float HalfHeightScale = RadiusScale; // conservative; can be made more accurate if projecting scale along axis

    // Final scaled radius & half-height
    const float CapsuleRadiusWorld = CapsuleRadius * RadiusScale;
    const float CapsuleHalfHeightWorld = CapsuleHalfHeight * HalfHeightScale;

    // Compute world-space capsule end points
    const FVector CapsuleEndPointTop    = CapsuleCenterWorld + CapsuleAxisWorld * CapsuleHalfHeightWorld;
    const FVector CapsuleEndPointBottom = CapsuleCenterWorld - CapsuleAxisWorld * CapsuleHalfHeightWorld;

    return FCapsule(CapsuleEndPointTop, CapsuleEndPointBottom, CapsuleRadiusWorld);
}