#pragma once
struct FAABB;
struct FOBB;
struct FBoundingSphere;
struct FCapsule;
struct FContactInfo;

/**
 * Test whether an axis-aligned bounding box intersects a bounding sphere.
 * @param Aabb Axis-aligned bounding box to test.
 * @param Sphere Bounding sphere to test.
 * @param OutContactInfo If non-null and an overlap is detected, populated with contact information describing penetration (normal, depth, and contact point) for the intersection.
 * @returns `true` if the AABB and sphere overlap, `false` otherwise.
 */

/**
 * Test whether an axis-aligned bounding box intersects an oriented bounding box.
 * @param Aabb Axis-aligned bounding box to test.
 * @param Obb Oriented bounding box to test.
 * @param OutContactInfo If non-null and an overlap is detected, populated with contact information describing penetration (normal, depth, and contact point) for the intersection.
 * @returns `true` if the AABB and OBB overlap, `false` otherwise.
 */

/**
 * Test whether an oriented bounding box intersects a bounding sphere.
 * @param Obb Oriented bounding box to test.
 * @param Sphere Bounding sphere to test.
 * @param OutContactInfo If non-null and an overlap is detected, populated with contact information describing penetration (normal, depth, and contact point) for the intersection.
 * @returns `true` if the OBB and sphere overlap, `false` otherwise.
 */

/**
 * Test whether two oriented bounding boxes intersect.
 * @param A First oriented bounding box to test.
 * @param B Second oriented bounding box to test.
 * @param OutContactInfo If non-null and an overlap is detected, populated with contact information describing penetration (normal, depth, and contact point) for the intersection.
 * @returns `true` if the two OBBs overlap, `false` otherwise.
 */

/**
 * Test whether an oriented bounding box intersects a capsule.
 * @param Obb Oriented bounding box to test.
 * @param Capsule Capsule to test.
 * @param OutContactInfo If non-null and an overlap is detected, populated with contact information describing penetration (normal, depth, and contact point) for the intersection.
 * @returns `true` if the OBB and capsule overlap, `false` otherwise.
 */

/**
 * Test whether a capsule intersects a bounding sphere.
 * @param Capsule Capsule to test.
 * @param Sphere Bounding sphere to test.
 * @param OutContactInfo If non-null and an overlap is detected, populated with contact information describing penetration (normal, depth, and contact point) for the intersection.
 * @returns `true` if the capsule and sphere overlap, `false` otherwise.
 */

/**
 * Test whether two capsules intersect.
 * @param A First capsule to test.
 * @param B Second capsule to test.
 * @param OutContactInfo If non-null and an overlap is detected, populated with contact information describing penetration (normal, depth, and contact point) for the intersection.
 * @returns `true` if the two capsules overlap, `false` otherwise.
 */

/**
 * Test whether two bounding spheres intersect.
 * @param A First bounding sphere to test.
 * @param B Second bounding sphere to test.
 * @param OutContactInfo If non-null and an overlap is detected, populated with contact information describing penetration (normal, depth, and contact point) for the intersection.
 * @returns `true` if the two spheres overlap, `false` otherwise.
 */
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