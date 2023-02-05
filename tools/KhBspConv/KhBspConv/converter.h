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

#ifndef _converter_H_
#define _converter_H_

#include <string>
#include <vector>
#include "utMath.h"
#include <irrlicht.h>
#include "CQ3LevelMesh.h";

using namespace Horde3D;



struct Vertex
{
	Vec3f  storedPos, pos;
	Vec3f  storedNormal, normal, tangent, bitangent;
	Vec3f  texCoords[4];
};
struct TriGroup
{
	unsigned int  first, count;
	unsigned int  vertRStart, vertREnd;
	std::string   matName;
	int lightmap_id;
	int modelNum;
	std::vector<unsigned int> vertIndexes;
};
struct Mesh
{
	std::vector<TriGroup*> triGroups;
	Mesh()
	{
	}
	~Mesh() { for (int i = triGroups.size(); i>0; ) delete triGroups[--i]; }
};

class Converter
{
public:
	Converter(khbsp::CQ3LevelMesh	*bspmesh , const std::string &outPath,const std::string& baseq3);
	~Converter();

	void processMeshes(bool optimize, bool fix_split_seems);
	bool writeModel(const std::string &assetPath, const std::string &assetName,std::string entity_xml) const;

private:
	void calcTangentSpaceBasis(std::vector< Vertex > &vertices, bool fix_split_seems) const;
	bool writeGeometry(const std::string &assetPath, const std::string &assetName) const;

private:
	khbsp::CQ3LevelMesh				*_bspmesh;
	std::vector< Vertex >        _vertices;
	std::vector< unsigned int >  _indices;
	std::vector< Mesh * >        _meshes;
	std::string                  _outPath;
	std::string					 _baseq3;
};

#endif // _converter_H_
