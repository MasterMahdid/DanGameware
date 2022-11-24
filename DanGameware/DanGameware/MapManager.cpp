#include <thread>
#include <fstream>
#include <stdlib.h>
#include <fstream>
#include <thread>
#include <Horde3D.h>
#include <functional>
#include "Horde3DUtils.h"
#include "MapManager.h"
#include "PhysicsEngine.h"
#include "Gameplay.h"
#include "khmath.h"
#include "SoundEngine.h"
#include "enemy.h"
#include "ai.h"
#include <GLFW/glfw3.h>
#include "filesystem.h"





extern physicEngine g_phyis;
extern std::vector<GameObject*> gameObjects_array;
extern H3DNode main_camera;
extern std::vector<H3DNode> dynamic_lights;
extern gentity_t g_entities;
extern size_t g_entities_len;
H3DRes envRes, level_mesh_res;
std::vector<std::string> lms;
int add_light_from_map(const char* filename);
int add_player_from_map(const char* filename);
int add_enemy_from_map(const char* filename);


void removeAll()
{
	//TODO: implement this correctly
	for (int i = 3;i<10000; i++)
	{
		h3dRemoveNode(i);
	}
	
}
H3DRes fireparticle;

void dw_console_log(const char* fmt, ...);

void mapPreload(const char* name, std::function<void()> update)
{
	//2-load the new map
	char geo_path[1024];
	sprintf(geo_path, "maps/%s/%s.geo", name, name);

	char scene_path[1024];
	sprintf(scene_path, "maps/%s/%s.scene.xml", name, name);

	level_mesh_res = h3dAddResource(H3DResTypes::Geometry, geo_path, 0);
	envRes = h3dAddResource(H3DResTypes::SceneGraph, scene_path, 0);

	//TODO unload lightmaps
	h3dUnloadResource(level_mesh_res);
	h3dUnloadResource(envRes);
	//3- load entities

	WeaponAxe::initRes();
	Enemy::add_res();

	size_t sz2;
	char* data = (char*)g_archive_reader.loadFileData(scene_path,sz2);
	lms.clear();
	auto chr = strstr(data, "lightmap_id=\"");
	while (chr != 0)
	{
		char num[3] = { 0 };
		if (chr[14] >= '0' && chr[14] <= '9')//two digit
		{
			num[0] = chr[13];
			num[1] = chr[14];
		}
		else
		{
			num[0] = '0';
			num[1] = chr[13];
		}
		char lightmap_path[1024];
		sprintf(lightmap_path, "maps/%s/lm_00%s.tga", name, num);
		lms.push_back(lightmap_path);
		h3dAddResource(H3DResTypes::Texture, lightmap_path, 0);
		chr = strstr(chr + 14, "lightmap_id=\"");
	}
	//4-load resources from disk
	g_archive_reader.loadResourcesFromKhArchive(update);
	khsound::load_sounds(update);

}

