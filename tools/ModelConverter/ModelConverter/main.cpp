#include <iostream>
#include <vector>
#include <fstream>
#include <algorithm>
#include <windows.h>
#include <direct.h>
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "converter.h"
#undef min
Mesh* processMesh(aiMesh *mesh, const aiScene *scene, ConvertScene& outs);
void processNode(aiNode *node, const aiScene *scene, ConvertScene& outs);
void writeGeometry(FILE* f, ConvertScene *s);
bool writeSceneGraph(ConvertScene& scene, const std::string &assetPath, const std::string &assetName, const std::string &modelName);
void createDirectories(const std::string &basePath, const std::string &newPath);
template<class T>
inline char* elemcpy_le(T* dest, const T* src, size_t num_elems)
{
	memcpy(dest, src, num_elems * sizeof(*src));
	return (char*)(src)+num_elems * sizeof(*src);
}

template<class T>
inline void fwrite_le(const T* data, size_t count, FILE* f)
{
	char buffer[256];
	//ASSERT(sizeof(T) < sizeof(buffer));
	const size_t capacity = sizeof(buffer) / sizeof(T);

	size_t i = 0;
	while (i < count)
	{
		size_t nelems = std::min(capacity, (count - i));
		data = (const T*)elemcpy_le((T*)(buffer), data, nelems);
		fwrite(buffer, sizeof(T), nelems, f);
		i += nelems;
	}
}

int main(int argc, char** argv)
{
	Assimp::Importer importer;
	const aiScene *scene = importer.ReadFile("d:/Alvahshi/game/artsource/models/weapons/fiveseven/fiveseven.fbx", aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_FindDegenerates);
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
		return 1;
	}
	ConvertScene s;
	s._outPath = "d:/Alvahshi/game/content/";
	processNode(scene->mRootNode, scene, s);

	createDirectories(s._outPath, "models/fbxtest/");
	FILE *geomfile = fopen((s._outPath + "models/fbxtest/" + "fiveseven"+ ".geo").c_str(), "wb");
	writeGeometry(geomfile, &s);

	writeSceneGraph(s, "models/fbxtest/", "fiveseven", "fiveseven");
	return 0;
}
void processNode(aiNode *node, const aiScene *scene, ConvertScene& outs)
{
	// process all the node's meshes (if any)
	std::cout << node->mName.C_Str() << std::endl;
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
		std::cout << mesh->mName.C_Str() << std::endl;
		outs._meshes.push_back(processMesh(mesh, scene, outs));
	}
	// then do the same for each of its children
	for (unsigned int i = 0; i < node->mNumChildren; i++)
	{
		processNode(node->mChildren[i], scene, outs);
	}
}
template <typename T,typename S>
void copyVec(S& dst,const T& src)
{
	dst.x = src.x;
	dst.y = src.y;
	dst.z = src.z;
}
Mesh* processMesh(aiMesh *mesh, const aiScene *scene, ConvertScene& outs)
{
	auto ret = new Mesh();
	ret->name = mesh->mName.C_Str();

	ret->first = (unsigned int)outs._indices.size();
	ret->vertRStart = (unsigned int)outs._vertices.size();

	for (int i = 0; i < mesh->mNumVertices; i++)
	{
		Vertex v;
		copyVec(v.pos,mesh->mVertices[i]);
		copyVec(v.normal, mesh->mNormals[i]);
		copyVec(v.tangent, mesh->mTangents[i]);
		copyVec(v.bitangent, mesh->mBitangents[i]);
		copyVec(v.texCoords[0], mesh->mTextureCoords[0][i]);

		outs._vertices.push_back(v);
	}

	for (int i = 0; i < mesh->mNumFaces; i++)
	{
		for (int j = 0; j < mesh->mFaces[i].mNumIndices; j++)
		{
			outs._indices.push_back(mesh->mFaces[i].mIndices[j]+ ret->vertRStart);
		}
	}


	ret->vertREnd = (unsigned int)outs._vertices.size() - 1;
	ret->count = (unsigned int)outs._indices.size()- ret->first;

	auto mat = scene->mMaterials[mesh->mMaterialIndex];
	ret->matName=mat->GetName().C_Str();

	return ret;
}

