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

#include <string.h> // memset
#include <vector>
#include "bsp.h"



float math_epsilon = 0.000001f;
struct Vec3f
{
	float x;
	float y;
	float z;
	Vec3f(float _x, float _y, float _z) :x(_x), y(_y), z(_z) {}
	Vec3f()
	{
		x = y = z = 0;
	}
	// -----------
	// Comparisons
	// -----------
	bool operator==(const Vec3f &v) const
	{
		return (x > v.x - math_epsilon && x < v.x + math_epsilon &&
			y > v.y - math_epsilon && y < v.y + math_epsilon &&
			z > v.z - math_epsilon && z < v.z + math_epsilon);
	}

	bool operator!=(const Vec3f &v) const
	{
		return (x < v.x - math_epsilon || x > v.x + math_epsilon ||
			y < v.y - math_epsilon || y > v.y + math_epsilon ||
			z < v.z - math_epsilon || z > v.z + math_epsilon);
	}
	// ---------------------
	// Arithmetic operations
	// ---------------------
	Vec3f operator-() const
	{
		return Vec3f(-x, -y, -z);
	}

	Vec3f operator+(const Vec3f &v) const
	{
		return Vec3f(x + v.x, y + v.y, z + v.z);
	}

	Vec3f &operator+=(const Vec3f &v)
	{
		return *this = *this + v;
	}

	Vec3f operator-(const Vec3f &v) const
	{
		return Vec3f(x - v.x, y - v.y, z - v.z);
	}

	Vec3f &operator-=(const Vec3f &v)
	{
		return *this = *this - v;
	}

	Vec3f operator*(const float f) const
	{
		return Vec3f(x * f, y * f, z * f);
	}

	Vec3f &operator*=(const float f)
	{
		return *this = *this * f;
	}

	Vec3f operator/(const float f) const
	{
		return Vec3f(x / f, y / f, z / f);
	}

	Vec3f &operator/=(const float f)
	{
		return *this = *this / f;
	}

	// ----------------
	// Special products
	// ----------------
	float dot(const Vec3f &v) const
	{
		return x * v.x + y * v.y + z * v.z;
	}

	Vec3f cross(const Vec3f &v) const
	{
		return Vec3f(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x);
	}

	// ----------------
	// Other operations
	// ----------------
	float length() const
	{
		return sqrtf(x * x + y * y + z * z);
	}

	Vec3f normalized() const
	{
		float invLen = 1.0f / length();
		return Vec3f(x * invLen, y * invLen, z * invLen);
	}

	void normalize()
	{
		float invLen = 1.0f / length();
		x *= invLen;
		y *= invLen;
		z *= invLen;
	}
};

struct Vertex
{
	Vec3f  storedPos, pos;
	Vec3f  storedNormal, normal, tangent, bitangent;
	Vec3f  texCoords[4];
	int    index;
	Vertex()
	{

	}
};


struct TriGroup
{
	unsigned int  first, count;
	unsigned int  vertRStart, vertREnd;
	std::string   matName;

	//unsigned int                 numPosIndices;
	//std::vector< unsigned int >  *posIndexToVertices;

	TriGroup() //: posIndexToVertices(0x0)
	{
	}

	~TriGroup() { /*delete[] posIndexToVertices;*/ }
};


struct SceneNode
{
	bool                        typeJoint;
	char                        name[256];
	//Matrix4f                    matRel, matAbs;
	//DaeNode                     *daeNode;
	//DaeInstance                 *daeInstance;
	SceneNode                   *parent;
	std::vector< SceneNode * >  children;

	

	SceneNode()
	{
		memset(name, 0, sizeof(name));
		//daeNode = 0x0;
		//daeInstance = 0x0;
		parent = 0x0;
	}

	virtual ~SceneNode()
	{
		for (unsigned int i = 0; i < children.size(); ++i) delete children[i];
	}
};


struct Mesh : public SceneNode
{
	std::vector< TriGroup* > triGroups;
	unsigned int             lodLevel;

	Mesh()
	{
		typeJoint = false;
		parent = 0x0;
		lodLevel = 0;
	}

	~Mesh() { for (int i = triGroups.size(); i>0; ) delete triGroups[--i]; }

};

/*
struct Joint : public SceneNode
{
	unsigned int  index;
	Matrix4f      invBindMat;
	bool          used;

	// Temporary
	Matrix4f      daeInvBindMat;

	Joint()
	{
		typeJoint = true;
		used = false;
	}
};


struct MorphDiff
{
	unsigned int  vertIndex;
	Vec3f         posDiff;
	Vec3f         normDiff, tanDiff, bitanDiff;
};


struct MorphTarget
{
	char                      name[256];
	std::vector< MorphDiff >  diffs;

	MorphTarget()
	{
		memset(name, 0, sizeof(name));
	}
};*/


class Converter
{
public:
	Converter(bsp::Map &map, const std::string &outPath, float *lodDists);
	~Converter();

	bool convertModel(bool optimize);

	bool writeModel(const std::string &assetPath, const std::string &assetName, const std::string &modelName) const;
	bool writeMaterials(const std::string &assetPath, const std::string &modelName, bool replace) const;
	bool hasAnimation() const;
	bool writeAnimation(const std::string &assetPath, const std::string &assetName) const;

private:
	//Matrix4f getNodeTransform(DaeNode &node, unsigned int frame);
	//SceneNode *findNode(const char *name, SceneNode *ignoredNode);
	//void checkNodeName(SceneNode *node);
	//bool validateInstance(const std::string &instanceId) const;
	//SceneNode *processNode(DaeNode &node, SceneNode *parentNode,Matrix4f transAccum, std::vector< Matrix4f > animTransAccum);
	void calcTangentSpaceBasis(std::vector<Vertex> &vertices) const;
	//void processJoints();
	void processMeshes(bool optimize);
	bool writeGeometry(const std::string &assetPath, const std::string &assetName) const;
	//void writeSGNode(const std::string &assetPath, const std::string &modelName, SceneNode *node, unsigned int depth, std::ofstream &outf) const;
	bool writeSceneGraph(const std::string &assetPath, const std::string &assetName, const std::string &modelName) const;
	//void writeAnimFrames(SceneNode &node, FILE *f) const;

private:
	bsp::Map              &_bspmap;

	std::vector<Vertex>        _vertices;
	std::vector< unsigned int >  _indices;
	std::vector<Mesh*>        _meshes;
	//std::vector<Joint*>       _joints;
	//std::vector<MorphTarget>   _morphTargets;
	//std::vector<SceneNode*>    _nodes;

	std::string                  _outPath;
	float                        _lodDist1, _lodDist2, _lodDist3, _lodDist4;
	unsigned int                 _frameCount;
	unsigned int                 _maxLodLevel;
	bool                         _animNotSampled;
};

#endif // _converter_H_
