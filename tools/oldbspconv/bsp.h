#pragma once
#include <string>
#include <vector>
namespace bsp
{
	using s32 = __int32;
	using u32 = unsigned __int32;
	using u8 = unsigned char;
	using f32 = __int32;
	using c8 = char;
	enum eLumps
	{
		kEntities = 0,	// Stores player/object positions, etc...
		kShaders = 1,	// Stores texture information
		kPlanes = 2,	// Stores the splitting planes
		kNodes = 3,	// Stores the BSP nodes
		kLeafs = 4,	// Stores the leafs of the nodes
		kLeafFaces = 5,	// Stores the leaf's indices into the faces
		kLeafBrushes = 6,	// Stores the leaf's indices into the brushes
		kModels = 7,	// Stores the info of world models
		kBrushes = 8,	// Stores the brushes info (for collision)
		kBrushSides = 9,	// Stores the brush surfaces info
		kVertices = 10,	// Stores the level vertices
		kMeshVerts = 11,	// Stores the model vertices offsets
		kFogs = 12,	// Stores the shader files (blending, anims..)
		kFaces = 13,	// Stores the faces for the level
		kLightmaps = 14,	// Stores the lightmaps for the level
		kLightGrid = 15,	// Stores extra world lighting information
		kVisData = 16,	// Stores PVS and cluster info (visibility)
		kLightArray = 17,	// RBSP
		kMaxLumps				// A constant to store the number of lumps
	};
	struct tBSPHeader
	{
		s32 strID;		// This should always be 'IBSP'
		s32 version;	// This should be 0x2e for Quake 3 files
	};
	struct tBSPLump
	{
		s32 offset;
		s32 length;
	};
	struct tBSPVertex
	{
		f32 vPosition[3];      // (x, y, z) position.
		f32 vTextureCoord[2];  // (u, v) texture coordinate
		f32 vLightmapCoord[2]; // (u, v) lightmap coordinate
		f32 vNormal[3];        // (x, y, z) normal vector
		u8 color[4];           // RGBA color for the vertex
	};
	struct tBSPPlane
	{
		f32 normal[3];
		f32 dist;
	};
	struct tBSPBrush
	{
		s32 brushside;
		s32 n_brushsides;
		s32 texture;
	};
	struct tBSPBrushside
	{
		s32 plane;
		s32 texture;
	};
	struct tBSPFace
	{
		s32 textureID;        // The index into the texture array
		s32 fogNum;           // The index for the effects (or -1 = n/a)
		s32 type;             // 1=polygon, 2=patch, 3=mesh, 4=billboard
		s32 vertexIndex;      // The index into this face's first vertex
		s32 numOfVerts;       // The number of vertices for this face
		s32 meshVertIndex;    // The index into the first meshvertex
		s32 numMeshVerts;     // The number of mesh vertices
		s32 lightmapID;       // The texture index for the lightmap
		s32 lMapCorner[2];    // The face's lightmap corner in the image
		s32 lMapSize[2];      // The size of the lightmap section
		f32 lMapPos[3];     // The 3D origin of lightmap.
		f32 lMapBitsets[2][3]; // The 3D space for s and t unit vectors.
		f32 vNormal[3];     // The face normal.
		s32 size[2];          // The bezier patch dimensions.
	};
	struct tBSPModel
	{
		f32 min[3];           // The min position for the bounding box
		f32 max[3];           // The max position for the bounding box.
		s32 faceIndex;          // The first face index in the model
		s32 numOfFaces;         // The number of faces in the model
		s32 brushIndex;         // The first brush index in the model
		s32 numOfBrushes;       // The number brushes for the model
	};
	struct tBSPTexture
	{
		c8 strName[64];   // The name of the texture w/o the extension
		u32 flags;          // The surface flags (unknown)
		u32 contents;       // The content flags (unknown)
	};
	struct Vec3f
	{
		float x;
		float y;
		float z;
		Vec3f() :x(0), y(0), z(0) {};
		Vec3f operator-() const
		{
			return Vec3f(-x, -y, -z);
		}
		Vec3f(const float x, const float y, const float z) : x(x), y(y), z(z)
		{
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
	struct OutputVertex
	{
		Vec3f pos;
		Vec3f normal, tangent, bitangent;
		Vec3f texCoords[2];
	};
	class Map
	{
	public:
		Map() {};
		bool load(const char* filename);
		void writeGeo(const char* filename)const;
		void buildMesh();
	private:
		std::string entities;
		std::vector<tBSPTexture> textures;
		std::vector<tBSPPlane> planes;
		std::vector<tBSPVertex> vertices;
		std::vector<tBSPFace> faces;
		std::vector<tBSPModel> models;
		std::vector<s32> meshVerts;
		std::vector<OutputVertex> outVerts;
		void cleanfaces();
		
	private:
		void loadEntities(tBSPLump* l, FILE* file);
		
		void generateOutputVertices();
		void calcTangentSpaceBasis();
	};
}