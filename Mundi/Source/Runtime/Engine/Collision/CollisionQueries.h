#pragma once
struct FAABB;
struct FOBB;
struct FBoundingSphere;
struct FCapsule;

namespace Collision
{
    bool OverlapAABBSphere(const FAABB& Aabb, const FBoundingSphere& Sphere);
    bool OverlapAABBOBB(const FAABB& Aabb, const FOBB& Obb);
	
	bool OverlapOBBSphere(const FOBB& Obb, const FBoundingSphere& Sphere);
	bool OverlapOBBOBB(const FOBB& A, const FOBB& B);
	bool OverlapOBBCapsule(const FOBB& Obb, const FCapsule& Capsule);
	
	bool OverlapCapsuleSphere(const FCapsule& Capsule, const FBoundingSphere& Sphere);
	bool OverlapCapsuleCapsule(const FCapsule& A, const FCapsule& B);
	
	bool OverlapSphereSphere(const FBoundingSphere& A, const FBoundingSphere& B);
}