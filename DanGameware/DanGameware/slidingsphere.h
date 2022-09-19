#pragma once;
#include "khmath.h"

namespace braynzar
{
	struct CollisionPacket {
		// Information about ellipsoid (in world space)
		Vector3df ellipsoidSpace;
		Vector3df w_Position;
		Vector3df w_Velocity;

		// Information about ellipsoid (in ellipsoid space)
		Vector3df e_Position;
		Vector3df e_Velocity;
		Vector3df e_normalizedVelocity;

		// Collision Information
		bool foundCollision;
		float nearestDistance;
		Vector3df intersectionPoint;
		int collisionRecursionDepth;

		bool grounded;
	};
	bool SphereCollidingWithTriangle(CollisionPacket& cP,    // Pointer to a CollisionPacket object    
		Vector3df &p0,                                        // First vertex position of triangle
		Vector3df &p1,                                        // Second vertex position of triangle
		Vector3df &p2,                                        // Third vertex position of triangle 
		Vector3df &triNormal);                                // Triangle's Normal
	Vector3df CollisionSlide(CollisionPacket& cP, Vector3df falling_vel,bool& out_falling);
}