void mapLoad(const char* name, std::function<void()> update)
{
	update();
	mapPreload(name, update);
	
	//1- unload the previous map
	for (const auto& go : gameObjects_array)
	{
		delete go;
	}
	gameObjects_array.clear();

	removeAll();
	khsound::stopAll();
	g_phyis.reset();
	//5- setup
	H3DNode env = h3dAddNodes(H3DRootNode, envRes);
	h3dSetNodeTransform(env, 0, 0, 0, 0, 0, 0, 1, 1, 1);

	int num = h3dFindNodes(env, "", H3DNodeTypes::Mesh);
	//int num = lms.size();
	for (size_t i = 0; i < num; i++)
	{
		char temp[100];
		//sprintf(temp, "bspconv-%d", i);
		//h3dFindNodes(env, temp, H3DNodeTypes::Mesh);
		//auto node = h3dGetNodeFindResult(0);
		auto node = h3dGetNodeFindResult(i);
		auto mat = h3dGetNodeParamI(node, H3DMesh::MatResI);
		auto matname = h3dGetResName(mat);
		
		size_t sz;
		char* data = (char*)g_archive_reader.loadFileData(matname,sz);
		if (data == nullptr)
			continue;
		
		std::string addd = std::string("<Sampler name=\"lightMap\" map=\"")+ lms[i]+std::string("\" /></Material>");


		auto str = strstr(data, "</Material>");
		data[str - data] = 0;
		auto dddsd = std::string(data) + addd;
		auto ffres = dddsd.c_str();
		sz = dddsd.length();








		char matname2[1024];
		sprintf(matname2, "%druntime/%s",i,matname);

		auto mat2res = h3dAddResource(H3DResTypes::Material, matname2, 0);
		
		h3dLoadResource(mat2res, ffres, sz);

		delete[] data;
		

		h3dSetNodeParamI(node, H3DMesh::MatResI, mat2res);


		

	}
	// Add skybox
	//H3DNode sky = h3dAddNodes(H3DRootNode, skyBoxRes);
	//h3dSetNodeTransform(sky, 0, 0, 0, 0, 0, 0, 18000, 4000, 18000);
	//h3dSetNodeFlags(sky, H3DNodeFlags::NoCastShadow, true);
	int tri_count = -1;
	auto tri_data = g_phyis.createLevelPhysTriData(level_mesh_res, tri_count);
	g_phyis.loadLevel(tri_data, tri_count);
	

	auto wx = new WeaponAxe();
	gameObjects_array.push_back(wx);
	g_weapon_axe = wx;

	if (g_entities != nullptr)
		delete[] g_entities;

	g_entities = new gentity_s[100];
	g_entities_len = 0;
	
	
	char mapfn[1024];
	sprintf(mapfn, "maps/%s/%s.map",name,name);
	
	add_light_from_map(mapfn);
	add_player_from_map(mapfn);
	//add_enemy_from_map(mapfn);
	//khsound::play_sound(khsound::E1M1_MUSIC,1,true);

}
std::string map_find_ent_prop_in_string(const char* key, const std::string& str, size_t start, size_t end)
{
	std::string res;
	char buf[25];
	sprintf(buf, "\"%s\"", key);
	auto dbg_substr = str.substr(start, end - start);

	auto of = dbg_substr.find(buf);
	if (of != std::string::npos)
	{
		of += strlen(buf);
		size_t qt_1 = dbg_substr.find("\"", of + 1);
		size_t qt_2 = dbg_substr.find("\"", qt_1 + 1);
		res = dbg_substr.substr(qt_1 + 1, qt_2 - qt_1 - 1);
	}
	return res;
}
Vector3df map_get_end_prop_value_as_position(std::string value)
{

	auto of4 = value.find(" ");
	auto of5 = value.find(" ", of4 + 1);

	auto xx = std::atof(value.substr(0, of4).c_str());
	auto yy = std::atof(value.substr(of4, of5 - of4).c_str());
	auto zz = std::atof(value.substr(of5).c_str());

	return Vector3df(-xx, zz, yy);
}
Vector3df map_get_end_prop_value_as_color(std::string value)
{

	auto of4 = value.find(" ");
	auto of5 = value.find(" ", of4 + 1);

	auto xx = std::atof(value.substr(0, of4).c_str());
	auto yy = std::atof(value.substr(of4, of5 - of4).c_str());
	auto zz = std::atof(value.substr(of5).c_str());

	return Vector3df(xx, yy, zz);
}
void addLight(Vector3df pos,Vector3df diff,float rad,float intensity,bool shadow)
{
	auto light = h3dAddLightNode(H3DRootNode, "Light1", 0, "LIGHTING", "SHADOWMAP");
	h3dSetNodeParamF(light, H3DLight::FovF, 0, 360);
	h3dSetNodeParamF(light, H3DLight::RadiusF, 0, rad);
	h3dSetNodeParamF(light, H3DLight::ColorMultiplierF, 0, intensity);
	if(shadow)
	{
		h3dSetNodeParamI(light, H3DLight::ShadowMapCountI, 3);
		h3dSetNodeParamF(light, H3DLight::FovF, 0, 90);
		h3dSetNodeTransform(light, pos.x, pos.y, pos.z, -52, -165, 0, 1, 1, 1);
	}
	else
	{
		h3dSetNodeTransform(light, pos.x, pos.y, pos.z, 0, -86, 0, 1, 1, 1);
		h3dSetNodeParamF(light, H3DLight::FovF, 0, 360);
	}
	
	h3dSetNodeParamF(light, H3DLight::ShadowMapBiasF, 0, 0.003f);
	h3dSetNodeParamF(light, H3DLight::ColorF3, 0, diff.x);
	h3dSetNodeParamF(light, H3DLight::ColorF3, 1, diff.y);
	h3dSetNodeParamF(light, H3DLight::ColorF3, 2, diff.z);

	dynamic_lights.push_back(light);

	//H3DNode part = h3dAddNodes(H3DRootNode, fireparticle);
	//h3dSetNodeTransform(part, pos.x, pos.y,pos.z, 0, 0, 0, 1, 1, 1);
}
int add_light_from_map(const char* filename)
{
	size_t sz;
	auto ret = g_archive_reader.loadFileData(filename, sz);
	std::string str((char*)ret, sz);
	size_t of = 0;
	int lind = 0;
	while (true)
	{
		of = str.find("\"classname\" \"DynamicLight\"", of + 1);
		if (of == std::string::npos)
			break;
		auto of_end = str.find("}", of);
		auto ssss = str.substr(of, of_end - of + 1);
		auto temp_str = map_find_ent_prop_in_string("origin", str, of, of_end);
		auto pos = map_get_end_prop_value_as_position(temp_str);

		Vector3df diff(1, 1, 1);
		temp_str = map_find_ent_prop_in_string("diffuse_color", str, of, of_end);
		if (temp_str != "")//diffuse_color is optional
		{
			diff = map_get_end_prop_value_as_color(temp_str);
		}
		
		float radius = 100;
		temp_str = map_find_ent_prop_in_string("radius", str, of, of_end);
		if (temp_str != "")
		{
			radius = std::atof(temp_str.c_str());
		}
		float intensity=3;
		temp_str = map_find_ent_prop_in_string("intensity", str, of, of_end);
		if (temp_str != "")
		{
			intensity = std::atof(temp_str.c_str());
		}

		bool shadow = false;
		temp_str = map_find_ent_prop_in_string("fog_power", str, of, of_end);
		if (temp_str != "")
		{
			f32 fog_power = std::atof(temp_str.c_str());
			if (fog_power > 4)
				shadow = true;
		}

		addLight(pos,diff,radius, intensity,shadow);
		lind++;
		if (512 == lind)
			break;
	}
	return lind;
}
int add_player_from_map(const char* filename)
{
	size_t sz;
	auto ret = g_archive_reader.loadFileData(filename, sz);
	std::string str((char*)ret, sz);
	size_t of = 0;
	int lind = 0;
	while (true)
	{
		of = str.find("\"classname\" \"info_player_start\"", of + 1);
		if (of == std::string::npos)
			break;
		auto of_end = str.find("}", of);
		auto temp_str = map_find_ent_prop_in_string("origin", str, of, of_end);
		auto pos = map_get_end_prop_value_as_position(temp_str);
		
		h3dSetNodeTransform(main_camera, pos.x, pos.y, pos.z, 1, 90, 1, 1, 1, 1);
		auto pp = new FPSCharacter(main_camera,&g_entities[g_entities_len++]);
		gameObjects_array.push_back(pp);
		::fpsCharacter = pp;
		auto flycam = new flyThroughCam(main_camera);
		::flyCam = flycam;
		gameObjects_array.push_back(flycam);

		break;
	}
	return lind;
}
int add_enemy_from_map(const char* filename)
{
	size_t sz;
	auto ret = g_archive_reader.loadFileData(filename, sz);
	std::string str((char*)ret, sz);
	size_t of = 0;
	int lind = 0;
	while (true)
	{
		of = str.find("\"classname\" \"info_player_deathmatch\"", of + 1);
		if (of == std::string::npos)
			break;
		auto of_end = str.find("}", of);
		auto temp_str = map_find_ent_prop_in_string("origin", str, of, of_end);
		auto pos = map_get_end_prop_value_as_position(temp_str);
		auto ent = &g_entities[g_entities_len++];
		ent->pos1 = pos;

		temp_str = map_find_ent_prop_in_string("angle", str, of, of_end);
		ent->angle = 0;
		if (temp_str != "")
		{
			ent->angle = std::atof(temp_str.c_str());
		}
		ent->angle -= 90;
		auto emn = new Enemy(ent);
		gameObjects_array.push_back(emn);
	}
	return lind;
}
