#include <string>
#include <vector>
#include <iostream>
#include "utMath.h"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"



struct Joint;
using namespace Horde3D;
void log(const std::string &msg)
{
	std::cout<<msg<<std::endl;

#ifdef PLATFORM_WIN
	OutputDebugString(msg.c_str());
	OutputDebugString("\r\n");
#endif
}
struct Vertex
{
	Vec3f  pos;
	Vec3f  normal, tangent, bitangent;
	Vec3f  texCoords[4];
	Joint  *joints[4];
	float  weights[4];
	Vertex()
	{
		joints[0] = 0x0; joints[1] = 0x0; joints[2] = 0x0; joints[3] = 0x0;
		weights[0] = 1; weights[1] = 0; weights[2] = 0; weights[3] = 0;
	}
};
struct TriGroup
{
	unsigned int first, count;
	unsigned int vertRStart, vertREnd;
	std::string matName;

	unsigned int numPosIndices;
	std::vector<unsigned int> *posIndexToVertices;

	TriGroup() : posIndexToVertices(0x0)
	{
	}

	~TriGroup() { delete[] posIndexToVertices; }
};
struct SceneNode
{
	bool                        typeJoint;
	std::string                 name;
	Matrix4f                    matRel, matAbs;
	aiNode *assimpNode;
	aiScene *assimpScene;
	SceneNode                   *parent;
	std::vector< SceneNode * >  children;

	// Animation
	std::vector<Matrix4f>     frames;  // Relative transformation for every frame

	SceneNode()
	{
		assimpNode = nullptr;
		assimpScene = nullptr;
		parent = nullptr;
	}

	virtual ~SceneNode()
	{
		for (unsigned int i = 0; i < children.size(); ++i) delete children[i];
	}
};


struct Mesh : public SceneNode
{
	unsigned int first, count;
	unsigned int vertRStart, vertREnd;
	std::string matName;

	Mesh()
	{
		typeJoint = false;
		parent = 0x0;
	}
	~Mesh() { }
};


struct Joint : public SceneNode
{
	unsigned int  index;
	//Matrix4f      invBindMat;
	bool          used;

	// Temporary
	//Matrix4f      daeInvBindMat;

	Joint()
	{
		typeJoint = true;
		used = false;
	}
};

struct ConvertScene
{
	aiScene              *assimpScene;

	std::vector<Vertex>        _vertices;
	std::vector<unsigned int>  _indices;
	std::vector<Mesh*>        _meshes;
	std::vector<Joint *>       _joints;
	//std::vector<MorphTarget>   _morphTargets;
	std::vector<SceneNode*>    _nodes;
	std::string                  _outPath;
	float                        _lodDist1, _lodDist2, _lodDist3, _lodDist4;
	unsigned int                 _frameCount;
	unsigned int                 _maxLodLevel;
	bool                         _animNotSampled;
};