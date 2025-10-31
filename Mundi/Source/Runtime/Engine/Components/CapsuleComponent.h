#pragma once
#include "ShapeComponent.h"
#include "Capsule.h"

class UCapsuleComponent : public UShapeComponent
{
public:
    float CapsuleHalfHeight;
    float CapsuleRadius;

    FCapsule GetWorldCapsule() const;
};
