#pragma once
struct FAABB;
struct FOBB;
struct FBoundingSphere;

namespace Collision
{
    bool OverlapAABBSphere(const FAABB& Aabb, const FBoundingSphere& Sphere);
    bool OverlapAABBOBB(const FAABB& Aabb, const FOBB& Obb);
	bool OverlapOBBSphere(const FOBB& Obb, const FBoundingSphere& Sphere);
	bool OverlapOBBOBB(const FOBB& A, const FOBB& B);
	bool OverlapSphereSphere(const FBoundingSphere& A, const FBoundingSphere& B);
}