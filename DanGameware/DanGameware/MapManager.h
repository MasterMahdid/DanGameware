#pragma once
#include <string>
#include <vector>
#include "khmath.h"
/*typedef int H3DRes;
struct MapEntityProp
{
	std::string key;
	std::string value;
};
struct MapEntity
{
	std::vector<MapEntityProp> props;
};

struct Map
{
	std::vector<MapEntity> entities;
	void load();
};
class MapManager
{
	Map _currentMap;
	void load()
	{

	}
};*/
void mapLoad(const char* name, std::function<void()> update);
void addLight(Vector3df pos, Vector3df rot, Vector3df diff, float rad, float intensity, bool shadow, float shadow_bias, float fov);



