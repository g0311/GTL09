#include "pch.h"
#include "Capsule.h"

FCapsule::FCapsule() : Center1(0,0,0), Center2(1,1,1), Radius(0){}

FCapsule::FCapsule(const FVector& InCenter1, const FVector& InCenter2, float InRadius)
{
    Center1 = InCenter1;
    Center2 = InCenter2;
    Radius = InRadius;
}