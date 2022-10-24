#pragma once
#include "khmath.h"
#include "Horde3D.h"
struct gentity_s;
typedef gentity_s * gentity_t;

extern gentity_t player_ent;
struct gentity_s
{
	const char* classname;
	int spawnflags;
	int flags;
	H3DNode node;
	int pawn;
	
	int clipmask;//collision mask
	gentity_t parent;
	Vector3df pos1, pos2;
	float angle;

	char* target;
	char* targetname;
	gentity_t target_ent;
	gentity_t goalentity;
	gentity_t enemy;

	float speed;
	Vector3df movedir;

	float nextthink;
	void(*think)(gentity_t);

	int health;
	bool takedamage;
};


enum class Range
{
	RANGE_MELLEE,
	RANGE_NEAR,
	RANGE_MID,
	RANGE_FAR
};
Range getRange(f32 r);

//returns true if the entity is visible to self, even if not infront()
bool visible(gentity_t self, gentity_t target);
//return true if the entity is front (in sight) of self
bool infront(gentity_t self, gentity_t target);
bool canShoot(gentity_t self, gentity_t target);

bool findTarget(gentity_t self);
void update_ents(gentity_t ents, size_t len, float dt);
void init_enemy(gentity_t ent);

