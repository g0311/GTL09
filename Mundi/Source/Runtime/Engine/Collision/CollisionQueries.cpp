#include "pch.h"
#include "CollisionQueries.h"
#include "AABB.h"
#include "OBB.h"
#include "BoundingSphere.h"
#include "Capsule.h"

namespace
{
    inline float DistPointSegmentSq(const FVector& P, const FVector& A, const FVector& B)
    {
        const FVector AB = B - A;
        const FVector AP = P - A;
        const float abLen2 = FVector::Dot(AB, AB);
        if (abLen2 <= KINDA_SMALL_NUMBER)
        {
            return (P - A).SizeSquared();
        }
        float t = FVector::Dot(AP, AB) / abLen2;
        t = FMath::Clamp(t, 0.0f, 1.0f);
        const FVector Closest = A + AB * t;
        return (P - Closest).SizeSquared();
    }

    inline float DistSegmentSegmentSq(const FVector& P0, const FVector& P1,
                                      const FVector& Q0, const FVector& Q1)
    {
        const FVector u = P1 - P0;
        const FVector v = Q1 - Q0;
        const FVector w = P0 - Q0;
        const float a = FVector::Dot(u, u); // always >= 0
        const float b = FVector::Dot(u, v);
        const float c = FVector::Dot(v, v); // always >= 0
        const float d = FVector::Dot(u, w);
        const float e = FVector::Dot(v, w);
        const float D = a * c - b * b;      // always >= 0

        float sc, sN, sD = D; // sc = sN / sD
        float tc, tN, tD = D; // tc = tN / tD

        // compute the line parameters of the two closest points
        if (D < KINDA_SMALL_NUMBER)
        {
            // the lines are almost parallel
            sN = 0.0f;     // force using s = 0.0
            sD = 1.0f;     // to avoid divide by zero
            tN = e;
            tD = c;
        }
        else
        {
            // get the closest points on the infinite lines
            sN = (b * e - c * d);
            tN = (a * e - b * d);
            // clamp s to [0,1]
            if (sN < 0.0f)
            {
                sN = 0.0f;
                tN = e;
                tD = c;
            }
            else if (sN > sD)
            {
                sN = sD;
                tN = e + b;
                tD = c;
            }
        }

        // clamp t to [0,1]
        if (tN < 0.0f)
        {
            tN = 0.0f;
            // recompute s for this edge
            if (-d < 0.0f)
                sN = 0.0f;
            else if (-d > a)
                sN = sD;
            else
            {
                sN = -d;
                sD = a;
            }
        }
        else if (tN > tD)
        {
            tN = tD;
            // recompute s for this edge
            if ((-d + b) < 0.0f)
                sN = 0.0f;
            else if ((-d + b) > a)
                sN = sD;
            else
            {
                sN = (-d + b);
                sD = a;
            }
        }

        sc = (FMath::Abs(sN) < KINDA_SMALL_NUMBER ? 0.0f : sN / sD);
        tc = (FMath::Abs(tN) < KINDA_SMALL_NUMBER ? 0.0f : tN / tD);

        const FVector dP = w + (u * sc) - (v * tc); // =  P(sc) - Q(tc)
        return dP.SizeSquared();
    }

    inline bool SegmentAABBIntersect(const FVector& P0, const FVector& P1,
                                     const FVector& Bmin, const FVector& Bmax)
    {
        FVector d = P1 - P0;
        float tmin = 0.0f;
        float tmax = 1.0f;
        for (int i = 0; i < 3; ++i)
        {
            const float dir = d[i];
            const float orig = P0[i];
            if (std::fabs(dir) < KINDA_SMALL_NUMBER)
            {
                // Segment is parallel to slab. Reject if origin not within slab.
                if (orig < Bmin[i] || orig > Bmax[i])
                    return false;
            }
            else
            {
                const float invD = 1.0f / dir;
                float t1 = (Bmin[i] - orig) * invD;
                float t2 = (Bmax[i] - orig) * invD;
                if (t1 > t2) std::swap(t1, t2);
                tmin = std::max(tmin, t1);
                tmax = std::min(tmax, t2);
                if (tmin > tmax)
                    return false;
            }
        }
        return true;
    }
}

