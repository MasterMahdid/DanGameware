#include "Gameplay.h"
#include "PhysicsEngine.h"
#include "khmath.h"
#include "enemy.h"
#include "ai.h"
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
	auto tr = g_phyis.getTransform(pawn);
	auto y = tr.y-25;
	float sx;
	h3dGetNodeTransform(node, NULL, NULL, NULL,NULL, NULL, NULL, &sx, NULL, NULL);
	
	float PI = std::atan(1) * 4;
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

