#include "Gameplay.h"
#include "PhysicsEngine.h"
#include "khmath.h"
#include "enemy.h"
#include "ai.h"
#include "debug_draw.h"
#include "Tween.h"
void dw_console_log(const char* fmt, ...);
H3DRes enemy_res,enemy_anim_run_res, enemy_anim_idle_res, enemy_anim_death_res, enemy_anim_hit_res;
H3DRes blood_particle_res;
extern H3DNode main_camera;

void Enemy::add_res()
{
	//enemy_res = h3dAddResource(H3DResTypes::SceneGraph, "models/paladin/Paladin.scene.xml", 0);
	//enemy_anim_run_res = h3dAddResource(H3DResTypes::Animation, "models/paladin/Walking.anim",0);

	enemy_res = h3dAddResource(H3DResTypes::SceneGraph, "models/enemysoldier/enemysoldier.scene.xml", 0);
	enemy_anim_run_res = h3dAddResource(H3DResTypes::Animation, "models/enemysoldier/run_sword.anim", 0);
	enemy_anim_idle_res = h3dAddResource(H3DResTypes::Animation, "models/enemysoldier/idle.anim", 0);
	enemy_anim_death_res = h3dAddResource(H3DResTypes::Animation, "models/enemysoldier/death.anim", 0);
	enemy_anim_hit_res = h3dAddResource(H3DResTypes::Animation, "models/enemysoldier/react.anim", 0);
	blood_particle_res = h3dAddResource(H3DResTypes::SceneGraph, "particles/blood/blood.scene.xml", 0);

}
Enemy::Enemy(gentity_t _ent)
{
	ent = _ent;
	node = h3dAddNodes(H3DRootNode, enemy_res);
	h3dSetNodeTransform(node, ent->pos1.x , ent->pos1.y, ent->pos1.z, 0, ent->angle, 0, 2,2,2);
	float x, y, z;
	h3dGetNodeTransform(node, &x, &y, &z, NULL, NULL, NULL, NULL, NULL, NULL);
	float pos[3]{ x,y,z};
	pawn = g_phyis.createPawn(16,50, pos,(void*)this,PAWN_FLAG_ENEMY);
	ent->node = node;
	ent->pawn = pawn;
	init_enemy(ent);
	this->animTime = 0;
	this->hittime = 0;
	h3dSetupModelAnimStage(node, 0, enemy_anim_run_res, 0, "", false);
	h3dSetupModelAnimStage(node, 1, enemy_anim_idle_res, 0, "", false);
	h3dSetupModelAnimStage(node, 2, enemy_anim_death_res, 0, "", false);
	h3dSetupModelAnimStage(node, 3, enemy_anim_hit_res, 0, "", false);
	
	
	
	float ran = (rand()*1.0f) / RAND_MAX;
	animrandomofset = ran * 30;
	
	health = 100;
	alive = true;

	impulseVel = Vector3df(0,0,0);
}

Enemy::~Enemy()
{
	//g_phyis.destroy(pawn);
}

void Enemy::PhysicUpdate(float dt)
{
	auto vel = ent->movedir*ent->speed;
	float vv[3]{ vel.x+ impulseVel.x,vel.y+ impulseVel.y,vel.z+ impulseVel.z };
	g_phyis.setVelocity(pawn, vv);
	velocity = vel;
}
extern Tween g_tween;
void Enemy::Update(float dt)
{
	float* mat = new float[16];
	h3dGetNodeTransMats(node, NULL, (const float**)&mat);
	//dd_axes(mat, 0);

	float PI = std::atan(1) * 4;
	if (impulseVel.getLengthSQ() > 0.1)
	{
		auto n = impulseVel;
		n.normalize();
		impulseVel -= n*4500*dt;
	}
	else
	{
		impulseVel = Vector3df(0, 0, 0);

	}
	
	{
		float minx, miny, minz, maxx, maxy, maxz;
		h3dGetNodeAABB(node, &minx, &miny, &minz, &maxx, &maxy, &maxz);
		//dd_aabb(Vector3df(minx, miny, minz), Vector3df(maxx, maxy, maxz), Vector3df(1, 0, 1));
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
				//dd_sphere(hit, 5, Vector3df(1, 0, 0));
				//dd_line(hit, camv, Vector3df(1, 1, 0), 0);

			}
			else
			{
				//dd_line(to, camv, Vector3df(1, 1, 0), 0);
				//dd_point(to, Vector3df(1, 1, 0), 10);

			}
		}
		

	}


	auto tr = g_phyis.getTransform(pawn);
	auto y = tr.y-25;
	float sx;
	h3dGetNodeTransform(node, NULL, NULL, NULL,NULL, NULL, NULL, &sx, NULL, NULL);
	
	float len = velocity.x*velocity.x + velocity.y*velocity.y;
	bool moving = false;
	if (len > 0.001)
	{
		float ry = atan2(velocity.x, velocity.z) / PI * 180;
		ent->angle = ry;
		moving = true;
	}
	h3dSetNodeTransform(node, tr.x, y, tr.z, 0, ent->angle, 0, sx, sx, sx);
	float animspeed = 32;
	hittime -= dt;
	if (alive)
	{
		
		if (hittime > 0)
		{
			this->animTime += animspeed*dt*2;
			h3dSetModelAnimParams(node, 0, this->animTime + this->animrandomofset, moving ? 1 : 0);//run
			h3dSetModelAnimParams(node, 1, this->animTime + this->animrandomofset, moving ? 0 : 1);//idle
			h3dSetModelAnimParams(node, 2, this->animTime, 0);//death
			h3dSetModelAnimParams(node, 3, this->animTime, 5);//hit
		}
		else
		{
			this->animTime += animspeed*dt;
			h3dSetModelAnimParams(node, 0, this->animTime + this->animrandomofset, moving ? 1 : 0);//run
			h3dSetModelAnimParams(node, 1, this->animTime + this->animrandomofset, moving ? 0 : 1);//idle
			h3dSetModelAnimParams(node, 2, 0, 0);//death
			h3dSetModelAnimParams(node, 3, 0, 0);//hit
		}
		
	}
	else
	{
		if(this->animTime<60)
			this->animTime += animspeed*dt;

		h3dSetModelAnimParams(node, 0, 0, 0);//run
		h3dSetModelAnimParams(node, 1, 0, 0);//idle
		h3dSetModelAnimParams(node, 2, this->animTime,1);//death
		h3dSetModelAnimParams(node, 3, 0, 0);//idle
	}
	
	h3dUpdateModel(node, H3DModelUpdateFlags::Animation | H3DModelUpdateFlags::Geometry);

	
}
void Enemy::onProjectileHit(Vector3df hit_pos)
{
	if (!alive)
		return;
	dw_console_log("hit enemy");
	health -= 40;
	if (health < 0)
	{
		alive = false;
		ent->think = nullptr;
		this->animTime = 0;
		ent->speed = 0;
		g_tween.delayCall(0.6, [this](){
			g_phyis.removePawn(pawn);
		});
		

	
	}	
	else
	{
		hittime = 0.4;
		this->animTime = 0;
		
	}
	auto max_impulse = (this->ent->pos1 - player_ent->pos1).normalize() * 900;
	impulseVel = max_impulse;

	auto blood_particle = h3dAddNodes(node, blood_particle_res);
	h3dSetNodeTransform(blood_particle, 0, 20, 3, 0, 0, 0, 1, 1, 1);
	g_tween.delayCall(1, [=]() {
		h3dRemoveNode(blood_particle);
	});


	
}

