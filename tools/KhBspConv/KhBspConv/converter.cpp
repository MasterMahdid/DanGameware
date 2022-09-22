// *************************************************************************************************
//
// Horde3D
//   Next-Generation Graphics Engine
// --------------------------------------
// Copyright (C) 2006-2016 Nicolas Schulz and Horde3D team
//
// This software is distributed under the terms of the Eclipse Public License v1.0.
// A copy of the license may be obtained at: http://www.eclipse.org/legal/epl-v10.html
//
// *************************************************************************************************

#if defined( _MSC_VER )
#	if _MSC_VER >= 1400
#		define _CRT_SECURE_NO_DEPRECATE
#	endif
#endif

#include "converter.h"
#include "optimizer.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>

using namespace std;


template<class T>
inline char* elemcpy_le(T* dest, const T* src, size_t num_elems)
{
	memcpy(dest, src, num_elems * sizeof(*src));
	return (char*)(src)+num_elems * sizeof(*src);
}
// little endian element writer
template<class T>
inline void fwrite_le(const T* data, size_t count, FILE* f)
{
	char buffer[256];
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

Converter::Converter(khbsp::CQ3LevelMesh	*bspmesh, const std::string &outPath, const std::string& baseq3) :_bspmesh(bspmesh), _outPath(outPath), _baseq3(baseq3)
{
	
}
Converter::~Converter() {}



void Converter::calcTangentSpaceBasis(vector<Vertex> &verts) const
{
	for (unsigned int i = 0; i < verts.size(); ++i)
	{
		verts[i].normal = Vec3f(0, 0, 0);
		verts[i].tangent = Vec3f(0, 0, 0);
		verts[i].bitangent = Vec3f(0, 0, 0);
	}

	// Basic algorithm: Eric Lengyel, Mathematics for 3D Game Programming & Computer Graphics
	for (unsigned int i = 0; i < _meshes.size(); ++i)
	{
		for (unsigned int j = 0; j < _meshes[i]->triGroups.size(); ++j)
		{
			TriGroup *triGroup = _meshes[i]->triGroups[j];

			for (unsigned int k = triGroup->first; k < triGroup->first + triGroup->count; k += 3)
			{
				// Compute basis vectors for triangle
				Vec3f edge1uv = verts[_indices[k + 1]].texCoords[0] - verts[_indices[k]].texCoords[0];
				Vec3f edge2uv = verts[_indices[k + 2]].texCoords[0] - verts[_indices[k]].texCoords[0];
				Vec3f edge1 = verts[_indices[k + 1]].pos - verts[_indices[k]].pos;
				Vec3f edge2 = verts[_indices[k + 2]].pos - verts[_indices[k]].pos;
				Vec3f normal = edge1.cross(edge2);  // Normal weighted by triangle size (hence unnormalized)

				float r = 1.0f / (edge1uv.x * edge2uv.y - edge2uv.x * edge1uv.y); // UV area normalization
				Vec3f uDir = (edge1 * edge2uv.y - edge2 * edge1uv.y) * r;
				Vec3f vDir = (edge2 * edge1uv.x - edge1 * edge2uv.x) * r;

				// Accumulate basis for vertices
				for (unsigned int l = 0; l < 3; ++l)
				{
					verts[_indices[k + l]].normal += normal;
					verts[_indices[k + l]].tangent += uDir;
					verts[_indices[k + l]].bitangent += vDir;

					// Handle texture seams where vertices were split
					for (auto m : triGroup->vertIndexes)
					{
						if (m != _indices[k + l] && verts[m].storedNormal == verts[_indices[k + l]].storedNormal)
						{
							verts[m].normal += normal;
							//verts[m].tangent += uDir;
							//verts[m].bitangent += vDir;
						}
					}
				}
			}
		}
	}

	// Normalize tangent space basis
	unsigned int numInvalidBasis = 0;
	for (unsigned int i = 0; i < verts.size(); ++i)
	{
		// Check if tangent space basis is invalid
		if (verts[i].normal.length() == 0 || verts[i].tangent.length() == 0 || verts[i].bitangent.length() == 0)
			++numInvalidBasis;

		// Gram-Schmidt orthogonalization
		verts[i].normal.normalize();
		Vec3f &n = verts[i].normal;
		Vec3f &t = verts[i].tangent;
		verts[i].tangent = (t - n * n.dot(t)).normalized();

		// Calculate handedness (required to support mirroring) and final bitangent
		float handedness = n.cross(t).dot(verts[i].bitangent) < 0 ? -1.0f : 1.0f;
		verts[i].bitangent = n.cross(t) * handedness;
	}

	if (numInvalidBasis > 0)
	{
		printf("Warning: Geometry has zero-length basis vectors");
		printf("Maybe two faces point in opposite directions and share same vertices");
	}
}
void fill_hord_vert_from_irr(Vertex& v, const irr::video::S3DVertex2TCoords& s)
{
	v.storedPos = s.Pos;
	v.storedPos.x *= -1;
	v.pos = v.storedPos;
	auto uv1 = s.TCoords;
	auto uv2 = s.TCoords2;
	v.texCoords[0] = Vec3f(uv1.X, uv1.Y, 0);
	v.texCoords[1] = Vec3f(uv2.X, uv2.Y, 0);
	v.storedNormal = s.Normal;
	if (s.Normal.getLength() == 0)
	{
		printf("zero length normal");
	}
}
void Converter::processMeshes(bool optimize)
{
	_meshes.push_back(new Mesh());

	int of = 0;
	auto m1 = _bspmesh->getMesh(0);
	for (int j = 0; j < m1->buffers.size(); j++)
	{
		auto iTriGroup = m1->buffers[j];
		TriGroup* oTriGroup = new TriGroup();
		auto lightmap_id = iTriGroup->lightmapID;

		std::string name = iTriGroup->texture;
		name = name.substr(strlen("textures/"));
		oTriGroup->matName = name;



		oTriGroup->first = (unsigned int)_indices.size();
		oTriGroup->count = (unsigned int)iTriGroup->Indices.size();
		oTriGroup->vertRStart = (unsigned int)_vertices.size();
		for (int az = 0; az < iTriGroup->Vertices.size(); az++)
		{
			Vertex v;
			fill_hord_vert_from_irr(v, iTriGroup->Vertices[az]);
			_vertices.push_back(v);
			oTriGroup->vertIndexes.push_back((unsigned int)_vertices.size() - 1);
		}
		for (int az = 0; az < iTriGroup->Indices.size(); az++)
		{
			_indices.push_back(iTriGroup->Indices[iTriGroup->Indices.size()-az-1] + of);
		}
		of += iTriGroup->Vertices.size();
		oTriGroup->vertREnd = (unsigned int)_vertices.size() - 1;
		unsigned int numDegTris = MeshOptimizer::removeDegeneratedTriangles(oTriGroup, _vertices, _indices);
		_meshes[0]->triGroups.push_back(oTriGroup);
	}
	calcTangentSpaceBasis(_vertices);

	// Optimization and clean up
	float optEffBefore = 0, optEffAfter = 0;
	unsigned int optNumCalls = 0;
	for (unsigned int i = 0; i < _meshes.size(); ++i)
	{
		for (unsigned int j = 0; j < _meshes[i]->triGroups.size(); ++j)
		{
			// Optimize order of indices for best vertex cache usage and remap vertices
			if (optimize)
			{
				map< unsigned int, unsigned int > vertMap;

				++optNumCalls;
				optEffBefore += MeshOptimizer::calcCacheEfficiency(_meshes[i]->triGroups[j], _indices);
				MeshOptimizer::optimizeIndexOrder(_meshes[i]->triGroups[j], _vertices, _indices, vertMap);
				optEffAfter += MeshOptimizer::calcCacheEfficiency(_meshes[i]->triGroups[j], _indices);
			}
		}
	}


}


bool Converter::writeGeometry(const string &assetPath, const string &assetName) const
{
	string fileName = _outPath + assetPath + assetName + ".geo";
	FILE *f = fopen(fileName.c_str(), "wb");
	if (f == 0x0)
	{
		printf("Failed to write %s ", fileName.c_str());
		return false;
	}

	// Write header
	unsigned int version = 5;
	fwrite_le("H3DG", 4, f);
	fwrite_le(&version, 1, f);

	// Write joints
	unsigned int count = 1;
	fwrite_le(&count, 1, f);

	// Write default identity matrix
	for (unsigned int j = 0; j < 16; ++j)
		fwrite_le<float>(&Matrix4f().x[j], 1, f);

	// Write vertex stream data
	count = 6;
	fwrite_le(&count, 1, f);
	count = (unsigned int)_vertices.size();
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
				fwrite_le<float>(&_vertices[j].pos.x, 1, f);
				fwrite_le<float>(&_vertices[j].pos.y, 1, f);
				fwrite_le<float>(&_vertices[j].pos.z, 1, f);
			}
			break;
		case 1:		// Normal
			fwrite_le(&i, 1, f);
			streamElemSize = 3 * sizeof(short); fwrite_le(&streamElemSize, 1, f);
			for (unsigned int j = 0; j < count; ++j)
			{
				sh = (short)(_vertices[j].normal.x * 32767); fwrite_le<short>(&sh, 1, f);
				sh = (short)(_vertices[j].normal.y * 32767); fwrite_le<short>(&sh, 1, f);
				sh = (short)(_vertices[j].normal.z * 32767); fwrite_le<short>(&sh, 1, f);
			}
			break;
		case 2:		// Tangent
			fwrite_le(&i, 1, f);
			streamElemSize = 3 * sizeof(short); fwrite_le(&streamElemSize, 1, f);
			for (unsigned int j = 0; j < count; ++j)
			{
				auto tv = _vertices[j].tangent.normalized();
				auto vs = _vertices[j].tangent;
				if (((short)(tv.x * 32767)) == 0 && ((short)(tv.y * 32767))==0 && ((short)(tv.z * 32767))==0)
				{
					printf("%d============ bad error==============\n",j);
				}
				sh = (short)(_vertices[j].tangent.x * 32767); fwrite_le<short>(&sh, 1, f);
				sh = (short)(_vertices[j].tangent.y * 32767); fwrite_le<short>(&sh, 1, f);
				sh = (short)(_vertices[j].tangent.z * 32767); fwrite_le<short>(&sh, 1, f);
			}
			break;
		case 3:		// Bitangent
			fwrite_le(&i, 1, f);
			streamElemSize = 3 * sizeof(short); fwrite_le(&streamElemSize, 1, f);
			for (unsigned int j = 0; j < count; ++j)
			{
				sh = (short)(_vertices[j].bitangent.x * 32767); fwrite_le<short>(&sh, 1, f);
				sh = (short)(_vertices[j].bitangent.y * 32767); fwrite_le<short>(&sh, 1, f);
				sh = (short)(_vertices[j].bitangent.z * 32767); fwrite_le<short>(&sh, 1, f);
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
				fwrite_le<float>(&_vertices[j].texCoords[0].x, 1, f);
				fwrite_le<float>(&_vertices[j].texCoords[0].y, 1, f);
			}
			break;
		case 7:		// Texture Coord Set 2
			fwrite_le(&i, 1, f);
			streamElemSize = 2 * sizeof(float); fwrite_le(&streamElemSize, 1, f);
			for (unsigned int j = 0; j < count; ++j)
			{
				fwrite_le<float>(&_vertices[j].texCoords[1].x, 1, f);
				fwrite_le<float>(&_vertices[j].texCoords[1].y, 1, f);
			}
			break;
		}
	}

	// Write triangle indices
	count = (unsigned int)_indices.size();
	fwrite_le(&count, 1, f);

	for (unsigned int i = 0; i < _indices.size(); ++i)
	{
		fwrite_le(&_indices[i], 1, f);
	}

	// Write morph targets
	count = 0;
	fwrite_le(&count, 1, f);


	fclose(f);

	return true;
}