void writeGeometry(FILE* f,ConvertScene *s)
{
	//header
	unsigned int version = 5;
	fwrite_le("H3DG", 4, f);
	fwrite_le(&version, 1, f);

	//joint count
	unsigned int joint_count = 1;
	fwrite_le(&joint_count, 1, f);
	
	//identity matrix
	Matrix4f identity;
	for (unsigned int j = 0; j < 16; ++j)
		fwrite_le<float>(&identity.x[j], 1, f);
	
	//number of streams is 6: pos,normal,tagent,btangent,uv1,uv2
	unsigned int count = 6;
	fwrite_le(&count, 1, f);
	count = (unsigned int)s->_vertices.size();
	fwrite_le(&count, 1, f);

	for (unsigned int i = 0; i < 8; ++i)
	{
		if (i == 4 || i == 5) continue;//no joints,weights

		unsigned char uc;
		short sh;
		unsigned int streamElemSize;

		switch (i)
		{
		case 0:		// Position
			fwrite_le(&i, 1, f);
			streamElemSize = 3 * sizeof(float); fwrite_le(&streamElemSize, 1, f);
			for (unsigned int j = 0; j < count; ++j)
			{
				fwrite_le<float>(&s->_vertices[j].pos.x, 1, f);
				fwrite_le<float>(&s->_vertices[j].pos.y, 1, f);
				fwrite_le<float>(&s->_vertices[j].pos.z, 1, f);
			}
			break;
		case 1:		// Normal
			fwrite_le(&i, 1, f);
			streamElemSize = 3 * sizeof(short); fwrite_le(&streamElemSize, 1, f);
			for (unsigned int j = 0; j < count; ++j)
			{
				sh = (short)(s->_vertices[j].normal.x * 32767); fwrite_le<short>(&sh, 1, f);
				sh = (short)(s->_vertices[j].normal.y * 32767); fwrite_le<short>(&sh, 1, f);
				sh = (short)(s->_vertices[j].normal.z * 32767); fwrite_le<short>(&sh, 1, f);
			}
			break;
		case 2:		// Tangent
			fwrite_le(&i, 1, f);
			streamElemSize = 3 * sizeof(short); fwrite_le(&streamElemSize, 1, f);
			for (unsigned int j = 0; j < count; ++j)
			{
				auto tv = s->_vertices[j].tangent.normalized();
				auto vs = s->_vertices[j].tangent;
				/*if (((short)(tv.x * 32767)) == 0 && ((short)(tv.y * 32767))==0 && ((short)(tv.z * 32767))==0)
				{
				printf("%d============ bad error==============\n",j);
				}*/
				sh = (short)(s->_vertices[j].tangent.x * 32767); fwrite_le<short>(&sh, 1, f);
				sh = (short)(s->_vertices[j].tangent.y * 32767); fwrite_le<short>(&sh, 1, f);
				sh = (short)(s->_vertices[j].tangent.z * 32767); fwrite_le<short>(&sh, 1, f);
			}
			break;
		case 3:		// Bitangent
			fwrite_le(&i, 1, f);
			streamElemSize = 3 * sizeof(short); fwrite_le(&streamElemSize, 1, f);
			for (unsigned int j = 0; j < count; ++j)
			{
				sh = (short)(s->_vertices[j].bitangent.x * 32767); fwrite_le<short>(&sh, 1, f);
				sh = (short)(s->_vertices[j].bitangent.y * 32767); fwrite_le<short>(&sh, 1, f);
				sh = (short)(s->_vertices[j].bitangent.z * 32767); fwrite_le<short>(&sh, 1, f);
			}
			break;
		case 4:		// Joint indices

			break;
		case 5:		// Weights

			break;
		case 6:		// Texture Coord Set 1
			fwrite_le(&i, 1, f);
			streamElemSize = 2 * sizeof(float); fwrite_le(&streamElemSize, 1, f);
			for (unsigned int j = 0; j < count; ++j)
			{
				fwrite_le<float>(&s->_vertices[j].texCoords[0].x, 1, f);
				fwrite_le<float>(&s->_vertices[j].texCoords[0].y, 1, f);
			}
			break;
		case 7:		// Texture Coord Set 2
			fwrite_le(&i, 1, f);
			streamElemSize = 2 * sizeof(float); fwrite_le(&streamElemSize, 1, f);
			for (unsigned int j = 0; j < count; ++j)
			{
				fwrite_le<float>(&s->_vertices[j].texCoords[1].x, 1, f);
				fwrite_le<float>(&s->_vertices[j].texCoords[1].y, 1, f);
			}
			break;
		}
	}
	// Write triangle indices
	count = (unsigned int)s->_indices.size();
	fwrite_le(&count, 1, f);

	for (unsigned int i = 0; i < s->_indices.size(); ++i)
	{
		fwrite_le(&s->_indices[i], 1, f);
	}

	// Write morph targets
	count = 0;
	fwrite_le(&count, 1, f);
	
	fclose(f);
}

