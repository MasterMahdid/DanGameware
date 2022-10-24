#include <Horde3D.h>
#include "Gameplay.h"
#include "Tween.h"
#include "khmath.h"
#include "SoundEngine.h"
#include "ai.h"
#include "utMath.h"
#include "debug_draw.h"

extern H3DNode main_camera;
WeaponAxe* g_weapon_axe;
extern std::vector<GameObject*> gameObjects_array;
namespace WeaponAxe_internal
{
	H3DRes axe, axe_pr, axanim,attackanim;
	float animspeed = 10;
	Tween tweener;
	bool attacking = false;
	bool allow_attack = true;
	Hndl axeignorePawn;
}
using namespace WeaponAxe_internal;
void WeaponAxe::initRes()
{
	axe = h3dAddResource(H3DResTypes::SceneGraph, "models/axe/axe_cam.scene.xml", 0);
	axe_pr = h3dAddResource(H3DResTypes::SceneGraph, "models/axe/axe_projectile.scene.xml", 0);
	axanim = h3dAddResource(H3DResTypes::Animation, "models/axe/axe_cam.anim", 0);
	attackanim = h3dAddResource(H3DResTypes::Animation, "models/axe/axe_cam_attack.anim", 0);

}
WeaponAxe::WeaponAxe()
{
	this->modelNode = h3dAddNodes(main_camera, axe);
	h3dSetNodeTransform(this->modelNode, 0, 0, 0, 0, 0, 0, 1, 1, 1);
	h3dSetupModelAnimStage(this->modelNode, 0, axanim, 0, "", false);
	h3dSetupModelAnimStage(this->modelNode, 1, attackanim, 0, "", false);

	this->animTime = 0;
}


void WeaponAxe::Update(float dt)
{
	if (attacking == false)
	{
		this->animTime += animspeed*dt;
		h3dSetModelAnimParams(this->modelNode, 0, this->animTime, 1);
	}
	else
	{
		h3dSetModelAnimParams(this->modelNode, 1, this->animTime, 1);
		this->animTime += 60 * dt;
		if (animTime >= 3)
		{
			auto projectile = new ProjectileAxe();
			addGameObject(projectile);
			float px, py, pz,dx,dy;
			h3dGetNodeTransform(main_camera, &px, &py, &pz, &dx, &dy, nullptr, nullptr, nullptr,nullptr);
			
			float* mat = new float[16];
			h3dGetNodeTransMats(main_camera, NULL, (const float**)&mat);
			Horde3D::Matrix4f mt2(mat);
			Horde3D::Vec3f xa(15, -10, 0);
			Horde3D::Vec3f xa2 = mt2*xa;

			float dir_rad_y = dy*H3D_DEG2RAD;
			float dir_rad_x = dx*H3D_DEG2RAD;
			auto move_dir = Vector3df(-sinf(dir_rad_y), sinf(dir_rad_x), -cosf(dir_rad_y));
			projectile->shoot(Vector3df(xa2.x, xa2.y, xa2.z), move_dir, axeignorePawn);
		

			h3dSetModelAnimParams(this->modelNode, 1, this->animTime, 0);
			
			attacking = false;
			h3dSetNodeTransform(this->modelNode, 20,0 , 0, -20, 0, 0, 1, 1, 1);
			tweener.callFuncPeriodic(20, 0, [&](float f)
			{
				h3dSetNodeTransform(this->modelNode, f, 0, 0, -f, 0, 0, 1, 1, 1);
			}, 0.3, EASING_FUNCTION::CircularEaseOut,213,0.25);
			tweener.delayCall(0.2, [&]() {allow_attack = true; });
		}
	}
	
	h3dUpdateModel(this->modelNode, H3DModelUpdateFlags::Animation | H3DModelUpdateFlags::Geometry);
	tweener.update(dt);
}

void WeaponAxe::setAnimSpeed(float speed)
{
	tweener.callFuncPeriodic(animspeed, speed, [&](float f) {animspeed = f; }, 0.2f, EASING_FUNCTION::Linear);
}
void WeaponAxe::attack(Hndl ignorePawn)
{
	if (!allow_attack)
		return;
	attacking = true;
	allow_attack = false;
	animTime = 0;
	axeignorePawn = ignorePawn;
}
void WeaponAxe::jump()
{
	tweener.callFuncPeriodic(0, 1, [&](float f)
	{
		h3dSetNodeTransform(this->modelNode, 0, -f * 1, 0, -f * 7, 0, 0, 1, 1, 1);
	}, 0.5, EASING_FUNCTION::CubicEaseOut,2221);
	tweener.callFuncPeriodic(1, 0, [&](float f)
	{
		h3dSetNodeTransform(this->modelNode, 0, -f * 1, 0, -f * 7, 0, 0, 1, 1, 1);
	}, 0.5, EASING_FUNCTION::CubicEaseOut, 0, 0.3);
}
void WeaponAxe::land()
{
	//tweener.removeByTag(2221);
	
}
void WeaponAxe::death()
{
	//tweener.removeByTag(2221);
	h3dRemoveNode(this->modelNode);

}
ProjectileAxe::ProjectileAxe()
{
	projectile = h3dAddNodes(H3DRootNode, axe_pr);
	
}

void ProjectileAxe::shoot(Vector3df start_pos, Vector3df direction,Hndl ignorePawn)
{
	float ddr = -atan2f(direction.z, direction.x) / H3D_DEG2RAD;
	h3dSetNodeTransform(projectile, start_pos.x, start_pos.y, start_pos.z, 0, ddr, 0, 1, 1, 1);
	shoot_direction = direction * 1200;
	float pp[3]{ start_pos.x,start_pos.y,start_pos.z };
	phys_handle = g_phyis.createSphereProjectile(7, pp, [&](const contanctInfo& ci)
	{
		h3dRemoveNode(projectile);
		projectile = 0;

		if (ci.isOtherPawn == false)
		{
			float cam_x, cam_y, cam_z;
			h3dGetNodeTransform(main_camera, &cam_x, &cam_y, &cam_z, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
			auto len = (ci.contactPoint - Vector3df(cam_x, cam_y, cam_z)).getLength();
			float vol = 100.f / len;
			vol = clamp(vol, 0, 1);
			khsound::play_sound(khsound::SOUND_IMPACT_1, vol);
		}
	}, ignorePawn);
	khsound::play_sound(khsound::SOUND_WHOOSH);
}
void ProjectileAxe::PhysicUpdate(float dt)
{

}
void ProjectileAxe::Update(float dt)
{
	if (projectile == 0)
		return;
	float x, y, z, rz;
	h3dGetNodeTransform(projectile, &x, &y, &z, nullptr, nullptr, &rz, nullptr, nullptr, nullptr);
	rz += dt * -1900;//rotational speed

	shoot_direction.y -= 800 * dt;//gravity;
	
	float ddr = -atan2f(shoot_direction.z, shoot_direction.x)/ H3D_DEG2RAD;
	auto move_dir = shoot_direction*dt;
	
	x += move_dir.x;
	y += move_dir.y;
	z += move_dir.z;
	
	h3dSetNodeTransform(projectile, x, y, z, 0, ddr, rz, 1, 1, 1);
	//dd_sphere(Vector3df(x,y,z), 10, Vector3df(0, 0, 1));
	g_phyis.setProjectilePosition(phys_handle, Vector3df(x, y, z));

}