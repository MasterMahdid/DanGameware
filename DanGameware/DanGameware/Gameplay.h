#pragma once
#include "PhysicsEngine.h"
#include <Horde3D.h>
#include "khmath.h"
#define H3D_DEG2RAD  0.017453292f

const u32 PAWN_FLAG_ENEMY = 1;
const u32 PAWN_FLAG_PLAYER = 2;

struct GLFWwindow;
struct playerInput
{
	float dx, dy;//move x,y
	float drx, dry, drz;//rotate to look
	bool jumpHeld,jumpPressed,jumpReleased;
	bool runHeld,runPressed,runReleased;
	bool attack;
	void capture(GLFWwindow* _winHandle);
};


class GameObject
{
private:
	f32 x, y, z, pan, tilt, roll;
	
public:
	virtual void PhysicUpdate(float dt){};//before physics
	virtual void Update(float dt) {};//before render
	virtual void Think() {};
	virtual void Touch() {};
};

class FPSCharacter : public GameObject
{
private:
	Hndl pawn;
	H3DNode camera;

public:
	FPSCharacter(H3DNode cam);
	~FPSCharacter();
	virtual void PhysicUpdate(float dt)override;
	virtual void Update(float dt)override;
	void onProjectileHit(Vector3df hit_pos);
};

class WeaponAxe :public GameObject
{
private:
	float animTime;
	H3DNode modelNode;
public:
	WeaponAxe();
	static void initRes();
	//~WeaponAxe();
	//virtual void PhysicUpdate(float dt)override;
	virtual void Update(float dt)override;
	void setAnimSpeed(float speed);
	void attack();
	void jump();
	void land();
	void death();
};
class ProjectileAxe :public GameObject
{
private:
	H3DNode projectile;
	Vector3df shoot_direction;
	Hndl phys_handle;
public:
	ProjectileAxe();
	//~ProjectileAxe();
	void shoot(Vector3df start_pos, Vector3df direction, Hndl ignorePawn);
	virtual void PhysicUpdate(float dt)override;
	virtual void Update(float dt)override;
};
extern WeaponAxe* g_weapon_axe;



class flyThroughCam : public GameObject
{
private:
	H3DNode _cam;
public:
	flyThroughCam(H3DNode cam) :_cam(cam) {};
	virtual void Update(float dt)override;
};

extern physicEngine g_phyis;
extern playerInput g_input;


class GameplayContext
{
public:
	physicEngine* g_phys;
	playerInput* g_input;
	
};

extern std::vector<GameObject*> gameObjects_array;
void addGameObject(GameObject* go);
void commitGameObjectListChanges();

float SmoothDamp(float current, float target, float& currentVelocity, float smoothTime, float maxSpeed, float deltaTime);
