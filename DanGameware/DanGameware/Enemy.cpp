#include "Gameplay.h"
#include "PhysicsEngine.h"
#include "khmath.h"
#include "enemy.h"
#include "ai.h"
#include "debug_draw.h"
H3DRes enemy_res,enemy_anim_run_res;
extern H3DNode main_camera;
extern gentity_t g_entities;

void Enemy::add_res()
{
	enemy_res = h3dAddResource(H3DResTypes::SceneGraph, "models/paladin/Paladin.scene.xml", 0);
	enemy_anim_run_res = h3dAddResource(H3DResTypes::Animation, "models/paladin/Walking.anim",0);
}
Enemy::Enemy()
{
	node = h3dAddNodes(H3DRootNode, enemy_res);
	h3dSetNodeTransform(node, 250, -120, 800,0,0,0,0.38,0.38,0.38);
	float x, y, z;
	h3dGetNodeTransform(node, &x, &y, &z, NULL, NULL, NULL, NULL, NULL, NULL);
	float pos[3]{ x,y,z};
	pawn = g_phyis.createPawn(16,50, pos,(void*)this,PAWN_FLAG_ENEMY);
	g_entities[1].node = node;
	g_entities[1].pawn = pawn;
	init_enemy(&g_entities[1]);
	this->animTime = 0;
	h3dSetupModelAnimStage(node, 0, enemy_anim_run_res, 0, "", false);
}

Enemy::~Enemy()
{
	//g_phyis.destroy(pawn);
}
void Enemy::PhysicUpdate(float dt)
{
	auto vel = g_entities[1].movedir*g_entities[1].speed;
	float vv[3]{ vel.x,vel.y,vel.z };
	g_phyis.setVelocity(pawn, vv);
	velocity = vel;
}

void Enemy::Update(float dt)
{
	float* mat = new float[16];
	h3dGetNodeTransMats(node, NULL, (const float**)&mat);
	dd_axes(mat, 0);

	float PI = std::atan(1) * 4;


	{
		float minx, miny, minz, maxx, maxy, maxz;
		h3dGetNodeAABB(node, &minx, &miny, &minz, &maxx, &maxy, &maxz);
		dd_aabb(Vector3df(minx, miny, minz), Vector3df(maxx, maxy, maxz), Vector3df(1, 0, 1));
	}
	{
		float x, y, z,ryy;
		h3dGetNodeTransform(node, &x, &y, &z, nullptr, &ryy, nullptr, nullptr, nullptr, nullptr);
		float rys[2] = { ryy + 90,ryy-90 };
		for(float ry : rys)
		{
			float rad = ry / 180.0f * PI;
			Vector3df camv(x, y + 32, z);
			auto to = camv + Vector3df(sinf(rad) * 200, 0, cosf(rad) * 200);

			Vector3df hit;
			bool res = g_phyis.trace(camv, to, hit);
			if (res)
			{
				dd_sphere(hit, 5, Vector3df(1, 0, 0));
				dd_line(hit, camv, Vector3df(1, 1, 0), 0);

			}
			else
			{
				dd_line(to, camv, Vector3df(1, 1, 0), 0);
				dd_point(to, Vector3df(1, 1, 0), 10);

			}
		}
		

	}


	auto tr = g_phyis.getTransform(pawn);
	auto y = tr.y-25;
	float sx;
	h3dGetNodeTransform(node, NULL, NULL, NULL,NULL, NULL, NULL, &sx, NULL, NULL);
	if (iszero(velocity.getLength()))
		return;
	
	float ry = atan2(velocity.x,velocity.z)/ PI*180;
	h3dSetNodeTransform(node, tr.x, y, tr.z, 0, ry, 0, sx, sx, sx);
	float animspeed = .4* this->velocity.getLength();
	this->animTime += animspeed*dt;
	h3dSetModelAnimParams(node, 0, this->animTime, 1);
	h3dUpdateModel(node, H3DModelUpdateFlags::Animation | H3DModelUpdateFlags::Geometry);

	
}
void Enemy::onProjectileHit(Vector3df hit_pos)
{
	
	
}

