#pragma once
struct FAABB;
struct FOBB;
struct FBoundingSphere;
struct FCapsule;
struct FContactInfo;

namespace Collision
{
    bool OverlapAABBSphere(const FAABB& Aabb, const FBoundingSphere& Sphere, FContactInfo* OutContactInfo = nullptr);
    bool OverlapAABBOBB(const FAABB& Aabb, const FOBB& Obb, FContactInfo* OutContactInfo = nullptr);

	bool OverlapOBBSphere(const FOBB& Obb, const FBoundingSphere& Sphere, FContactInfo* OutContactInfo = nullptr);
	bool OverlapOBBOBB(const FOBB& A, const FOBB& B, FContactInfo* OutContactInfo = nullptr);
	bool OverlapOBBCapsule(const FOBB& Obb, const FCapsule& Capsule, FContactInfo* OutContactInfo = nullptr);

	bool OverlapCapsuleSphere(const FCapsule& Capsule, const FBoundingSphere& Sphere, FContactInfo* OutContactInfo = nullptr);
	bool OverlapCapsuleCapsule(const FCapsule& A, const FCapsule& B, FContactInfo* OutContactInfo = nullptr);

	bool OverlapSphereSphere(const FBoundingSphere& A, const FBoundingSphere& B, FContactInfo* OutContactInfo = nullptr);
}