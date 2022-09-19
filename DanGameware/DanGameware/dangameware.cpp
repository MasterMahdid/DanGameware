#include "dangameware.h"

#define MAX_ENTITY (10000)
#define NULL_ENT (0)
#define NULL_NODE (0);
gentity_s g_ents[MAX_ENTITY];

void init()
{

}
void vector_set(vector3d *v, float x, float y, float z)
{
	v->x = x;
	v->y = y;
	v->z = z;
}
void init_entity(gentity_t ent)
{
	ent->angle = 0;
	ent->classname = "";
	ent->clipmask = 0;
	ent->enemy = NULL_ENT;
	ent->flags = 0;
	ent->goalentity = NULL_ENT;
	ent->health = 0;
	vector_set(&ent->movedir, 0, 0, 0);
	ent->nextthink = 0;
	ent->node = NULL_NODE;
	ent->parent = NULL_ENT;
	ent->pawn = 0;
	vector_set(&ent->pos1, 0, 0, 0);
	vector_set(&ent->pos2, 0, 0, 0);
	ent->spawnflags = 0;
	ent->speed = 0;
	ent->takedamage = false;
	ent->target = "";
	ent->think = nullptr;
	ent->targetname = "";
	ent->target_ent = NULL_ENT;
}