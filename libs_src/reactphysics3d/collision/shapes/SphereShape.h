/********************************************************************************
* ReactPhysics3D physics library, http://www.reactphysics3d.com                 *
* Copyright (c) 2010-2016 Daniel Chappuis                                       *
*********************************************************************************
*                                                                               *
* This software is provided 'as-is', without any express or implied warranty.   *
* In no event will the authors be held liable for any damages arising from the  *
* use of this software.                                                         *
*                                                                               *
* Permission is granted to anyone to use this software for any purpose,         *
* including commercial applications, and to alter it and redistribute it        *
* freely, subject to the following restrictions:                                *
*                                                                               *
* 1. The origin of this software must not be misrepresented; you must not claim *
*    that you wrote the original software. If you use this software in a        *
*    product, an acknowledgment in the product documentation would be           *
*    appreciated but is not required.                                           *
*                                                                               *
* 2. Altered source versions must be plainly marked as such, and must not be    *
*    misrepresented as being the original software.                             *
*                                                                               *
* 3. This notice may not be removed or altered from any source distribution.    *
*                                                                               *
********************************************************************************/

#ifndef REACTPHYSICS3D_SPHERE_SHAPE_H
#define REACTPHYSICS3D_SPHERE_SHAPE_H

// Libraries
#include <reactphysics3d/body/CollisionBody.h>
#include <reactphysics3d/collision/shapes/ConvexShape.h>
#include <reactphysics3d/mathematics/mathematics.h>

// ReactPhysics3D namespace
namespace reactphysics3d {

// Class SphereShape
/**
 * This class represents a sphere collision shape that is centered
 * at the origin and defined by its radius. This collision shape does not
 * have an explicit object margin distance. The margin is implicitly the
 * radius of the sphere. Therefore, no need to specify an object margin
 * for a sphere shape.
 */
class SphereShape : public ConvexShape {

    protected :

        // -------------------- Attributes -------------------- //


        // -------------------- Methods -------------------- //

        /// Private copy-constructor
        SphereShape(const SphereShape& shape);

        /// Private assignment operator
        SphereShape& operator=(const SphereShape& shape);

        /// Return a local support point in a given direction without the object margin
        virtual Vector3 getLocalSupportPointWithoutMargin(const Vector3& direction,
                                                          void** cachedCollisionData) const;

        /// Return true if a point is inside the collision shape
        virtual bool testPointInside(const Vector3& localPoint, ProxyShape* proxyShape) const;

        /// Raycast method with feedback information
        virtual bool raycast(const Ray& ray, RaycastInfo& raycastInfo, ProxyShape* proxyShape) const;

        /// Return the number of bytes used by the collision shape
        virtual size_t getSizeInBytes() const;

    public :

        // -------------------- Methods -------------------- //

        /// Constructor
        SphereShape(decimal radius);

        /// Destructor
        virtual ~SphereShape();

        /// Return the radius of the sphere
        decimal getRadius() const;

        /// Set the scaling vector of the collision shape
        virtual void setLocalScaling(const Vector3& scaling);

        /// Return the local bounds of the shape in x, y and z directions.
        virtual void getLocalBounds(Vector3& min, Vector3& max) const;

        /// Return the local inertia tensor of the collision shape
        virtual void computeLocalInertiaTensor(Matrix3x3& tensor, decimal mass) const;

        /// Update the AABB of a body using its collision shape
        virtual void computeAABB(AABB& aabb, const Transform& transform) const;
};

// Get the radius of the sphere
/**
 * @return Radius of the sphere (in meters)
 */
inline decimal SphereShape::getRadius() const {
    return mMargin;
}

// Set the scaling vector of the collision shape
inline void SphereShape::setLocalScaling(const Vector3& scaling) {

    mMargin = (mMargin / mScaling.x) * scaling.x;

    CollisionShape::setLocalScaling(scaling);
}

// Return the number of bytes used by the collision shape
inline size_t SphereShape::getSizeInBytes() const {
    return sizeof(SphereShape);
}

// Return a local support point in a given direction without the object margin
inline Vector3 SphereShape::getLocalSupportPointWithoutMargin(const Vector3& direction,
                                                              void** cachedCollisionData) const {

    // Return the center of the sphere (the radius is taken into account in the object margin)
    return Vector3(0.0, 0.0, 0.0);
}

// Return the local bounds of the shape in x, y and z directions.
// This method is used to compute the AABB of the box
/**
 * @param min The minimum bounds of the shape in local-space coordinates
 * @param max The maximum bounds of the shape in local-space coordinates
 */
inline void SphereShape::getLocalBounds(Vector3& min, Vector3& max) const {

    // Maximum bounds
    max.x = mMargin;
    max.y = mMargin;
    max.z = mMargin;

    // Minimum bounds
    min.x = -mMargin;
    min.y = min.x;
    min.z = min.x;
}

// Return the local inertia tensor of the sphere
/**
 * @param[out] tensor The 3x3 inertia tensor matrix of the shape in local-space
 *                    coordinates
 * @param mass Mass to use to compute the inertia tensor of the collision shape
 */
inline void SphereShape::computeLocalInertiaTensor(Matrix3x3& tensor, decimal mass) const {
    decimal diag = decimal(0.4) * mass * mMargin * mMargin;
    tensor.setAllValues(diag, 0.0, 0.0,
                        0.0, diag, 0.0,
                        0.0, 0.0, diag);
}

// Update the AABB of a body using its collision shape
/**
 * @param[out] aabb The axis-aligned bounding box (AABB) of the collision shape
 *                  computed in world-space coordinates
 * @param transform Transform used to compute the AABB of the collision shape
 */
inline void SphereShape::computeAABB(AABB& aabb, const Transform& transform) const {

    // Get the local extents in x,y and z direction
    Vector3 extents(mMargin, mMargin, mMargin);

    // Update the AABB with the new minimum and maximum coordinates
    aabb.setMin(transform.getPosition() - extents);
    aabb.setMax(transform.getPosition() + extents);
}

// Return true if a point is inside the collision shape
inline bool SphereShape::testPointInside(const Vector3& localPoint, ProxyShape* proxyShape) const {
    return (localPoint.lengthSquare() < mMargin * mMargin);
}

}

#endif
