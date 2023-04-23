#include <Horde3D.h>
#include "Gameplay.h"
#include "Tween.h"
#include "khmath.h"
#include "SoundEngine.h"
#include "ai.h"
#include "utMath.h"
#include "debug_draw.h"

extern H3DNode main_camera;
WeaponHandgun* g_weapon_handgun;
extern std::vector<GameObject*> gameObjects_array;
namespace WeaponHandgun_internal
{
	H3DRes hg,hganim;
	float animspeed = 5;
	Tween tweener;
	bool attacking = false;
	bool allow_attack = true;
	Hndl ignorePawn;
}
using namespace WeaponHandgun_internal;
void WeaponHandgun::initRes()
{
	hg = h3dAddResource(H3DResTypes::SceneGraph, "models/weapons/handgun/FN57out.scene.xml", 0);
	hganim = h3dAddResource(H3DResTypes::Animation, "models/weapons/handgun/FN57out.anim", 0);

}
WeaponHandgun::WeaponHandgun()
{
	this->modelNode = h3dAddNodes(H3DRootNode, hg);
	h3dSetNodeTransform(this->modelNode, 0, 0, 0, 0, 0, 0, 0.5, 0.5, 0.5);
	h3dSetupModelAnimStage(this->modelNode, 0, hganim, 0, "", false);
	this->animTime = 0;
}


void WeaponHandgun::Update(float dt)
{
	{
		int i = 0;
		while (1)
		{
			float x1, x2, y1, y2, z1, z2;
			auto n = h3dGetNodeChild(this->modelNode, i++);
			if (n == 0)
				break;
			h3dGetNodeAABB(n, &x1, &y1, &z1, &x2, &y2, &z2);
			dd_aabb(Vector3df(x1, y1, z1), Vector3df(x2, y2, z2), DD_RED);
		}
		

	}
	if (attacking == false)
	{
		this->animTime += animspeed*dt;
		h3dSetModelAnimParams(this->modelNode, 0, this->animTime, 1);
	}
	else
	{
		h3dSetModelAnimParams(this->modelNode, 1, this->animTime, 1);

		if (animTime >= 7)
		{
			this->animTime += 60 * dt;
		}
		else
		{
			this->animTime += 35 * dt;
		}
		if (animTime >= 16)
		{
			auto projectile = new ProjectileAxe();
			addGameObject(projectile);
			float px, py, pz, dx, dy;
			h3dGetNodeTransform(main_camera, &px, &py, &pz, &dx, &dy, nullptr, nullptr, nullptr, nullptr);

			float* mat = new float[16];
			h3dGetNodeTransMats(main_camera, NULL, (const float**)&mat);
			Horde3D::Matrix4f mt2(mat);
			Horde3D::Vec3f xa(0, -10, 0);
			Horde3D::Vec3f xa2 = mt2*xa;

			float dir_rad_y = dy*H3D_DEG2RAD;
			float dir_rad_x = dx*H3D_DEG2RAD;
			auto move_dir = Vector3df(-sinf(dir_rad_y), sinf(dir_rad_x), -cosf(dir_rad_y));
			projectile->shoot(Vector3df(xa2.x, xa2.y, xa2.z), move_dir, ignorePawn);

			h3dSetModelAnimParams(this->modelNode, 1, this->animTime, 0);

			attacking = false;
			h3dSetNodeTransform(this->modelNode, 20, 0, 0, -20, 0, 0, 1, 1, 1);
			tweener.callFuncPeriodic(20, 0, [&](float f)
			{
				h3dSetNodeTransform(this->modelNode, f, 0, 0, -f, 0, 0, 1, 1, 1);
			}, 0.3, EASING_FUNCTION::CircularEaseOut, 213, 0.25);
			tweener.delayCall(0.4, [&]() {allow_attack = true; });
		}
	}

	h3dUpdateModel(this->modelNode, H3DModelUpdateFlags::Animation | H3DModelUpdateFlags::Geometry);
	tweener.update(dt);
}

void WeaponHandgun::setAnimSpeed(float speed)
{
	tweener.callFuncPeriodic(animspeed, speed, [&](float f) {animspeed = f; }, 0.2f, EASING_FUNCTION::Linear);
}
void WeaponHandgun::attack(Hndl ignorePawn)
{
	if (!allow_attack)
		return;
	attacking = true;
	allow_attack = false;
	animTime = 0;
	ignorePawn = ignorePawn;
	fpsCharacter->shootCamAnim();
}
void WeaponHandgun::jump()
{
	tweener.callFuncPeriodic(0, 1, [&](float f)
	{
		h3dSetNodeTransform(this->modelNode, 0, -f * 1, 0, -f * 7, 0, 0, 1, 1, 1);
	}, 0.5, EASING_FUNCTION::CubicEaseOut, 2221);
	tweener.callFuncPeriodic(1, 0, [&](float f)
	{
		h3dSetNodeTransform(this->modelNode, 0, -f * 1, 0, -f * 7, 0, 0, 1, 1, 1);
	}, 0.5, EASING_FUNCTION::CubicEaseOut, 0, 0.3);
}
void WeaponHandgun::land()
{
	//tweener.removeByTag(2221);

}
void WeaponHandgun::death()
{
	//tweener.removeByTag(2221);
	h3dRemoveNode(this->modelNode);

}