const char* mat_out_folder = "models/bsptextures/materials/";
std::string getxmlMatPath(std::string matname, std::string outpath)
{
	std::string res = "materials/"+ matname;
	res = res+ "/mat.material.xml";
	return res;

}
inline bool exists_test1(const std::string& name) {
	if (FILE *file = fopen(name.c_str(), "r")) {
		fclose(file);
		return true;
	}
	else {
		return false;
	}
}
bool Converter::writeModel(const std::string &assetPath, const std::string &assetName,std::string entity_xml) const
{
	bool result = true;
	if (!writeGeometry(assetPath, assetName)) result = false;

	auto xml_path = _outPath + assetPath + assetName + ".scene.xml";
	auto f = fopen(xml_path.c_str(), "w");
	char buffer[4096];
	int len;
	sprintf(buffer, "<Model name=\"%s\" geometry=\"%s%s.geo\">\n",assetName.c_str(),assetPath.c_str(),assetName.c_str());
	fwrite(buffer, 1, strlen(buffer), f);
	for (const auto& tg : _meshes[0]->triGroups)
	{
		auto mat_xml_local_path = getxmlMatPath(tg->matName, _outPath);
		//TODO: remove bspconv below
		sprintf(buffer, "<Mesh name=\"bspconv\" material=\"%s\" batchStart=\"%d\" batchCount=\"%d\" vertRStart=\"%d\" vertREnd=\"%d\"  />\n", mat_xml_local_path.c_str(), tg->first, tg->count, tg->vertRStart, tg->vertREnd);
		fwrite(buffer, 1, strlen(buffer), f);
	}
	sprintf(buffer, "</Model>\n");
	fwrite(buffer, 1, strlen(buffer), f);
	sprintf(buffer, entity_xml.c_str());
	fwrite(buffer, 1, strlen(buffer), f);
	fclose(f);
	return result;
}