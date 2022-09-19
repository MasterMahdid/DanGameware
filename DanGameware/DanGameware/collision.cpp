#include "collision.h"

float SqDistPointSegment(Vector3df a, Vector3df b, Vector3df c)
{
	Vector3df ab = b - a;
	Vector3df ac = c - a;
	Vector3df bc = c -b;
	float e = ac.dot(ab);
	// Handle cases where c projects outside ab
	if (e <= 0.0f) return ac.dot(ac);
	float f = ab.dot(ab);
	if (e >= f) return bc.dot(bc);
	// Handle cases where c projects onto ab
	return ac.dot(ac) - e*e / f;
}

Vector3df ClosestPtPointTriangle(Vector3df p, Vector3df a, Vector3df b, Vector3df c)
{
	Vector3df ab = b - a;
	Vector3df ac = c - a;
	Vector3df bc = c - b;
	// Compute parametric position s for projection P’ of P on AB,
	// P’ = A + s*AB, s = snom/(snom+sdenom)
	float snom = (p - a).dot(ab), sdenom = (p - b).dot(a - b);
	// Compute parametric position t for projection P’ of P on AC,
	// P’ = A + t*AC, s = tnom/(tnom+tdenom)
	float tnom = (p - a).dot(ac), tdenom = (p - c).dot(a - c);
	if (snom <= 0.0f && tnom <= 0.0f) return a; // Vertex region early out
												// Compute parametric position u for projection P’ of P on BC,
												// P’ = B + u*BC, u = unom/(unom+udenom)
	float unom = (p - b).dot(bc), udenom = (p - c).dot(b - c);
	if (sdenom <= 0.0f && unom <= 0.0f) return b; // Vertex region early out
	if (tdenom <= 0.0f && udenom <= 0.0f) return c; // Vertex region early out
													// P is outside (or on) AB if the triple scalar product [N PA PB] <= 0
	Vector3df n = (b - a).cross(c - a);
	float vc =n.dot((a - p).cross(b - p));
	// If P outside AB and within feature region of AB,
	// return projection of P onto AB
	if (vc <= 0.0f && snom >= 0.0f && sdenom >= 0.0f)
		return a + ab*(snom / (snom + sdenom));
	// P is outside (or on) BC if the triple scalar product [N PB PC] <= 0
	float va = n.dot((b - p).cross(c - p));
	// If P outside BC and within feature region of BC,
	// return projection of P onto BC
	if (va <= 0.0f && unom >= 0.0f && udenom >= 0.0f)
		return b + bc*(unom / (unom + udenom));
	// P is outside (or on) CA if the triple scalar product [N PC PA] <= 0
	float vb = n.dot((c - p).cross(a - p));
	// If P outside CA and within feature region of CA,
	// return projection of P onto CA
	if (vb <= 0.0f && tnom >= 0.0f && tdenom >= 0.0f)
		return a + ac*(tnom / (tnom + tdenom));
	// P must project inside face region. Compute Q using barycentric coordinates
	float u = va / (va + vb + vc);
	float v = vb / (va + vb + vc);
	float w = 1.0f - u - v; // = vc / (va + vb + vc)
	return a*u + b*v + c*w;
}
bool ColShpereTriangle(const Triangle& t, const Vector3df& shpere_center, float shpere_radius, contanctInfo& ci)
{
	auto p = ClosestPtPointTriangle(shpere_center, t.p1, t.p2, t.p3);
	auto dist = (p - shpere_center).getLength();
	if (dist <= shpere_radius)
	{
		ci.contactPoint = p;
		ci.normal = (t.p2 - t.p1).cross(t.p3 - t.p1).normalize();
		return true;
	}
	return false;
}
bool ColShpereCapsule(Vector3df sphere_center, f32 sphere_raidus, Vector3df capsule_a, Vector3df capsule_b, f32 capsule_radius, contanctInfo& ci)
{
	// Compute (squared) distance between sphere center and capsule line segment
	float dist2 = SqDistPointSegment(capsule_a, capsule_b, sphere_center);
	// If (squared) distance smaller than (squared) sum of radii, they collide
	float radius = sphere_raidus + capsule_radius;
	bool col  = dist2 <= radius * radius;
	if (col)
	{
		ci.contactPoint = sphere_center;
		ci.normal = Vector3df(1, 1, 1);
	}
	return col;
}