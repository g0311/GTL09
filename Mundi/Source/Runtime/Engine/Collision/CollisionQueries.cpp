#include "pch.h"
#include "CollisionQueries.h"
#include "AABB.h"
#include "OBB.h"
#include "BoundingSphere.h"
#include "Capsule.h"
#include "PrimitiveComponent.h"

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
    bool OverlapAABBOBB(const FAABB& Aabb, const FOBB& Obb, FContactInfo* OutContactInfo)
    {
        const FOBB ConvertedOBB(Aabb, FMatrix::Identity());
        bool bOverlaps = ConvertedOBB.Intersects(Obb);

        if (bOverlaps && OutContactInfo)
        {
            FVector aabbCenter = (Aabb.Min + Aabb.Max) * 0.5f;
            OutContactInfo->ContactPoint = (aabbCenter + Obb.Center) * 0.5f;

            FVector AtoB = Obb.Center - aabbCenter;
            float distance = AtoB.Size();
            if (distance > KINDA_SMALL_NUMBER)
            {
                OutContactInfo->ContactNormal = AtoB / distance;
            }
            else
            {
                OutContactInfo->ContactNormal = FVector(0, 0, 1);
            }
            OutContactInfo->PenetrationDepth = 0.0f;
        }

        return bOverlaps;
    }

    bool OverlapAABBSphere(const FAABB& Aabb, const FBoundingSphere& Sphere, FContactInfo* OutContactInfo)
    {
        // Clamp sphere center to AABB, then compare squared distance to radius^2
        float dist2 = 0.0f;
        FVector closestPoint;
        for (int i = 0; i < 3; ++i)
        {
            float v = Sphere.Center[i];
            if (v < Aabb.Min[i])
            {
                closestPoint[i] = Aabb.Min[i];
                float d = Aabb.Min[i] - v;
                dist2 += d * d;
            }
            else if (v > Aabb.Max[i])
            {
                closestPoint[i] = Aabb.Max[i];
                float d = v - Aabb.Max[i];
                dist2 += d * d;
            }
            else
            {
                closestPoint[i] = v;
            }
        }

        bool bOverlaps = dist2 <= (Sphere.Radius * Sphere.Radius);

        if (bOverlaps && OutContactInfo)
        {
            FVector toSphere = Sphere.Center - closestPoint;
            float distance = toSphere.Size();

            if (distance > KINDA_SMALL_NUMBER)
            {
                OutContactInfo->ContactNormal = toSphere / distance;
                // ContactPoint: AABB 표면과 Sphere 표면의 중간 지점
                FVector aabbSurface = closestPoint;
                FVector sphereSurface = Sphere.Center - OutContactInfo->ContactNormal * Sphere.Radius;
                OutContactInfo->ContactPoint = (aabbSurface + sphereSurface) * 0.5f;
                OutContactInfo->PenetrationDepth = Sphere.Radius - distance;
            }
            else
            {
                // Sphere center inside AABB
                OutContactInfo->ContactPoint = closestPoint;
                OutContactInfo->ContactNormal = FVector(0, 0, 1);
                OutContactInfo->PenetrationDepth = Sphere.Radius;
            }
        }

        return bOverlaps;
    }

    bool OverlapOBBSphere(const FOBB& Obb, const FBoundingSphere& Sphere, FContactInfo* OutContactInfo)
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
        bool bOverlaps = dist2 <= (Sphere.Radius * Sphere.Radius);

        if (bOverlaps && OutContactInfo)
        {
            FVector toSphere = Sphere.Center - closest;
            float distance = toSphere.Size();

            if (distance > KINDA_SMALL_NUMBER)
            {
                OutContactInfo->ContactNormal = toSphere / distance;
                // ContactPoint: OBB 표면과 Sphere 표면의 중간 지점
                FVector obbSurface = closest;
                FVector sphereSurface = Sphere.Center - OutContactInfo->ContactNormal * Sphere.Radius;
                OutContactInfo->ContactPoint = (obbSurface + sphereSurface) * 0.5f;
                OutContactInfo->PenetrationDepth = Sphere.Radius - distance;
            }
            else
            {
                // Sphere center is inside OBB
                OutContactInfo->ContactPoint = closest;
                OutContactInfo->ContactNormal = FVector(0, 0, 1);
                OutContactInfo->PenetrationDepth = Sphere.Radius;
            }
        }

        return bOverlaps;
    }

    bool OverlapOBBOBB(const FOBB& A, const FOBB& B, FContactInfo* OutContactInfo)
    {
        bool bOverlaps = A.Intersects(B);

        if (bOverlaps && OutContactInfo)
        {
            FVector AtoB = B.Center - A.Center;
            float distance = AtoB.Size();

            if (distance > KINDA_SMALL_NUMBER)
            {
                OutContactInfo->ContactNormal = AtoB / distance;

                // A 박스에서 B 방향으로의 표면점 계산 (간단한 근사)
                FVector surfaceA = A.Center;
                for (int i = 0; i < 3; ++i)
                {
                    float proj = FVector::Dot(OutContactInfo->ContactNormal, A.Axes[i]);
                    surfaceA += A.Axes[i] * (proj > 0 ? A.HalfExtent[i] : -A.HalfExtent[i]);
                }

                // B 박스에서 A 방향으로의 표면점 계산
                FVector surfaceB = B.Center;
                for (int i = 0; i < 3; ++i)
                {
                    float proj = FVector::Dot(-OutContactInfo->ContactNormal, B.Axes[i]);
                    surfaceB += B.Axes[i] * (proj > 0 ? B.HalfExtent[i] : -B.HalfExtent[i]);
                }

                // 두 표면점의 중간을 ContactPoint로 사용
                OutContactInfo->ContactPoint = (surfaceA + surfaceB) * 0.5f;
            }
            else
            {
                // 중심이 같은 경우
                OutContactInfo->ContactPoint = A.Center;
                OutContactInfo->ContactNormal = FVector(0, 0, 1);
            }

            OutContactInfo->PenetrationDepth = 0.0f; // OBB vs OBB는 계산 복잡
        }

        return bOverlaps;
    }

    bool OverlapSphereSphere(const FBoundingSphere& A, const FBoundingSphere& B, FContactInfo* OutContactInfo)
    {
        bool bOverlaps = A.Intersects(B);

        if (bOverlaps && OutContactInfo)
        {
            // 두 구의 중심을 잇는 벡터
            FVector AtoB = B.Center - A.Center;
            float distance = AtoB.Size();

            if (distance > KINDA_SMALL_NUMBER)
            {
                // ContactNormal: A에서 B로 향하는 방향
                OutContactInfo->ContactNormal = AtoB / distance;

                // ContactPoint: A 표면과 B 표면의 중간 지점
                FVector surfaceA = A.Center + OutContactInfo->ContactNormal * A.Radius;
                FVector surfaceB = B.Center - OutContactInfo->ContactNormal * B.Radius;
                OutContactInfo->ContactPoint = (surfaceA + surfaceB) * 0.5f;

                // PenetrationDepth: 겹친 깊이
                OutContactInfo->PenetrationDepth = (A.Radius + B.Radius) - distance;
            }
            else
            {
                // 완전히 겹친 경우 (중심이 같음)
                OutContactInfo->ContactPoint = A.Center;
                OutContactInfo->ContactNormal = FVector(0, 0, 1); // 임의 방향
                OutContactInfo->PenetrationDepth = A.Radius + B.Radius;
            }
        }

        return bOverlaps;
    }

    bool OverlapCapsuleSphere(const FCapsule& Capsule, const FBoundingSphere& Sphere, FContactInfo* OutContactInfo)
    {
        const float dist2 = DistPointSegmentSq(Sphere.Center, Capsule.Center1, Capsule.Center2);
        const float r = Capsule.Radius + Sphere.Radius;
        bool bOverlaps = dist2 <= (r * r);

        if (bOverlaps && OutContactInfo)
        {
            // Capsule의 중심 선분에서 Sphere에 가장 가까운 점 찾기
            FVector AB = Capsule.Center2 - Capsule.Center1;
            FVector AP = Sphere.Center - Capsule.Center1;
            float abLen2 = FVector::Dot(AB, AB);

            FVector closestOnSegment;
            if (abLen2 <= KINDA_SMALL_NUMBER)
            {
                closestOnSegment = Capsule.Center1;
            }
            else
            {
                float t = FVector::Dot(AP, AB) / abLen2;
                t = FMath::Clamp(t, 0.0f, 1.0f);
                closestOnSegment = Capsule.Center1 + AB * t;
            }

            FVector toSphere = Sphere.Center - closestOnSegment;
            float distance = toSphere.Size();

            if (distance > KINDA_SMALL_NUMBER)
            {
                OutContactInfo->ContactNormal = toSphere / distance;
                // ContactPoint: Capsule 표면과 Sphere 표면의 중간 지점
                FVector capsuleSurface = closestOnSegment + OutContactInfo->ContactNormal * Capsule.Radius;
                FVector sphereSurface = Sphere.Center - OutContactInfo->ContactNormal * Sphere.Radius;
                OutContactInfo->ContactPoint = (capsuleSurface + sphereSurface) * 0.5f;
                OutContactInfo->PenetrationDepth = (Capsule.Radius + Sphere.Radius) - distance;
            }
            else
            {
                OutContactInfo->ContactPoint = closestOnSegment;
                OutContactInfo->ContactNormal = FVector(0, 0, 1);
                OutContactInfo->PenetrationDepth = Capsule.Radius + Sphere.Radius;
            }
        }

        return bOverlaps;
    }

    bool OverlapCapsuleCapsule(const FCapsule& A, const FCapsule& B, FContactInfo* OutContactInfo)
    {
        const float dist2 = DistSegmentSegmentSq(A.Center1, A.Center2, B.Center1, B.Center2);
        const float r = A.Radius + B.Radius;
        bool bOverlaps = dist2 <= (r * r);

        if (bOverlaps && OutContactInfo)
        {
            // 간단하게 두 Capsule의 중간 지점을 충돌 위치로 사용
            FVector centerA = (A.Center1 + A.Center2) * 0.5f;
            FVector centerB = (B.Center1 + B.Center2) * 0.5f;
            OutContactInfo->ContactPoint = (centerA + centerB) * 0.5f;

            FVector AtoB = centerB - centerA;
            float distance = AtoB.Size();
            if (distance > KINDA_SMALL_NUMBER)
            {
                OutContactInfo->ContactNormal = AtoB / distance;
            }
            else
            {
                OutContactInfo->ContactNormal = FVector(0, 0, 1);
            }
            OutContactInfo->PenetrationDepth = r - sqrt(dist2);
        }

        return bOverlaps;
    }

    bool OverlapOBBCapsule(const FOBB& Obb, const FCapsule& Capsule, FContactInfo* OutContactInfo)
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
        bool bOverlaps = SegmentAABBIntersect(P0l, P1l, Bmin, Bmax);

        if (bOverlaps && OutContactInfo)
        {
            // 간단하게 OBB 중심과 Capsule 중심의 중간 지점 사용
            FVector capsuleCenter = (Capsule.Center1 + Capsule.Center2) * 0.5f;
            OutContactInfo->ContactPoint = (Obb.Center + capsuleCenter) * 0.5f;

            FVector AtoB = capsuleCenter - Obb.Center;
            float distance = AtoB.Size();
            if (distance > KINDA_SMALL_NUMBER)
            {
                OutContactInfo->ContactNormal = AtoB / distance;
            }
            else
            {
                OutContactInfo->ContactNormal = FVector(0, 0, 1);
            }
            OutContactInfo->PenetrationDepth = 0.0f;
        }

        return bOverlaps;
    }
}