void writeSGNode(ConvertScene& scene,const std::string &assetPath, const std::string &modelName, std::ofstream &outf)
{
	// Write triangle groups as submeshes of first triangle group
	for (unsigned int i = 0; i < scene._meshes.size(); ++i)
	{
		Vec3f trans, rot, scale;
		scene._meshes[i]->matRel.decompose(trans, rot, scale);
		rot.x = radToDeg(rot.x);
		rot.y = radToDeg(rot.y);
		rot.z = radToDeg(rot.z);
		

		outf << "<Mesh ";
		outf << "name=\"" << (i > 0 ? "#" : "") << scene._meshes[i]->name << "\" ";
		outf << "material=\"";
		outf << assetPath + modelName + scene._meshes[i]->matName + ".material.xml\" ";

		if (i == 0)
		{
			if (trans != Vec3f(0, 0, 0))
				outf << "tx=\"" << trans.x << "\" ty=\"" << trans.y << "\" tz=\"" << trans.z << "\" ";
			if (rot != Vec3f(0, 0, 0))
				outf << "rx=\"" << rot.x << "\" ry=\"" << rot.y << "\" rz=\"" << rot.z << "\" ";
			if (scale != Vec3f(1, 1, 1))
				outf << "sx=\"" << scale.x << "\" sy=\"" << scale.y << "\" sz=\"" << scale.z << "\" ";
		}

		outf << "batchStart=\"";
		outf << scene._meshes[i]->first;
		outf << "\" batchCount=\"";
		outf << scene._meshes[i]->count;
		outf << "\" vertRStart=\"";
		outf << scene._meshes[i]->vertRStart;
		outf << "\" vertREnd=\"";
		outf << scene._meshes[i]->vertREnd;
		outf << "\"";

		if (i == 0 && scene._meshes.size() > 1) outf << ">\n";
		if (i > 0) outf << " />\n";
	}
	outf << "</Mesh>\n";
}

bool writeSceneGraph(ConvertScene& scene,const std::string &assetPath, const std::string &assetName, const std::string &modelName)
{
	std::ofstream outf;
	outf.open((scene._outPath + assetPath + assetName + ".scene.xml").c_str(), std::ios::out);
	if (!outf.good())
	{
		log("Failed to write " + scene._outPath + assetPath + assetName + ".scene file");
		return false;
	}

	outf << "<Model name=\"" << assetName << "\" geometry=\"" << assetPath << assetName << ".geo\"";
	outf << ">\n";
	// Meshes
	writeSGNode(scene, assetPath, modelName, outf);
	
	outf << "</Model>\n";

	outf.close();

	return true;
}

void createDirectories(const std::string &basePath, const std::string &newPath)
{
	if (newPath.empty()) return;

	std::string tmpString;
	tmpString.reserve(256);
	size_t i = 0, len = newPath.length();

	while (++i < len)
	{
		if (newPath[i] == '/' || newPath[i] == '\\' || i == len - 1)
		{
			tmpString = basePath + newPath.substr(0, ++i);
			_mkdir(tmpString.c_str());
		}
	}
}