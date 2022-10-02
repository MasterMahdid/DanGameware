#pragma once
#include <vector>
#include <functional>
#include "khmath.h"
#include "collision.h"
typedef int Hndl;
typedef int H3DRes;
struct physObjectTransform
{
	float x, y, z, rx, ry, rz, vx, vy, vz;
	bool grounded;
};

struct Pawn;
struct SphereProjectile;
class physicEngine
{
private:
	std::vector<Pawn*> pawns;
	std::vector<SphereProjectile*> sphereProjectiles;
public:
	void loadLevel(float* tri_data,size_t tri_count);
	void update(float dt);
	Hndl createPawn(float radius, float height, float *pos,void* userdata=nullptr, u32 flags=0);
	Hndl createSphereProjectile(float radius, float *pos,std::function<void(const contanctInfo&)> callback,Hndl ignore_pawn);
	void setProjectilePosition(Hndl proj, Vector3df pos);
	void setVelocity(Hndl, float* velocity);
	void setPosition(Hndl, float* velocity);
	void pawnJump(Hndl, float velocity);
	float* createLevelPhysTriData(H3DRes level_mesh_res, int& tri_count);
	//void destroy(Hndl);
	physObjectTransform getTransform(Hndl);
	bool physicEngine::trace(Vector3df from, Vector3df to, Vector3df& hit);


};