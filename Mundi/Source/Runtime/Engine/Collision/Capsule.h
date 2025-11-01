#pragma once

struct FCapsule
{
    ECollisionShapeType ShapeType = ECollisionShapeType::Capsule;
    
    FVector Center1; // center of one hemisphere
    FVector Center2; // center of the other hemisphere
    float Radius;

    FCapsule();
    FCapsule(const FVector& InCenter1, const FVector& InCenter2, float InRadius);
    virtual ~FCapsule() = default;

    float GetRadius() const { return Radius; }

    bool Contains(const FVector& Point) const;
    bool Contains(const FCapsule& Other) const;
    bool Intersects(const FCapsule& Other) const;
};