namespace Collision
{
    bool OverlapAABBOBB(const FAABB& Aabb, const FOBB& Obb)
    {
        const FOBB ConvertedOBB(Aabb, FMatrix::Identity());
        return ConvertedOBB.Intersects(Obb);
    }

    bool OverlapAABBSphere(const FAABB& Aabb, const FBoundingSphere& Sphere)
    {
        // Clamp sphere center to AABB, then compare squared distance to radius^2
        float dist2 = 0.0f;
        for (int i = 0; i < 3; ++i)
        {
            float v = Sphere.Center[i];
            if (v < Aabb.Min[i])
            {
                float d = Aabb.Min[i] - v;
                dist2 += d * d;
            }
            else if (v > Aabb.Max[i])
            {
                float d = v - Aabb.Max[i];
                dist2 += d * d;
            }
        }
        return dist2 <= (Sphere.Radius * Sphere.Radius);
    }

    bool OverlapOBBSphere(const FOBB& Obb, const FBoundingSphere& Sphere)
    {
        // Compute closest point on OBB to sphere center
        const FVector d = Sphere.Center - Obb.Center;
        FVector closest = Obb.Center;
        for (int i = 0; i < 3; ++i)
        {
            float t = FVector::Dot(d, Obb.Axes[i]);
            t = FMath::Clamp(t, -Obb.HalfExtent[i], Obb.HalfExtent[i]);
            closest += Obb.Axes[i] * t;
        }
        const float dist2 = (closest - Sphere.Center).SizeSquared();
        return dist2 <= (Sphere.Radius * Sphere.Radius);
    }

    bool OverlapOBBOBB(const FOBB& A, const FOBB& B)
    {
        return A.Intersects(B);
    }

    bool OverlapSphereSphere(const FBoundingSphere& A, const FBoundingSphere& B)
    {
        return A.Intersects(B);
    }

    bool OverlapCapsuleSphere(const FCapsule& Capsule, const FBoundingSphere& Sphere)
    {
        const float dist2 = DistPointSegmentSq(Sphere.Center, Capsule.Center1, Capsule.Center2);
        const float r = Capsule.Radius + Sphere.Radius;
        return dist2 <= (r * r);
    }

    bool OverlapCapsuleCapsule(const FCapsule& A, const FCapsule& B)
    {
        const float dist2 = DistSegmentSegmentSq(A.Center1, A.Center2, B.Center1, B.Center2);
        const float r = A.Radius + B.Radius;
        return dist2 <= (r * r);
    }

    bool OverlapOBBCapsule(const FOBB& Obb, const FCapsule& Capsule)
    {
        // Transform capsule segment into OBB local space
        const FVector P0w = Capsule.Center1;
        const FVector P1w = Capsule.Center2;
        const FVector OC0 = P0w - Obb.Center;
        const FVector OC1 = P1w - Obb.Center;

        FVector P0l(
            FVector::Dot(OC0, Obb.Axes[0]),
            FVector::Dot(OC0, Obb.Axes[1]),
            FVector::Dot(OC0, Obb.Axes[2]));
        FVector P1l(
            FVector::Dot(OC1, Obb.Axes[0]),
            FVector::Dot(OC1, Obb.Axes[1]),
            FVector::Dot(OC1, Obb.Axes[2]));

        // Expanded AABB in OBB local space (Minkowski sum with sphere radius)
        const FVector E = Obb.HalfExtent + FVector(Capsule.Radius, Capsule.Radius, Capsule.Radius);
        const FVector Bmin = FVector(-E.X, -E.Y, -E.Z);
        const FVector Bmax = FVector(+E.X, +E.Y, +E.Z);

        // Intersect segment with expanded box
        return SegmentAABBIntersect(P0l, P1l, Bmin, Bmax);
    }
}
