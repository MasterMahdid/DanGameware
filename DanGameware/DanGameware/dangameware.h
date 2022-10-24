#pragma once
struct gentity_s;
typedef gentity_s * gentity_t;
typedef int NodeHandle;
typedef int EntHandle;
struct vector3d
{
	float x, y, z;
};
struct gentity_s
{
	const char* classname;
	int spawnflags;
	int flags;
	NodeHandle node;
	int pawn;

	int clipmask;//collision mask
	gentity_t parent;
	vector3d pos1, pos2;
	float angle;

	char* target;
	char* targetname;
	gentity_t target_ent;
	gentity_t goalentity;
	gentity_t enemy;

	float speed;
	vector3d movedir;

	float nextthink;
	void(*think)(gentity_t);

	int health;
	bool takedamage;
};
enum class MOVE_TYPE
{
	NONE,
	PAWN,
	PROJECTILE,
	PROJECTILE_BOUNCE,
};
class Entity
{

};
void dw_loadmap(char* mapname);
EntHandle dw_create_entity(char* filename);
void dw_ent_distroy(EntHandle ent);













enum class ENTITY_FLAGS
{
	PASSABLE,
	POLYGON,
	NARROW, FAT,
	CLIPPED,
	SHOW,
	INVISIBLE,
	ZNEAR,
	ANIMATE,
	DYNAMIC,
};
enum class ENTITY_EVENTS
{
	EVENT_BLOCK,
	EVENT_ENTITY,
	EVENT_FRICTION,
	EVENT_PUSH,
	EVENT_IMPACT,
	EVENT_SHOOT,
	EVENT_SONAR,
	EVENT_SCAN,
	EVENT_DETECT,
	EVENT_TRIGGER,
	EVENT_FRAME,
};
struct entity
{
	float x, y, z;
	float pan, tilt, roll;
	float scale_x, scale_y, scale_z;
	int layer;
	float min_x, min_y, min_z, max_x, max_y, max_z;//AABB
	ENTITY_FLAGS flags;
	int push;
	int group;
	float trigger_range;
	float floor_dist;
	float user_params[100];
	char* user_strings[8];
	bool user_flags[32];
};

struct vector
{
	float x, y, z;
};
//vector functions
void vec_set(vector* v1, vector* v2);
vector* vec_fill(vector* v, float a);
vector* vec_add(vector* v1, vector* v2);
vector* vec_sub(vector* v1, vector* v2);
vector* vec_diff(vector* dest, vector*v1, vector*v2);
float vec_dist(vector* v1, vector* v2);
float vec_dot(vector* v1, vector* v2);
vector* vec_cross(vector* dest, vector*v1, vector*v2);
vector* vec_mull(vector* v1, vector* v2);
vector* vec_inverse(vector* v);
float vec_length(vector* v);
vector* vec_lerp(vector* dis, vector* v1, vector* v2, float factor);
vector* vec_normalize(vector *v);
vector* vec_scale(vector *v, float factor);
vector* vec_rotate(vector* v, float pan, float tilt);
vector* vec_for_angle(vector* dest, float pan, float tilt);
void vec_to_angle(vector* v, float *pan, float *roll);
vector* vec_for_screen(vector* screen_pos);
vector* vec_to_screen(vector* dest, vector* world_pos);
vector* vec_for_ent(vector* dest, entity* ent);
vector* vec_for_ent(vector* dest,vector* v, entity* ent);
vector* vec_to_ent(vector* dest, vector* v, entity* ent);
vector* vec_for_bone(vector* dest, entity* ent, char* bone);


//entity functions
entity* ent_create(char* filename, vector* position);
void ent_remove(entity* ent);
void ent_morph(entity* ent, char* filename);
entity* ent_clone(entity* ent);
entity* ent_next(entity* ent);
void ent_preload(entity* ent);
void ent_purge(entity* ent);
void ent_decal(entity* target,int texture,float size,float angle);
entity* ent_for_name(char*);
//collision and movement functions
enum col_flags
{
	IGNORE_YOU,
	IGNORE_FLAG2,
	IGNORE_PASSABLE,
	IGNORE_PASSENTS,
	IGNORE_WORLD,
	IGNORE_MAPS,
	IGNORE_MODELS,
	IGNORE_SPRITES,
	IGNORE_PUSH,
	IGNORE_CONTENT,
	ACTIVATE_TRIGGER,
	ACTIVATE_PUSH,
	ACTIVATE_SHOOT,
	ACTIVATE_SONAR,
	USE_POLYGON,
	GLIDE,
};
float c_move(entity* ent, vector* rel_dist, vector* abs_dist, col_flags flags);
float c_rotate(entity* ent, float pan, float tilt, float roll, col_flags flags);
float c_trace(vector* from, vector* to,col_flags flags);







