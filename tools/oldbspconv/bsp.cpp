#include <stdio.h>
#include <algorithm>
#include "bsp.h"

namespace bsp
{
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
	template <typename T>
	void loadObjects(std::vector<T>& dest, tBSPLump* l, FILE* file)
	{
		auto object_count = l->length / sizeof(T);
		if (object_count == 0)
			return;
		T* arr = (T*)malloc(l->length);
		fseek(file, l->offset, 0);
		fread(arr, 1, l->length, file);
		for (u32 i = 0; i < object_count; i++)
		{
			dest.push_back(arr[i]);
		}
		free(arr);
	}
	bool Map::load(const char* fn)
	{
		auto file = fopen(fn, "r");
		if (file == NULL)
		{
			printf("could not open file");
			return false;
		}
		tBSPHeader header;
		fread(&header, sizeof(tBSPHeader), 1, file);
		if ((header.strID != 0x50534249 ||(header.version != 0x2e && header.version != 0x2f))&&	(header.strID != 0x50534252 || header.version != 1))
		{
			printf("Could not load .bsp file, unknown header.");
			return false;
		}
		tBSPLump Lumps[kMaxLumps];
		fread(&Lumps[0], sizeof(tBSPLump), kMaxLumps, file);
		loadEntities(&Lumps[kEntities], file);
		loadObjects(textures,&Lumps[kShaders], file);
		loadObjects(vertices, &Lumps[kVertices], file);
		auto sz = sizeof(tBSPFace);
		loadObjects(faces, &Lumps[kFaces], file);
		loadObjects(models, &Lumps[kModels], file);
		loadObjects(meshVerts, &Lumps[kMeshVerts], file);
		loadObjects(planes, &Lumps[kPlanes], file);
		
		fclose(file);
		cleanfaces();
		//generateOutputVertices();
		//calcTangentSpaceBasis();
		return true;
	}
	void Map::loadEntities(tBSPLump* l, FILE* file)
	{
		char* s = (char*)malloc(l->length);
		fseek(file, l->offset, 0);
		fread(s, 1, l->length, file);
		entities = s;
		free(s);
	}
	void Map::cleanfaces()
	{
	}
	void Map::buildMesh()
	{
		s32 last_lightmap_id = 0;
		s32 last_texture_id = 0;
		int ind = 0;
		struct submesh
		{
			s32 lightmap_id;
			s32 texture_id;
			s32 batchStart;
			s32 batchCount;
			s32 vertRStart;
			s32 vertREnd;
		};
		std::vector<submesh> sub_meshes;
		for (const auto& face : this->faces)
		{
			if (face.type != 1)
				continue;

			
			if (ind == 0 || face.lightmapID != last_lightmap_id || face.textureID != last_texture_id)
			{
				submesh new_sub;
				new_sub.lightmap_id = face.lightmapID;
				new_sub.texture_id = face.textureID;
				new_sub.batchStart = face.meshVertIndex;
				new_sub.batchCount = face.numOfVerts;
				
				for (int i = 0; i < face.numOfVerts; i++)
				{
					auto v_ind = this->meshVerts[face.meshVertIndex + i];
					if (i == 0 || new_sub.vertRStart > v_ind)
						new_sub.vertRStart = v_ind;
					if (i == 0 || new_sub.vertREnd < v_ind)
						new_sub.vertREnd = v_ind;
				}
				sub_meshes.push_back(new_sub);
				last_lightmap_id = face.lightmapID;
				last_texture_id = face.textureID;
				
			}
			else
			{
				auto & sb = sub_meshes[sub_meshes.size() - 1];
				sb.batchCount += face.numOfVerts;
				for (int i = 0; i < face.numOfVerts; i++)
				{
					auto v_ind = this->meshVerts[face.meshVertIndex + i];
					if (i == 0 || sb.vertRStart > v_ind)
						sb.vertRStart = v_ind;
					if (i == 0 || sb.vertREnd < v_ind)
						sb.vertREnd = v_ind;
				}
			}
			ind++;
		}
		return;
	}
	void Map::calcTangentSpaceBasis()
	{
		auto& verts = this->outVerts;
		// Basic algorithm: Eric Lengyel, Mathematics for 3D Game Programming & Computer Graphics
		for (int bb = 0; bb < this->models.size(); bb++)
		{
			for (int cc = 0; cc < this->models[bb].numOfFaces;cc++)
			{
				int k = this->models[bb].faceIndex + cc;
				if (k >= this->faces.size())
					break;
				Vec3f edge1uv = verts[this->meshVerts[faces[k].meshVertIndex + 1]].texCoords[0] - verts[this->meshVerts[faces[k].meshVertIndex]].texCoords[0];
				Vec3f edge2uv = verts[this->meshVerts[faces[k].meshVertIndex + 2]].texCoords[0] - verts[this->meshVerts[faces[k].meshVertIndex]].texCoords[0];
				Vec3f edge1 = verts[this->meshVerts[faces[k].meshVertIndex + 1]].pos - verts[this->meshVerts[faces[k].meshVertIndex]].pos;
				Vec3f edge2 = verts[this->meshVerts[faces[k].meshVertIndex + 2]].pos - verts[this->meshVerts[faces[k].meshVertIndex]].pos;
				Vec3f normal = edge1.cross(edge2);  // Normal weighted by triangle size (hence unnormalized)

				float r = 1.0f / (edge1uv.x * edge2uv.y - edge2uv.x * edge1uv.y); // UV area normalization
				Vec3f uDir = (edge1 * edge2uv.y - edge2 * edge1uv.y) * r;
				Vec3f vDir = (edge2 * edge1uv.x - edge1 * edge2uv.x) * r;

				// Accumulate basis for vertices
				for (unsigned int l = 0; l < 3; ++l)
				{
					verts[this->meshVerts[faces[k].meshVertIndex + l]].normal += normal;
					verts[this->meshVerts[faces[k].meshVertIndex + l]].tangent += uDir;
					verts[this->meshVerts[faces[k].meshVertIndex + l]].bitangent += vDir;
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
	
	
	void Map::generateOutputVertices()
	{
		outVerts.clear();
		for (const auto& v : vertices)
		{
			OutputVertex ov;
			ov.pos.x = v.vPosition[0];
			ov.pos.y = v.vPosition[1];
			ov.pos.z = v.vPosition[2];
			ov.normal.x = v.vNormal[0];
			ov.normal.y = v.vNormal[1];
			ov.normal.z = v.vNormal[2];
			ov.texCoords[0].x = v.vTextureCoord[0];
			ov.texCoords[0].y = v.vTextureCoord[1];
			ov.texCoords[1].x = v.vLightmapCoord[0];
			ov.texCoords[1].y = v.vLightmapCoord[1];
			outVerts.push_back(ov);
		}

	}
	void Map::writeGeo(const char* filename)const
	{
		auto f = fopen(filename, "w");
		if (f == NULL)
		{
			printf("Rrror opening file for writing.");
			return;
		}
		//generate out verts
		

		// Write header
		unsigned int version = 5;
		fwrite_le("H3DG", 4, f);
		fwrite_le(&version, 1, f);

		// Write joints
		unsigned int count = 1;
		fwrite_le(&count, 1, f);

		// Write default identity matrix
		{
			union
			{
				float c[4][4];
				float x[16];
			};

			c[0][0] = 1; c[1][0] = 0; c[2][0] = 0; c[3][0] = 0;
			c[0][1] = 0; c[1][1] = 1; c[2][1] = 0; c[3][1] = 0;
			c[0][2] = 0; c[1][2] = 0; c[2][2] = 1; c[3][2] = 0;
			c[0][3] = 0; c[1][3] = 0; c[2][3] = 0; c[3][3] = 1;
			for (unsigned int j = 0; j < 16; ++j)
				fwrite_le<float>(&x[j], 1, f);
		}
		// Write vertex stream data
		count = 6; // Number of streams
		fwrite_le(&count, 1, f);
		count = (unsigned int)this->outVerts.size();
		auto& _vertices = this->outVerts;
		fwrite_le(&count, 1, f);

		for (unsigned int i = 0; i < 8; ++i)
		{
			if (i == 4 || i == 5) continue;
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
		std::vector<unsigned int> final_indicies;
		for (const auto& face : this->faces)
		{
			if (face.type != 1)
				continue;
			if (face.vertexIndex >= vertices.size() || face.vertexIndex<0)
				continue;
			if (face.meshVertIndex + face.numMeshVerts == this->meshVerts.size() || face.meshVertIndex + face.numMeshVerts < 0)
				continue;
			for(int i=0;i<face.numMeshVerts;i++)
			{
				s32 v_ind = face.vertexIndex + this->meshVerts[face.meshVertIndex + i];
				if (v_ind < 0)
					continue;
				final_indicies.push_back(v_ind);
			}
		}
		// Write triangle indices
		count = (unsigned int)final_indicies.size();
		fwrite_le(&count, 1, f);
		s32 vmin=0;
		s32 vmax=0;
		for (unsigned int i = 0; i < final_indicies.size(); ++i)
		{
			auto vind = final_indicies[i];
			if (i == 0 || vind > vmax)
				vmax = vind;
			if (i == 0 || vind < vmin)
				vmin = vind;
			fwrite_le(&(vind), 1, f);
		}
		// Write morph targets
		count = 0;
		fwrite_le(&count, 1, f);
		fclose(f);
		{
			s32 batchStart = 0;
			s32 batchCount = final_indicies.size();
			s32 vertRStart = vmin;
			s32 vertREnd = vmax;
			printf("%d,%d,%d,%d", batchStart, batchCount, vertRStart, vertREnd);
		}
	}
	
}