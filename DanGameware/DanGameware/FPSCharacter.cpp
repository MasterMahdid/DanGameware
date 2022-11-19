#include "Gameplay.h"
#include "Tween.h"
#include "khmath.h"
#include "SoundEngine.h"
//#include "debug_draw.h"
void dw_console_log(const char* fmt, ...);
gentity_t player_ent;

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
	f32 cam_rx = 0, cam_ry = 0,cam_rz=0;
	bool grounded_anim = true;
	float falling_time = 0;
	float current_moving_speed = 0;
	f32 bloodmask = 0;
	f32 ofset_rx = 0, ofset_rz = 0;
	f32 head_height = 23;
	H3DRes cammat;
	int bloodMaskIndex;
	float walk_time = 0;
	int foot_sound = 0;
	float damp_speed_x = 0;
	float damp_speed_x_vel = 0;
	float damp_speed_y = 0;
	float damp_speed_y_vel = 0;
	float animTime;
	bool shaking = false;
	f32 shaking_mag;
	bool allow_attack = true;
	bool playerDead = false;
}
int player_health = 100;

using namespace fpscharacter_internal;


void detectMat()
{
	cammat = h3dAddResource(H3DResTypes::Material, "pipelines/postalve.material.xml",0);
	bloodMaskIndex = h3dFindResElem(cammat, H3DMatRes::UniformElem, H3DMatRes::UnifNameStr, "bloodMask");
}
FPSCharacter::FPSCharacter(H3DNode cam,gentity_t ent)
{
	camera = cam;
	float cx, cy, cz;
	h3dGetNodeTransform(cam, &cx, &cy, &cz, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
	float pos[3]{ cx,cy,cz};
	pawn = g_phyis.createPawn(16, 56, pos,(void*)this,PAWN_FLAG_PLAYER);
	walk_tween.callFuncPeriodic(2, 0, [](float x) {cam_y_ofset2 = x; }, 0.5, EASING_FUNCTION::SineEaseOut, 26,0,0,true);
	walk_tween.callFuncPeriodic(-0.4, +0.4, [](float x) {cam_rz = x; }, 0.4, EASING_FUNCTION::SineEaseOut, 26, 0, 0, true);
	ent->pawn = pawn;

	ent->node = cam;
	ent->think = nullptr;
	ent->speed = 0;
	ent->movedir = Vector3df();
	::player_ent = ent;
	auto light = h3dAddLightNode(cam, "Light1", 0, "LIGHTING", "SHADOWMAP");
	h3dSetNodeTransform(light, 0, 0, 0, -86, 0, 0, 1, 1, 1);
	h3dSetNodeParamF(light, H3DLight::FovF, 0, 360);
	h3dSetNodeParamF(light, H3DLight::RadiusF, 0,200);
	h3dSetNodeParamF(light, H3DLight::ColorMultiplierF, 0,0.25);
	h3dSetNodeParamI(light, H3DLight::ShadowMapCountI, 0);
	h3dSetNodeParamF(light, H3DLight::ShadowMapBiasF, 0, 0.003f);
	h3dSetNodeParamF(light, H3DLight::ColorF3, 0, 1);
	h3dSetNodeParamF(light, H3DLight::ColorF3, 1, 0.75);
	h3dSetNodeParamF(light, H3DLight::ColorF3, 2,0.50);
	detectMat();

	tween =  Tween();
	walk_tween = Tween();
	vely = 0;
	move_speed = 100;
	fac = 0;
	allow_jump = true;
	last_grounded = false;
	cam_y_ofset=0;
	cam_y_ofset2=0;
	cam_rx = 0, cam_ry = 0, cam_rz = 0;
	grounded_anim = true;
	falling_time = 0;
	current_moving_speed = 0;
	bloodmask = 0;
	ofset_rx = 0, ofset_rz = 0;
	head_height = 23;
	walk_time = 0;
	foot_sound = 0;
	damp_speed_x = 0;
	damp_speed_x_vel = 0;
	damp_speed_y = 0;
	damp_speed_y_vel = 0;
	animTime=0;
	shaking = false;
	shaking_mag = 0;
	allow_attack = true;
	playerDead = false;
	player_health = 100;
	//tween.callFuncPeriodic(0, 1, [](float x) {bloodmask = x; }, 0.4615 / 2, EASING_FUNCTION::Linear, 0, 0, 0, true);
}
FPSCharacter::~FPSCharacter()
{
}


void StartShake(f32 mag,f32 duration)
{
	shaking = true;
	shaking_mag = mag;
	tween.delayCall(duration, [&]()
	{
		shaking = false;
	});
}
void FPSCharacter::PhysicUpdate(float dt)
{
	float rx, ry, rz, t;
	h3dGetNodeTransform(camera, &t, &t, &t, &rx, &ry, &rz, &t, &t, &t);
	float st = 0.1;
	if (playerDead)
	{
		g_input.dx = g_input.dy = 0;
		g_input.attack = false;
		g_input.jumpPressed = false;
		g_input.drx = g_input.dry = 0;
		st = 0.5;
	}
	damp_speed_x = SmoothDamp(damp_speed_x, g_input.dx, damp_speed_x_vel, st, 400, dt);
	if (abs(damp_speed_x) < 0.0001)
		damp_speed_x = 0;

	damp_speed_y = SmoothDamp(damp_speed_y, g_input.dy, damp_speed_y_vel, st, 400, dt);
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
	if (g_input.attack&& allow_attack)
	{
		g_weapon_axe->attack(pawn);
		//onProjectileHit(Vector3df());
		//allow_attack = false;
	}
	if (!g_input.attack)
	{
		allow_attack = true;
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
	if ((tr.grounded)&&g_input.jumpPressed && allow_jump)
	{	
		g_phyis.pawnJump(pawn, 270);
		allow_jump = false;
		grounded_anim = true;
		//tween.removeByTag(25);
		//tween.callFuncPeriodic(30*dt, 0, [](float x) {vely = x; }, 0.2, EASING_FUNCTION::Linear,25);
		g_weapon_axe->jump();
		khsound::play_sound(khsound::SOUND_JUMP,0.5);
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

f32 random(f32 min, f32 max)
{
	f32 r = (rand()*1.0f) / RAND_MAX;
	return min + (max - min)*r;
}

void FPSCharacter::Update(float dt)
{
	auto tr = g_phyis.getTransform(pawn);
	tr.y += head_height;//cetner to top 
	float cam_x = tr.x;
	float cam_y = tr.y;
	float cam_z = tr.z;
	
	if (!playerDead)
	{
		cam_y = cam_y + cam_y_ofset + cam_y_ofset2;
		float sens = 0.1;
		cam_ry -= g_input.drx*sens;// Look left/right
		cam_rx += g_input.dry*sens;// Loop up/down but only in a limited range
		if (cam_rx > 90) cam_rx = 90;
		if (cam_rx < -90) cam_rx = -90;
	}
	
	float shake_x = 0, shake_y = 0;
	if (shaking)
	{
		shake_x = random(-1, 1)*shaking_mag;
		shake_y = random(-1, 1)*shaking_mag;
	}
	if (!flyCamEnabled)
		h3dSetNodeTransform(camera, cam_x, cam_y, cam_z, cam_rx+ ofset_rx, cam_ry, cam_rz+ shake_y+ ofset_rz, 1, 1, 1);
	h3dSetResParamF(cammat, H3DMatRes::UniformElem, bloodMaskIndex, H3DMatRes::UnifValueF4, 0, bloodmask);

	//dd_sphere(Vector3df(), 200, DD_RED);
}
void map_reload();
void FPSCharacter::onProjectileHit(Vector3df hit_pos)
{
	if (playerDead)
		return;
	

	player_health -= random(4,8);
	khsound::play_sound(khsound::SOUND_GORE_1);
	if (player_health <= 0)
	{
		playerDead = true;
		tween.removeByTag(113);
		tween.callFuncPeriodic(0, 2, [](float x) {bloodmask = x; }, 0.3, EASING_FUNCTION::Linear);
		tween.callFuncPeriodic(0, 10, [](float x) {ofset_rz = x; },1, EASING_FUNCTION::Linear);
		tween.callFuncPeriodic(head_height, -23, [](float x) {head_height = x; }, 1, EASING_FUNCTION::BounceEaseOut);
		g_weapon_axe->death();
		khsound::play_sound(khsound::SOUND_DEATH);
		tween.delayCall(1, [](){
			map_reload();
		});
		

	}
	else
	{
		StartShake(3, 0.15f);
		tween.removeByTag(113);
		tween.callFuncPeriodic(0, 0.7, [](float x) {bloodmask = x; }, 0.05, EASING_FUNCTION::Linear, 113);
		tween.callFuncPeriodic(0.7, 0, [](float x) {bloodmask = x; }, 0.2, EASING_FUNCTION::Linear, 113, 0.05);


		tween.callFuncPeriodic(0, 8, [](float x) {ofset_rx = x; }, 1, EASING_FUNCTION::ElasticEaseOut, 113);
		//tween.callFuncPeriodic(4, 0, [](float x) {ofset_rx = x; }, 0.1, EASING_FUNCTION::Linear, 113, 0.8);
	}

	
}

void FPSCharacter::shootCamAnim()
{
	ofset_rx = 0;
	tween.callFuncPeriodic(0, 6, [](float x) {ofset_rx = x; }, 0.35, EASING_FUNCTION::CircularEaseIn,666,0);
	tween.callFuncPeriodic(6, 0, [](float x) {ofset_rx = x; }, 0.9, EASING_FUNCTION::ElasticEaseOut, 666,0.35);

}
