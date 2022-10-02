#pragma once
#include "khmath.h"

struct contanctInfo
{
	Vector3df contactPoint;
	Vector3df normal;
};

bool ColShpereTriangle(const Triangle& t, const Vector3df& shpere_center, float shpere_radius, contanctInfo& ci);
bool ColShpereCapsule(Vector3df sphere_center, f32 sphere_raidus, Vector3df capsule_a, Vector3df capsule_b, f32 capsule_radius,contanctInfo& ci);
bool colTriLine(Triangle* tri, Vector3df* from, Vector3df* to, Vector3df& out_res);
