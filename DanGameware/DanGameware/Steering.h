#pragma once
#include "khmath.h"
#include "Gameplay.h"


class MovingEntity : GameObject
{
protected:
	Vector3df m_velocity;
	Vector3df m_heading;
	Vector3df m_side;
	f32 m_mass;
	f32 m_maxSpeed;
	f32 dMaxForce;
	f32 dMaxTurnRate;
public:

};

class Vehicle : MovingEntity
{
public:

};

