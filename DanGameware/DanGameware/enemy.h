#pragma once;
#include "Gameplay.h"
#include "fsm.h"

class Enemy : public GameObject
{
private:
	Hndl pawn;
	H3DNode node;
	Vector3d<f32> velocity;
	float animTime;
	float walkAnimTime;
	float deathAnimTime;
	float idle_walk_fade;
	bool walking;
	bool alive;
public:
	Enemy();
	~Enemy();
	static void add_res();
	virtual void PhysicUpdate(float dt)override;
	virtual void Update(float dt)override;
	void onProjectileHit(Vector3df hit_pos);
};
class EnemyAnimationController
{

};

class FollowStaet : public State
{
	virtual void execute(float dt)override;
};