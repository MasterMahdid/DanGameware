#pragma once;
#include "Gameplay.h"
#include "ai.h"
class Enemy : public GameObject
{
private:
	Hndl pawn;
	H3DNode node;
	Vector3d<f32> velocity;
	float animrandomofset;
	float animTime;
	float hittime;
	float walkAnimTime;
	float deathAnimTime;
	float idle_walk_fade;
	bool walking;
	bool alive;
	float health;
	gentity_t ent;
	Vector3df impulseVel;
public:
	Enemy(gentity_t _ent);
	~Enemy();
	static void add_res();
	virtual void PhysicUpdate(float dt)override;
	virtual void Update(float dt)override;
	void onProjectileHit(Vector3df hit_pos);
};
class EnemyAnimationController
{

};

