#include <cstdlib>
#include <irrlicht.h>
#include "converter.h"
#include <Windows.h>
#include "CQ3LevelMesh.h"
using namespace irr;

IrrlichtDevice *device;
int main(int argc, char** argv)
{
	auto baseq3 = std::getenv("ALVAHSHI_BASEQ3");
	auto contentfolder = std::getenv("ALVAHSHI_CONTENT");
	if (baseq3 == nullptr || contentfolder == nullptr)
	{
		printf("ALVAHSHI_BASEQ3 or ALVAHSHI_CONTENT not set.");
		return 1;
	}
	if (argc < 2)
	{
		printf("pass map name as cli arg (without .bps)");
		return 2;
	}
	auto mapname = argv[1];
	

	device = createDevice(video::EDT_NULL);
	auto* smgr = device->getSceneManager();
	auto fs = device->getFileSystem();
	
	char outdir[MAX_PATH];
	sprintf(outdir, "%s\\maps\\%s\\", contentfolder, mapname);
	bool direxists = fs->existFile(outdir);
	if (!direxists)
	{
		auto ret = CreateDirectoryA(outdir, NULL);
		if (ret == 0)
		{
			printf("Creating map folder failed.");
			return 1;
		}
	}

	char mappath[MAX_PATH];
	sprintf(mappath, "%s\\maps\\%s.bsp", baseq3, mapname);
	fs->addFolderFileArchive(baseq3);
	irr::scene::quake3::Q3LevelLoadParameter LoadParam;
	auto mesh = new khbsp::CQ3LevelMesh(fs, smgr, LoadParam); //(scene::IQ3LevelMesh*) smgr->getMesh(mappath);
	mesh->loadFile(fs->createAndOpenFile(mappath));
	
	std::string entity_xml;
	irr::scene::quake3::tQ3EntityList &entityList = mesh->getEntityList();
	for (int i = 0; i < entityList.size(); i++)
	{
		entity_xml += "<entity ";
		auto ent = entityList[i];
		for (int j = 0; j < ent.getGroupSize(); j++)
		{
			auto gp = ent.getGroup(j);
			for (int k = 0; k < gp->Variable.size(); k++)
			{
				auto v = gp->Variable[k];
				auto cc = v.name.c_str();
				auto vv  = v.content.c_str();

				char buf[256];
				sprintf(buf, " %s=\"%s\"", cc, vv);
				entity_xml += buf;
			}
		}
		entity_xml += "/>\n";
	}
	char cf[MAX_PATH];
	sprintf(cf,"%s\\", contentfolder);
	Converter c(mesh, cf,baseq3);
	c.processMeshes(false);

	char assetpath[MAX_PATH];
	sprintf(assetpath, "maps\\%s\\", mapname);
	c.writeModel(assetpath, mapname, entity_xml);


	//copy lightmaps
	printf("Copy lightmaps");
	char lightmaps_dir[1024];
	sprintf(lightmaps_dir, "%s\\maps\\%s\\lm_*", baseq3, mapname);
	
	char command[2048];
	sprintf(command, "xcopy \"%s\" \"%s\\%s\" /i /y", lightmaps_dir, contentfolder, assetpath);
	system(command);

	return 0;
}