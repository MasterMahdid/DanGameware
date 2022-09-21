#include <thread>
#include <fstream>
#include <stdlib.h>
#include <fstream>
#include <Horde3D.h>
#include "Horde3DUtils.h"
#include "MapManager.h"
#include "PhysicsEngine.h"
#include "Gameplay.h"
#include "khmath.h"
#include "SoundEngine.h"
#include "enemy.h"
#include "ai.h"


extern physicEngine g_phyis;
extern std::vector<GameObject*> gameObjects_array;
extern H3DNode main_camera;
extern std::vector<H3DNode> dynamic_lights;
extern gentity_t g_entities;
int add_light_from_map(const char* filename);
int add_player_from_map(const char* filename);
void removeAll()
{
	//TODO: implement this correctly
	for (int i = 3;i<100; i++)
	{
		h3dRemoveNode(i);
	}
	
}
H3DRes fireparticle;
void* loadResourceData(const char* name, const char* content_dir, size_t& out_size)
{
	char path[1024] = {};
	sprintf(path, "%s/%s", content_dir, name);
	std::ifstream inf;
	inf.clear();
	inf.open(path, std::ios::binary);
	if (inf.good() == false)
	{
		return nullptr;
	}
	inf.seekg(0, std::ios::end);
	out_size = inf.tellg();
	char* ret = new char[out_size];
	inf.seekg(0);
	inf.read(ret, out_size);
	inf.close();
	return ret;
}
void mapLoad(const char* name)
{
	const char* baseq3 = getenv("ALVAHSHI_BASEQ3");
	const char* content_dir = getenv("ALVAHSHI_CONTENT");
	//1- unload the previous map
	for (const auto& go : gameObjects_array)
	{
		delete go;
	}
	gameObjects_array.clear();
	
	//removeAll();
	
	
	//2-load the new map
	char geo_path[1024];
	sprintf(geo_path,"maps/%s/%s.geo", name, name);

	char scene_path[1024];
	sprintf(scene_path, "maps/%s/%s.scene.xml", name, name);

	char lightmap_path[1024];
	sprintf(lightmap_path, "maps/%s/lm_0000.tga", name, name);

	H3DRes skyBoxRes = h3dAddResource(H3DResTypes::SceneGraph, "models/skybox/skybox.scene.xml", 0);
	H3DRes lightmapRes = h3dAddResource(H3DResTypes::Texture, lightmap_path, 0);
	
	fireparticle = h3dAddResource(H3DResTypes::SceneGraph, "particles/fire/fire.scene.xml", 0);
	
	auto level_mesh_res = h3dAddResource(H3DResTypes::Geometry, geo_path, 0);
	H3DRes envRes = h3dAddResource(H3DResTypes::SceneGraph, scene_path, 0);

	//TODO unload lightmaps
	//H3DRes lmres = h3dAddResource(H3DResTypes::Texture, "models/esatwall_light/lm_0000.tga", 0);
	//H3DRes lmres2 = h3dAddResource(H3DResTypes::Texture, "models/esatwall_light/lm_0000.tga", 0);
	//h3dUnloadResource(lmres);
	//h3dUnloadResource(lmres2);
	h3dUnloadResource(level_mesh_res);
	h3dUnloadResource(envRes);
	//3- load entities


	WeaponAxe::initRes();
	Enemy::add_res();
	//4-load resources from disk
	h3dutLoadResourcesFromDisk(content_dir);

	
	
	
	
	//5- setup
	H3DNode env = h3dAddNodes(H3DRootNode, envRes);
	h3dSetNodeTransform(env, 0, 0, 0, 0, 0, 0, 1, 1, 1);

	int num = h3dFindNodes(env, "", H3DNodeTypes::Mesh);
	for (size_t i = 0; i < num; i++)
	{
		auto node = h3dGetNodeFindResult(i);
		auto mat = h3dGetNodeParamI(node, H3DMesh::MatResI);
		auto matname = h3dGetResName(mat);
		
		size_t sz;
		char* data = (char*)loadResourceData(matname,content_dir,sz);
		if (data == nullptr)
			continue;
		
		std::string addd = std::string("<Sampler name=\"lightMap\" map=\"")+std::string(lightmap_path)+std::string("\" /></Material>");


		auto str = strstr(data, "</Material>");
		data[str - data] = 0;
		auto dddsd = std::string(data) + addd;
		auto ffres = dddsd.c_str();
		sz = dddsd.length();








		char matname2[1024];
		sprintf(matname2, "runtime/%s",matname);

		auto mat2res = h3dAddResource(H3DResTypes::Material, matname2, 0);
		
		h3dLoadResource(mat2res, ffres, sz);

		delete[] data;
		

		h3dSetNodeParamI(node, H3DMesh::MatResI, mat2res);


		

	}
	// Add skybox
	H3DNode sky = h3dAddNodes(H3DRootNode, skyBoxRes);
	h3dSetNodeTransform(sky, 0, 0, 0, 0, 0, 0, 18000, 4000, 18000);
	h3dSetNodeFlags(sky, H3DNodeFlags::NoCastShadow, true);
	int tri_count = -1;
	auto tri_data = g_phyis.createLevelPhysTriData(level_mesh_res, tri_count);
	g_phyis.loadLevel(tri_data, tri_count);
	

	auto wx = new WeaponAxe();
	gameObjects_array.push_back(wx);
	g_weapon_axe = wx;

	


	

	
	g_entities = new gentity_s[2];

	auto emn = new Enemy();
	gameObjects_array.push_back(emn);
	
	
	g_entities[0].node = main_camera;
	g_entities[0].think = nullptr;
	g_entities[0].speed = 0;
	g_entities[0].movedir = Vector3df();

	g_entities[1].enemy = &g_entities[0];
	g_entities[1].speed = 0;
	g_entities[1].movedir = Vector3df();
	

	
	

	
	char mapfn[1024];
	sprintf(mapfn, "%s\\maps\\%s.map", baseq3,name);
	
	add_light_from_map(mapfn);
	add_player_from_map(mapfn);

	khsound::load_sounds();
	//khsound::play_sound(khsound::E1M1_MUSIC,0.6f);
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
void addLight(Vector3df pos,Vector3df diff,float rad,float intensity)
{
	auto light = h3dAddLightNode(H3DRootNode, "Light1", 0, "LIGHTING", "SHADOWMAP");
	h3dSetNodeTransform(light, pos.x, pos.y, pos.z, 0, 0, 0, 1, 1, 1);
	h3dSetNodeParamF(light, H3DLight::FovF, 0, 90);
	//h3dSetNodeParamF(light, H3DLight::FovF, 0, 360);
	h3dSetNodeParamF(light, H3DLight::RadiusF, 0, rad);
	h3dSetNodeParamF(light, H3DLight::ColorMultiplierF, 0, intensity);
	h3dSetNodeParamI(light, H3DLight::ShadowMapCountI, 0);
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
	std::ifstream t(filename);
	std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
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
		addLight(pos,diff,radius, intensity);
		lind++;
		if (512 == lind)
			break;
	}
	return lind;
}
int add_player_from_map(const char* filename)
{
	std::ifstream t(filename);
	std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
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
		auto pp = new FPSCharacter(main_camera);
		gameObjects_array.push_back(pp);
		/*auto flycam = new flyThroughCam(main_camera);
		gameObjects_array.push_back(flycam);*/

		break;
	}
	return lind;
}
