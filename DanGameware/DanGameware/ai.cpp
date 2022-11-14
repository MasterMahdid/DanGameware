#include "ai.h"
#include "Horde3D.h"
#include <stdio.h>
#include <stdlib.h>
#include "Gameplay.h"
#include "utMath.h"
#include "debug_draw.h"
Range getRange(f32 r)
{
	if (r < 60)
		return Range::RANGE_MELLEE;
	if (r < 180)
		return Range::RANGE_NEAR;
	if (r < 400)
		return Range::RANGE_MID;
	if(r<2000)
		return Range::RANGE_FAR;
	return Range::RANGE_XFAR;
	
}
bool visible(gentity_t self, gentity_t target)
{
	Vector3df head_ofset = Vector3df(0, 45, 0);
	Vector3df vhit;
	//dd_sphere(self->pos1 + head_ofset, 5, DD_RED, 0.3);
	//dd_line(self->pos1 + head_ofset, target->pos1, DD_WHITE, 0.3);
	bool hit = g_phyis.trace(self->pos1 + head_ofset, target->pos1 + head_ofset,vhit);
	if (hit)
	{
		//dd_sphere(vhit, 5, DD_PURPLE, 0.3);
		return false;

	}
		
	if (getRange((self->pos1 - target->pos1).getLength()) == Range::RANGE_XFAR)
		return false;
	return true;
}
bool infront(gentity_t self, gentity_t target)
{
	Horde3D::Vec3f vforward(0,0,200);
	float* mat = new float[16];
	h3dGetNodeTransMats(self->node, NULL, (const float**)&mat);
	Horde3D::Matrix4f mt2(mat);
	vforward = (mt2*vforward);
	vforward = (vforward - Horde3D::Vec3f(self->pos1.x, self->pos1.y, self->pos1.z)).normalized();
	Vector3df vec = (target->pos1 - self->pos1).normalize();
	float dot = vforward.dot(Horde3D::Vec3f(vec.x,vec.y,vec.z));
	if (dot > 0.3)
		return true;
	return false;
}
bool canShoot(gentity_t self, gentity_t target)
{
	return (infront(self, target) && visible(self, target));
}

void ent_think(gentity_t ent,float dt)
{
	if(ent->nextthink>0)
		ent->nextthink -= dt;
	if (ent->think!=nullptr && ent->nextthink <= 0)
	{
		ent->think(ent);
	}
}
void ent_fetch(gentity_t ent)
{
	H3DNode node = ent->node;
	float x, y, z,ry;

	h3dGetNodeTransform(node, &x, &y, &z, nullptr,&ry, nullptr, nullptr, nullptr, nullptr);
	ent->pos1.x = x;
	ent->pos1.y = y;
	ent->pos1.z = z;
	ent->angle = ry;
}
void update_ents(gentity_t ents, size_t len,float dt)
{
	for (int i = 0; i < len; i++)
	{
		gentity_t ent = ents+i;
		ent_fetch(ent);
		ent_think(ent,dt);
	}
}

bool findTarget(gentity_t self)
{
	gentity_t player = self;
	if (infront(self, self->enemy) == false)
		return false;
	if (visible(self, self->enemy) == false)
		return false;
	//self->enemy = self->target_ent =  ;
	return true;
}
void think_shoot(gentity_t self);
void think_melee(gentity_t self);
void think_run(gentity_t self);
void think_stand(gentity_t self);

void think_melee(gentity_t self)
{
	float ran = (rand()*1.0f)/RAND_MAX;
	self->speed = 0;
	if (ran > 0.5)
	{
		gentity_t player = self->enemy;
		Vector3df move_dir = (self->pos1 - player->pos1);
		Range range = getRange(move_dir.getLength());
		move_dir.y = 0;
		self->movedir = move_dir.normalize();
		self->speed = 100;
		float random = (rand()*1.0f) / RAND_MAX;
		self->nextthink = random*2+2;
		self->think = think_run;
	}
	else
	{
		//melee player
		printf("melee player\n");
		self->think = think_melee;
		self->nextthink = 1;

	}
}
void think_shoot(gentity_t self)
{
	auto projectile = new ProjectileAxe();
	addGameObject(projectile);
	Vector3df move_dir = (self->pos1 + Vector3df(0, 40, 0) - (self->enemy->pos1+Vector3df(0, 30, 0))).normalize()*-1;
	//move_dir.y = 0;
	projectile->shoot(self->pos1+Vector3df(0,40,0), move_dir,self->pawn);

	//self->speed = 0;
	self->think = think_run;
	self->nextthink = 0.3;
	printf("shoot player\n");
	//shoot player
}

void think_run(gentity_t self)
{
	gentity_t player = self->enemy;
	Vector3df move_dir = (self->pos1 - player->pos1)*-1;
	Range range = getRange(move_dir.getLength());
	move_dir.y = 0;
	self->movedir = move_dir.normalize();
	self->speed = 150;
	/*if (range == Range::RANGE_MELLEE)
	{
		self->think = &think_melee;
		self->nextthink = 1;
	}*/
	if (range == Range::RANGE_NEAR)
	{
		self->speed = 0;
		self->think = think_run;//get closer
		self->nextthink = 0.5;
		float ran = (rand()*1.0f) / RAND_MAX;
		if (canShoot(self, self->enemy) && ran>0.8f)
		{
			self->think = &think_shoot;
			self->nextthink = 0.3;
		}
	}
	else if (range == Range::RANGE_MID || range == Range::RANGE_FAR)
	{
		float ran = (rand()*1.0f) / RAND_MAX;
		if (canShoot(self, self->enemy) && ran>0.5f)
		{
			self->think = &think_shoot;
			self->nextthink = 0.3;
		}
		else
		{
			self->think = think_run;//get closer
			self->nextthink = 0.5;
		}
	}
}
void think_stand(gentity_t self)
{
	self->speed = 0;
	if (findTarget(self))
	{
		self->think = &think_run;
		self->nextthink = 0.3;
	}
	else
	{
		self->nextthink = 0.1;
	}
}

void init_enemy(gentity_t ent)
{
	ent->speed = 0;
	ent->movedir = Vector3df();
	ent->think = think_stand;
	//ent->think = think_shoot;
	ent->nextthink = 0.1;
	ent->enemy = player_ent;
}