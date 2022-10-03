#include "Gameplay.h"
#include "Tween.h"
#include "khmath.h"
#include "SoundEngine.h"
#include "ai.h"
#include "debug_draw.h"
extern gentity_t g_entities;
namespace fpscharacter_internal
{
	Tween tween;
	Tween walk_tween;
	float vely = 0;
	float move_speed = 100;
	float fac;
	bool allow_jump = true;
	bool last_grounded = false;
	float cam_y_ofset;
	float cam_y_ofset2;
	float cam_rz = 0;
	bool grounded_anim = true;
	float falling_time = 0;
	float current_moving_speed = 0;
	float curr_y = 0;
	bool first_update = true;
}
using namespace fpscharacter_internal;
FPSCharacter::FPSCharacter(H3DNode cam)
{
	camera = cam;
	float cx, cy, cz;
	h3dGetNodeTransform(cam, &cx, &cy, &cz, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
	float pos[3]{ cx,cy,cz};
	pawn = g_phyis.createPawn(16, 56, pos,(void*)this,PAWN_FLAG_PLAYER);
	walk_tween.callFuncPeriodic(2, 0, [](float x) {cam_y_ofset2 = x; }, 0.5, EASING_FUNCTION::SineEaseOut, 26,0,0,true);
	walk_tween.callFuncPeriodic(-0.4, +0.4, [](float x) {cam_rz = x; }, 0.4, EASING_FUNCTION::SineEaseOut, 26, 0, 0, true);
	g_entities[0].pawn = pawn;

	auto light = h3dAddLightNode(cam, "Light1", 0, "LIGHTING", "SHADOWMAP");
	h3dSetNodeTransform(light, 0, 0, 0, 0, 0, 0, 1, 1, 1);
	h3dSetNodeParamF(light, H3DLight::FovF, 0, 360);
	h3dSetNodeParamF(light, H3DLight::RadiusF, 0, 300);
	h3dSetNodeParamF(light, H3DLight::ColorMultiplierF, 0,3);
	h3dSetNodeParamI(light, H3DLight::ShadowMapCountI, 0);
	h3dSetNodeParamF(light, H3DLight::ShadowMapBiasF, 0, 0.003f);
	h3dSetNodeParamF(light, H3DLight::ColorF3, 0, 1);
	h3dSetNodeParamF(light, H3DLight::ColorF3, 1, 0.75);
	h3dSetNodeParamF(light, H3DLight::ColorF3, 2,0.50);
}
FPSCharacter::~FPSCharacter()
{
	//g_phyis.destroy(pawn);
}
float walk_time = 0;
int foot_sound = 0;
float damp_speed_x = 0;
float damp_speed_x_vel = 0;
float damp_speed_y = 0;
float damp_speed_y_vel = 0;
/*extern*/ float animTime;
void FPSCharacter::PhysicUpdate(float dt)
{
	float rx, ry, rz, t;
	h3dGetNodeTransform(camera, &t, &t, &t, &rx, &ry, &rz, &t, &t, &t);

	damp_speed_x = SmoothDamp(damp_speed_x, g_input.dx, damp_speed_x_vel, 0.1, 400, dt);
	if (abs(damp_speed_x) < 0.0001)
		damp_speed_x = 0;

	damp_speed_y = SmoothDamp(damp_speed_y, g_input.dy, damp_speed_y_vel, 0.1, 400, dt);
	if (abs(damp_speed_y) < 0.0001)
		damp_speed_y = 0;

	Vector3df vv{ damp_speed_x,damp_speed_y,0 };
	vv.x*=0.85;//strafe is slower
	current_moving_speed = vv.getLength();
	if (current_moving_speed > 0)
	{
		walk_tween.update(dt*2*fac);
		g_weapon_axe->setAnimSpeed(15* current_moving_speed);
	}
	else
	{
		g_weapon_axe->setAnimSpeed(2);
	}
	if (g_input.attack)
	{
		g_weapon_axe->attack();
	}
	

	float vel[3]{ 0 };
	vel[0] = -sinf(H3D_DEG2RAD *(ry))*vv.y*move_speed;
	vel[2] = -cos(H3D_DEG2RAD *(ry))*vv.y*move_speed;

	vel[0] += sinf(H3D_DEG2RAD *(ry + 90))*vv.x * move_speed;
	vel[2] += cos(H3D_DEG2RAD *(ry + 90))*vv.x * move_speed;
	
	
	
	auto tr = g_phyis.getTransform(pawn);
	if (tr.grounded == false)
	{
		falling_time += dt;
	}
	if ((tr.grounded || true)&&g_input.jumpPressed && allow_jump)
	{	
		g_phyis.pawnJump(pawn, 270);
		allow_jump = false;
		grounded_anim = true;
		//tween.removeByTag(25);
		//tween.callFuncPeriodic(30*dt, 0, [](float x) {vely = x; }, 0.2, EASING_FUNCTION::Linear,25);
		g_weapon_axe->jump();
		khsound::play_sound(khsound::SOUND_JUMP,0.1);
	}
	if (g_input.jumpPressed == false)
	{
		allow_jump = true;
	}
	if (last_grounded != tr.grounded && tr.grounded && falling_time>0.2)
	{
		float v = -12;
		if(falling_time>1)
		{
			v = -20;
		}
		tween.callFuncPeriodic(0, v , [](float x) {cam_y_ofset = x; }, 0.1, EASING_FUNCTION::QuadraticEaseOut);
		tween.callFuncPeriodic(v, 0, [](float x) {cam_y_ofset = x; }, 0.3, EASING_FUNCTION::QuadraticEaseOut,0,0.1);
		
		g_weapon_axe->land();
		khsound::play_sound(khsound::SOUND_FOOTSTEP_1);
		grounded_anim = false;
	}
	if (tr.grounded)
	{
		falling_time = 0;
	}
	if ((abs(vel[0]) > 100 || abs(vel[2])>100) && tr.grounded)
	{
		walk_time += dt;
		if (walk_time > 0.4f)
		{
			if (foot_sound == 0)
			{
				khsound::play_sound(khsound::SOUND_FOOTSTEP_4,1);
				foot_sound = 1;
			}
			else if (foot_sound == 1)
			{
				khsound::play_sound(khsound::SOUND_FOOTSTEP_2,1);
				foot_sound = 2;
			}
			else if (foot_sound == 2)
			{
				khsound::play_sound(khsound::SOUND_FOOTSTEP_3,1);
				foot_sound = 0;
			}
				
			walk_time = 0;
		}
	}
	else
	{
		//walk_time = 0;
	}
	last_grounded = tr.grounded;
	tween.update(dt);
	g_phyis.setVelocity(pawn, vel);
}
void FPSCharacter::Update(float dt)
{

	auto tr = g_phyis.getTransform(pawn);
	
	tr.y += 28;//cetner to top 
			   //set node transform in h3d
	float cx, cy, cz, rx, ry, rz, t;
	h3dGetNodeTransform(camera, &cx, &cy, &cz, &rx, &ry, &rz, &t, &t, &t);
	float cam_x = tr.x;
	float cam_y = tr.y +cam_y_ofset + cam_y_ofset2;
	float cam_z = tr.z;

	float sens = 0.1;
	// Look left/right
	ry -= g_input.drx*sens;
	// Loop up/down but only in a limited range
	rx += g_input.dry*sens;
	if (rx > 90) rx = 90;
	if (rx < -90) rx = -90;

	

	if (first_update)
	{
		curr_y = cam_y;
		first_update = false;
	}
	if(cam_y!=curr_y)
		curr_y += ((cam_y - curr_y) >= 0 ? 1 : -1)*dt*100;
	float zzrcx = 0;
	if (current_moving_speed>1)
		zzrcx = cam_rz;
	h3dSetNodeTransform(camera, cam_x, cam_y, cam_z, rx, ry, /*zzrcx*/0, 1, 1, 1);
}
