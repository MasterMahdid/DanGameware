// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"
#ifdef _IRR_COMPILE_WITH_BSP_LOADER_

#include "CQ3LevelMesh.h"
#include "ISceneManager.h"
#include "SMeshBufferLightMap.h"
#include "irrString.h"
#include "ILightSceneNode.h"
#include "IQ3Shader.h"
#include "IFileList.h"

//#define TJUNCTION_SOLVER_ROUND
//#define TJUNCTION_SOLVER_0125

namespace khbsp
{
		using namespace irr::scene::quake3;

		//! constructor
		CQ3LevelMesh::CQ3LevelMesh(io::IFileSystem* fs, scene::ISceneManager* smgr,
			const Q3LevelLoadParameter &loadParam)
			: LoadParam(loadParam), Textures(0), NumTextures(0), LightMaps(0), NumLightMaps(0),
			Vertices(0), NumVertices(0), Faces(0), NumFaces(0), Models(0), NumModels(0),
			Planes(0), NumPlanes(0), Nodes(0), NumNodes(0), Leafs(0), NumLeafs(0),
			LeafFaces(0), NumLeafFaces(0), MeshVerts(0), NumMeshVerts(0),
			Brushes(0), NumBrushes(0), BrushEntities(0), FileSystem(fs),
			SceneManager(smgr), FramesPerSecond(25.f)
		{
#ifdef _DEBUG
#endif

			for (s32 i = 0; i != E_Q3_MESH_SIZE; ++i)
			{
				Mesh[i] = 0;
			}

			Driver = smgr ? smgr->getVideoDriver() : 0;
			if (Driver)
				Driver->grab();

			if (FileSystem)
				FileSystem->grab();

			// load default shaders
		}


		//! destructor
		CQ3LevelMesh::~CQ3LevelMesh()
		{
			cleanLoader();

			if (Driver)
				Driver->drop();

			if (FileSystem)
				FileSystem->drop();

			s32 i;

			for (i = 0; i != E_Q3_MESH_SIZE; ++i)
			{
				if (Mesh[i])
				{
					delete Mesh[i];
					Mesh[i] = 0;
				}
			}

			for (i = 1; i < NumModels; i++)
			{
				delete BrushEntities[i];
			}
			delete[] BrushEntities; BrushEntities = 0;

			ReleaseEntity();
		}


		//! loads a level from a .bsp-File. Also tries to load all needed textures. Returns true if successful.
		bool CQ3LevelMesh::loadFile(io::IReadFile* file)
		{
			if (!file)
				return false;

			LevelName = file->getFileName();

			file->read(&header, sizeof(tBSPHeader));

#ifdef __BIG_ENDIAN__
			header.strID = byteswap(header.strID);
			header.version = byteswap(header.version);
#endif

			if ((header.strID != 0x50534249 ||			// IBSP
				(header.version != 0x2e			// quake3
					&& header.version != 0x2f			// rtcw
					)
				)
				&&
				(header.strID != 0x50534252 || header.version != 1) // RBSP, starwars jedi, sof
				)
			{
				return false;
			}

#if 0
			if (header.strID == 0x50534252)	// RBSP Raven
			{
				LoadParam.swapHeader = 1;
			}
#endif

			// now read lumps
			file->read(&Lumps[0], sizeof(tBSPLump)*kMaxLumps);

			s32 i;
			if (LoadParam.swapHeader)
			{
				for (i = 0; i< kMaxLumps; ++i)
				{
					Lumps[i].offset = byteswap(Lumps[i].offset);
					Lumps[i].length = byteswap(Lumps[i].length);
				}
			}

			ReleaseEntity();

			// load everything
			loadEntities(&Lumps[kEntities], file);			// load the entities
			loadTextures(&Lumps[kShaders], file);			// Load the textures
			loadLightmaps(&Lumps[kLightmaps], file);		// Load the lightmaps
			loadVerts(&Lumps[kVertices], file);				// Load the vertices
			loadFaces(&Lumps[kFaces], file);				// Load the faces
			loadPlanes(&Lumps[kPlanes], file);				// Load the Planes of the BSP
			loadNodes(&Lumps[kNodes], file);				// load the Nodes of the BSP
			loadLeafs(&Lumps[kLeafs], file);				// load the Leafs of the BSP
			loadLeafFaces(&Lumps[kLeafFaces], file);		// load the Faces of the Leafs of the BSP
			loadVisData(&Lumps[kVisData], file);			// load the visibility data of the clusters
			loadModels(&Lumps[kModels], file);				// load the models
			loadMeshVerts(&Lumps[kMeshVerts], file);		// load the mesh vertices
			loadBrushes(&Lumps[kBrushes], file);			// load the brushes of the BSP
			loadBrushSides(&Lumps[kBrushSides], file);		// load the brushsides of the BSP
			loadLeafBrushes(&Lumps[kLeafBrushes], file);	// load the brushes of the leaf
			loadFogs(&Lumps[kFogs], file);					// load the fogs

			constructMesh();
			solveTJunction();

			cleanMeshes();
			cleanLoader();

			return true;
		}

		/*!
		*/
		void CQ3LevelMesh::cleanLoader()
		{
			//delete[] Textures; Textures = 0;
			delete[] LightMaps; LightMaps = 0;
			delete[] Vertices; Vertices = 0;
			delete[] Faces; Faces = 0;
			delete[] Models; Models = 0;
			delete[] Planes; Planes = 0;
			delete[] Nodes; Nodes = 0;
			delete[] Leafs; Leafs = 0;
			delete[] LeafFaces; LeafFaces = 0;
			delete[] MeshVerts; MeshVerts = 0;
			delete[] Brushes; Brushes = 0;

			Lightmap.clear();
			Tex.clear();
		}


		//! returns the animated mesh based on a detail level. 0 is the lowest, 255 the highest detail. Note, that some Meshes will ignore the detail level.
		KhMesh* CQ3LevelMesh::getMesh(s32 frameInMs, s32 detailLevel, s32 startFrameLoop, s32 endFrameLoop)
		{
			return Mesh[frameInMs];
		}


		void CQ3LevelMesh::loadTextures(tBSPLump* l, io::IReadFile* file)
		{
			NumTextures = l->length / sizeof(tBSPTexture);
			if (!NumTextures)
				return;
			Textures = new tBSPTexture[NumTextures];

			file->seek(l->offset);
			file->read(Textures, l->length);

			if (LoadParam.swapHeader)
			{
				for (s32 i = 0; i<NumTextures; ++i)
				{
					Textures[i].flags = byteswap(Textures[i].flags);
					Textures[i].contents = byteswap(Textures[i].contents);
					//os::Printer::log("Loaded texture", Textures[i].strName, ELL_INFORMATION);
				}
			}
		}


		void CQ3LevelMesh::loadLightmaps(tBSPLump* l, io::IReadFile* file)
		{
			NumLightMaps = l->length / sizeof(tBSPLightmap);
			if (!NumLightMaps)
				return;
			LightMaps = new tBSPLightmap[NumLightMaps];

			file->seek(l->offset);
			file->read(LightMaps, l->length);
		}

		/*!
		*/
		void CQ3LevelMesh::loadVerts(tBSPLump* l, io::IReadFile* file)
		{
			NumVertices = l->length / sizeof(tBSPVertex);
			if (!NumVertices)
				return;
			Vertices = new tBSPVertex[NumVertices];

			file->seek(l->offset);
			file->read(Vertices, l->length);

			if (LoadParam.swapHeader)
				for (s32 i = 0; i<NumVertices; i++)
				{
					Vertices[i].vPosition[0] = byteswap(Vertices[i].vPosition[0]);
					Vertices[i].vPosition[1] = byteswap(Vertices[i].vPosition[1]);
					Vertices[i].vPosition[2] = byteswap(Vertices[i].vPosition[2]);
					Vertices[i].vTextureCoord[0] = byteswap(Vertices[i].vTextureCoord[0]);
					Vertices[i].vTextureCoord[1] = byteswap(Vertices[i].vTextureCoord[1]);
					Vertices[i].vLightmapCoord[0] = byteswap(Vertices[i].vLightmapCoord[0]);
					Vertices[i].vLightmapCoord[1] = byteswap(Vertices[i].vLightmapCoord[1]);
					Vertices[i].vNormal[0] = byteswap(Vertices[i].vNormal[0]);
					Vertices[i].vNormal[1] = byteswap(Vertices[i].vNormal[1]);
					Vertices[i].vNormal[2] = byteswap(Vertices[i].vNormal[2]);
				}
		}


		/*!
		*/
		void CQ3LevelMesh::loadFaces(tBSPLump* l, io::IReadFile* file)
		{
			NumFaces = l->length / sizeof(tBSPFace);
			if (!NumFaces)
				return;
			Faces = new tBSPFace[NumFaces];

			file->seek(l->offset);
			file->read(Faces, l->length);

			if (LoadParam.swapHeader)
			{
				for (s32 i = 0; i<NumFaces; i++)
				{
					Faces[i].textureID = byteswap(Faces[i].textureID);
					Faces[i].fogNum = byteswap(Faces[i].fogNum);
					Faces[i].type = byteswap(Faces[i].type);
					Faces[i].vertexIndex = byteswap(Faces[i].vertexIndex);
					Faces[i].numOfVerts = byteswap(Faces[i].numOfVerts);
					Faces[i].meshVertIndex = byteswap(Faces[i].meshVertIndex);
					Faces[i].numMeshVerts = byteswap(Faces[i].numMeshVerts);
					Faces[i].lightmapID = byteswap(Faces[i].lightmapID);
					Faces[i].lMapCorner[0] = byteswap(Faces[i].lMapCorner[0]);
					Faces[i].lMapCorner[1] = byteswap(Faces[i].lMapCorner[1]);
					Faces[i].lMapSize[0] = byteswap(Faces[i].lMapSize[0]);
					Faces[i].lMapSize[1] = byteswap(Faces[i].lMapSize[1]);
					Faces[i].lMapPos[0] = byteswap(Faces[i].lMapPos[0]);
					Faces[i].lMapPos[1] = byteswap(Faces[i].lMapPos[1]);
					Faces[i].lMapPos[2] = byteswap(Faces[i].lMapPos[2]);
					Faces[i].lMapBitsets[0][0] = byteswap(Faces[i].lMapBitsets[0][0]);
					Faces[i].lMapBitsets[0][1] = byteswap(Faces[i].lMapBitsets[0][1]);
					Faces[i].lMapBitsets[0][2] = byteswap(Faces[i].lMapBitsets[0][2]);
					Faces[i].lMapBitsets[1][0] = byteswap(Faces[i].lMapBitsets[1][0]);
					Faces[i].lMapBitsets[1][1] = byteswap(Faces[i].lMapBitsets[1][1]);
					Faces[i].lMapBitsets[1][2] = byteswap(Faces[i].lMapBitsets[1][2]);
					Faces[i].vNormal[0] = byteswap(Faces[i].vNormal[0]);
					Faces[i].vNormal[1] = byteswap(Faces[i].vNormal[1]);
					Faces[i].vNormal[2] = byteswap(Faces[i].vNormal[2]);
					Faces[i].size[0] = byteswap(Faces[i].size[0]);
					Faces[i].size[1] = byteswap(Faces[i].size[1]);
				}
			}
		}


		/*!
		*/
		void CQ3LevelMesh::loadPlanes(tBSPLump* l, io::IReadFile* file)
		{
			NumPlanes = l->length / sizeof(tBSPPlane);
			if (!NumPlanes)
				return;
			Planes = new tBSPPlane[NumPlanes];

			file->seek(l->offset);
			file->read(Planes, l->length);

			if (LoadParam.swapHeader)
			{
				for (s32 i = 0; i < NumPlanes; i++)
				{
					Planes[i].d = byteswap(Planes[i].d);
					Planes[i].vNormal[0] = byteswap(Planes[i].vNormal[0]);
					Planes[i].vNormal[1] = byteswap(Planes[i].vNormal[1]);
					Planes[i].vNormal[2] = byteswap(Planes[i].vNormal[2]);

				}
			}


		}


		/*!
		*/
		void CQ3LevelMesh::loadNodes(tBSPLump* l, io::IReadFile* file)
		{
			// ignore
		}


		/*!
		*/
		void CQ3LevelMesh::loadLeafs(tBSPLump* l, io::IReadFile* file)
		{
			// ignore
		}


		/*!
		*/
		void CQ3LevelMesh::loadLeafFaces(tBSPLump* l, io::IReadFile* file)
		{
			// ignore
		}


		/*!
		*/
		void CQ3LevelMesh::loadVisData(tBSPLump* l, io::IReadFile* file)
		{
			// ignore
		}


		/*!
		*/
		void CQ3LevelMesh::loadEntities(tBSPLump* l, io::IReadFile* file)
		{
			core::array<u8> entity;
			entity.set_used(l->length + 2);
			entity[l->length + 1] = 0;

			file->seek(l->offset);
			file->read(entity.pointer(), l->length);

			parser_parse(entity.pointer(), l->length, &CQ3LevelMesh::scriptcallback_entity);
		}


		/*!
		load fog brushes
		*/
		void CQ3LevelMesh::loadFogs(tBSPLump* l, io::IReadFile* file)
		{
			u32 files = l->length / sizeof(tBSPFog);

			file->seek(l->offset);
			tBSPFog fog;
			const IShader *shader;
			STexShader t;
			for (u32 i = 0; i != files; ++i)
			{
				file->read(&fog, sizeof(fog));

				//shader = getShader(fog.shader);
				t.Texture = 0;
				t.ShaderID = shader ? shader->ID : -1;

				FogMap.push_back(t);
			}
		}


		/*!
		load models named in bsp
		*/
		void CQ3LevelMesh::loadModels(tBSPLump* l, io::IReadFile* file)
		{
			NumModels = l->length / sizeof(tBSPModel);
			Models = new tBSPModel[NumModels];

			file->seek(l->offset);
			file->read(Models, l->length);

			if (LoadParam.swapHeader)
			{
				for (s32 i = 0; i < NumModels; i++)
				{
					Models[i].min[0] = byteswap(Models[i].min[0]);
					Models[i].min[1] = byteswap(Models[i].min[1]);
					Models[i].min[2] = byteswap(Models[i].min[2]);
					Models[i].max[0] = byteswap(Models[i].max[0]);
					Models[i].max[1] = byteswap(Models[i].max[1]);
					Models[i].max[2] = byteswap(Models[i].max[2]);

					Models[i].faceIndex = byteswap(Models[i].faceIndex);
					Models[i].numOfFaces = byteswap(Models[i].numOfFaces);
					Models[i].brushIndex = byteswap(Models[i].brushIndex);
					Models[i].numOfBrushes = byteswap(Models[i].numOfBrushes);
				}
			}

			BrushEntities = new KhMesh*[NumModels];
		}

		/*!
		*/
		void CQ3LevelMesh::loadMeshVerts(tBSPLump* l, io::IReadFile* file)
		{
			NumMeshVerts = l->length / sizeof(s32);
			if (!NumMeshVerts)
				return;
			MeshVerts = new s32[NumMeshVerts];

			file->seek(l->offset);
			file->read(MeshVerts, l->length);

			if (LoadParam.swapHeader)
			{
				for (int i = 0; i<NumMeshVerts; i++)
					MeshVerts[i] = byteswap(MeshVerts[i]);
			}
		}

		/*!
		*/
		void CQ3LevelMesh::loadBrushes(tBSPLump* l, io::IReadFile* file)
		{
			NumBrushes = l->length / sizeof(tBSPBrush);
			if (!NumBrushes)
				return;
			Brushes = new tBSPBrush[NumBrushes];

			file->seek(l->offset);
			file->read(Brushes, l->length);

			if (LoadParam.swapHeader)
			{
				for (s32 i = 0; i < NumBrushes; i++)
				{
					Brushes[i].brushSide = byteswap(Brushes[i].brushSide);
					Brushes[i].numOfBrushSides = byteswap(Brushes[i].numOfBrushSides);
					Brushes[i].textureID = byteswap(Brushes[i].textureID);
				}
			}
		}

		/*!
		*/
		void CQ3LevelMesh::loadBrushSides(tBSPLump* l, io::IReadFile* file)
		{
			numBrushSides = l->length / sizeof(tBSPBrushSide);
			if (!numBrushSides)
				return;
			BrushSides = new tBSPBrushSide[numBrushSides];

			file->seek(l->offset);
			file->read(BrushSides, l->length);

			if (LoadParam.swapHeader)
			{
				for (s32 i = 0; i < numBrushSides; i++)
				{
					BrushSides[i].plane = byteswap(BrushSides[i].plane);
					BrushSides[i].textureID= byteswap(BrushSides[i].textureID);
				}
			}
		}

		/*!
		*/
		void CQ3LevelMesh::loadLeafBrushes(tBSPLump* l, io::IReadFile* file)
		{
			// ignore
		}

		/*!
		*/
		inline bool isQ3WhiteSpace(const u8 symbol)
		{
			return symbol == ' ' || symbol == '\t' || symbol == '\r';
		}

		/*!
		*/
		inline bool isQ3ValidName(const u8 symbol)
		{
			return	(symbol >= 'a' && symbol <= 'z') ||
				(symbol >= 'A' && symbol <= 'Z') ||
				(symbol >= '0' && symbol <= '9') ||
				(symbol == '/' || symbol == '_' || symbol == '.');
		}

		/*!
		*/
		void CQ3LevelMesh::parser_nextToken()
		{
			u8 symbol;

			Parser.token = "";
			Parser.tokenresult = Q3_TOKEN_UNRESOLVED;

			// skip white space
			do
			{
				if (Parser.index >= Parser.sourcesize)
				{
					Parser.tokenresult = Q3_TOKEN_EOF;
					return;
				}

				symbol = Parser.source[Parser.index];
				Parser.index += 1;
			} while (isQ3WhiteSpace(symbol));

			// first symbol, one symbol
			switch (symbol)
			{
			case 0:
				Parser.tokenresult = Q3_TOKEN_EOF;
				return;

			case '/':
				// comment or divide
				if (Parser.index >= Parser.sourcesize)
				{
					Parser.tokenresult = Q3_TOKEN_EOF;
					return;
				}
				symbol = Parser.source[Parser.index];
				Parser.index += 1;
				if (isQ3WhiteSpace(symbol))
				{
					Parser.tokenresult = Q3_TOKEN_MATH_DIVIDE;
					return;
				}
				else
					if (symbol == '*')
					{
						// C-style comment in quake?
					}
					else
						if (symbol == '/')
						{
							// skip to eol
							do
							{
								if (Parser.index >= Parser.sourcesize)
								{
									Parser.tokenresult = Q3_TOKEN_EOF;
									return;
								}
								symbol = Parser.source[Parser.index];
								Parser.index += 1;
							} while (symbol != '\n');
							Parser.tokenresult = Q3_TOKEN_COMMENT;
							return;
						}
				// take /[name] as valid token..?!?!?. mhmm, maybe
				break;

			case '\n':
				Parser.tokenresult = Q3_TOKEN_EOL;
				return;
			case '{':
				Parser.tokenresult = Q3_TOKEN_START_LIST;
				return;
			case '}':
				Parser.tokenresult = Q3_TOKEN_END_LIST;
				return;

			case '"':
				// string literal
				do
				{
					if (Parser.index >= Parser.sourcesize)
					{
						Parser.tokenresult = Q3_TOKEN_EOF;
						return;
					}
					symbol = Parser.source[Parser.index];
					Parser.index += 1;
					if (symbol != '"')
						Parser.token.append(symbol);
				} while (symbol != '"');
				Parser.tokenresult = Q3_TOKEN_ENTITY;
				return;
			}

			// user identity
			Parser.token.append(symbol);

			// continue till whitespace
			bool validName = true;
			do
			{
				if (Parser.index >= Parser.sourcesize)
				{
					Parser.tokenresult = Q3_TOKEN_EOF;
					return;
				}
				symbol = Parser.source[Parser.index];

				validName = isQ3ValidName(symbol);
				if (validName)
				{
					Parser.token.append(symbol);
					Parser.index += 1;
				}
			} while (validName);

			Parser.tokenresult = Q3_TOKEN_TOKEN;
			return;
		}


		/*
		parse entity & shader
		calls callback on content in {}
		*/
		void CQ3LevelMesh::parser_parse(const void * data, const u32 size, CQ3LevelMesh::tParserCallback callback)
		{
			Parser.source = static_cast<const c8*>(data);
			Parser.sourcesize = size;
			Parser.index = 0;

			SVarGroupList *groupList;

			s32 active;
			s32 last;

			SVariable entity("");

			groupList = new SVarGroupList();

			groupList->VariableGroup.push_back(SVarGroup());
			active = last = 0;

			do
			{
				parser_nextToken();

				switch (Parser.tokenresult)
				{
				case Q3_TOKEN_START_LIST:
				{
					//stack = core::min_( stack + 1, 7 );

					groupList->VariableGroup.push_back(SVarGroup());
					last = active;
					active = groupList->VariableGroup.size() - 1;
					entity.clear();
				}  break;

				// a unregisterd variable is finished
				case Q3_TOKEN_EOL:
				{
					if (entity.isValid())
					{
						groupList->VariableGroup[active].Variable.push_back(entity);
						entity.clear();
					}
				} break;

				case Q3_TOKEN_TOKEN:
				case Q3_TOKEN_ENTITY:
				{
					Parser.token.make_lower();

					// store content based on line-delemiter
					if (0 == entity.isValid())
					{
						entity.name = Parser.token;
						entity.content = "";

					}
					else
					{
						if (entity.content.size())
						{
							entity.content += " ";
						}
						entity.content += Parser.token;
					}
				} break;

				case Q3_TOKEN_END_LIST:
				{
					//stack = core::max_( stack - 1, 0 );

					// close tag for first
					if (active == 1)
					{
						(this->*callback)(groupList, Q3_TOKEN_END_LIST);

						// new group
						groupList->drop();
						groupList = new SVarGroupList();
						groupList->VariableGroup.push_back(SVarGroup());
						last = 0;
					}

					active = last;
					entity.clear();

				} break;

				default:
					break;
				}

			} while (Parser.tokenresult != Q3_TOKEN_EOF);

			(this->*callback)(groupList, Q3_TOKEN_EOF);

			groupList->drop();
		}


		

		/*!
		Internal function to build a mesh.
		*/
		KhMesh** CQ3LevelMesh::buildMesh(s32 num)
		{
			KhMesh** newmesh = new KhMesh *[quake3::E_Q3_MESH_SIZE];

			s32 i, j, k, s;

			for (i = 0; i < E_Q3_MESH_SIZE; i++)
			{
				newmesh[i] = new KhMesh();
			}

			s32 *index;

			video::S3DVertex2TCoords temp[3];
			s32 material;
			s32 material2;

			SToBuffer item[E_Q3_MESH_SIZE];
			u32 itemSize;

			for (i = Models[num].faceIndex; i < Models[num].numOfFaces + Models[num].faceIndex; ++i)
			{
				const tBSPFace * face = Faces + i;
				
				material = face->textureID;
				itemSize = 0;


				if (face->fogNum >= 0)
				{
					//todo fog
					/*setShaderFogMaterial(material2, face);
					item[itemSize].index = E_Q3_MESH_FOG;
					item[itemSize].takeVertexColor = 1;
					itemSize += 1;*/
				}

				switch (face->type)
				{
				case 1: // normal polygons
				case 2: // patches
				case 3: // meshes
					if (0 == 0)
					{
						if (LoadParam.cleanUnResolvedMeshes || material>=0)
						{
							item[itemSize].takeVertexColor = 1;
							item[itemSize].index = E_Q3_MESH_GEOMETRY;
							itemSize += 1;
						}
						else
						{
							item[itemSize].takeVertexColor = 1;
							item[itemSize].index = E_Q3_MESH_UNRESOLVED;
							itemSize += 1;
						}
					}
					else
					{
						item[itemSize].takeVertexColor = 1;
						item[itemSize].index = E_Q3_MESH_ITEMS;
						itemSize += 1;
					}
					break;

				case 4: // billboards
						//item[itemSize].takeVertexColor = 1;
						//item[itemSize].index = E_Q3_MESH_ITEMS;
						//itemSize += 1;
					break;

				}

				for (u32 g = 0; g != itemSize; ++g)
				{
					KhMeshBuffer* buffer = 0;

					if (item[g].index == E_Q3_MESH_GEOMETRY)
					{
						if (0 == item[g].takeVertexColor)
						{
							item[g].takeVertexColor = false;/*material.getTexture(0) == 0 || material.getTexture(1) == 0;*///TODO: vertext color
						}

						//if (Faces[i].lightmapID < -1 || Faces[i].lightmapID > NumLightMaps - 1)
						//{
							//Faces[i].lightmapID = -1;
						//}

#if 0
						// there are lightmapsids and textureid with -1
						const s32 tmp_index = ((Faces[i].lightmapID + 1) * (NumTextures + 1)) + (Faces[i].textureID + 1);
						buffer = (SMeshBufferLightMap*)newmesh[E_Q3_MESH_GEOMETRY]->getMeshBuffer(tmp_index);
						buffer->setHardwareMappingHint(EHM_STATIC);
						buffer->getMaterial() = material;
#endif
					}

					// Construct a unique mesh for each shader or combine meshbuffers for same shader
					if (0 == buffer)
					{

						if (LoadParam.mergeShaderBuffer == 1)
						{
							// combine
							buffer = (KhMeshBuffer*)newmesh[item[g].index]->getMeshBuffer(Faces[i].textureID,Faces[i].lightmapID);
						}

						// create a seperate mesh buffer
						if (0 == buffer)
						{
							buffer = new KhMeshBuffer();
							buffer->texture = this->Textures[Faces[i].textureID].strName;
							buffer->textureID = Faces[i].textureID;
							buffer->lightmapID = Faces[i].lightmapID;
							newmesh[item[g].index]->buffers.push_back(buffer);
							
						}
					}


					switch (Faces[i].type)
					{
					case 4: // billboards
						break;
					case 2: // patches
						createCurvedSurface_bezier(buffer, i,
							LoadParam.patchTesselation,
							item[g].takeVertexColor
						);
						break;

					case 1: // normal polygons
					case 3: // mesh vertices
						index = MeshVerts + face->meshVertIndex;
						k = buffer->Vertices.size();

						// reallocate better if many small meshes are used
						s = buffer->Indices.size() + face->numMeshVerts;
						if (buffer->Indices.allocated_size() < (u32)s)
						{
							if (buffer->Indices.allocated_size() > 0 &&
								face->numMeshVerts < 20 && NumFaces > 1000
								)
							{
								s = buffer->Indices.size() + (NumFaces >> 3 * face->numMeshVerts);
							}
							buffer->Indices.reallocate(s);
						}

						for (j = 0; j < face->numMeshVerts; ++j)
						{
							buffer->Indices.push_back(k + index[j]);
						}

						s = k + face->numOfVerts;
						if (buffer->Vertices.allocated_size() < (u32)s)
						{
							if (buffer->Indices.allocated_size() > 0 &&
								face->numOfVerts < 20 && NumFaces > 1000
								)
							{
								s = buffer->Indices.size() + (NumFaces >> 3 * face->numOfVerts);
							}
							buffer->Vertices.reallocate(s);
						}
						for (j = 0; j != face->numOfVerts; ++j)
						{
							copy(&temp[0], &Vertices[j + face->vertexIndex], item[g].takeVertexColor);
							buffer->Vertices.push_back(temp[0]);
						}
						break;

					} // end switch
				}
			}

			return newmesh;
		}

		/*!
		*/
		void CQ3LevelMesh::solveTJunction()
		{
		}

		/*!
		constructs a mesh from the quake 3 level file.
		*/
		void CQ3LevelMesh::constructMesh()
		{
			if (LoadParam.verbose > 0)
			{

				if (LoadParam.verbose > 1)
				{
					snprintf(buf, sizeof(buf),
						"quake3::constructMesh start to create %d faces, %d vertices,%d mesh vertices",
						NumFaces,
						NumVertices,
						NumMeshVerts
					);
				}

			}

			s32 i, j;

			// First the main level
			KhMesh **tmp = buildMesh(0);

			for (i = 0; i < E_Q3_MESH_SIZE; i++)
			{
				Mesh[i] = tmp[i];
			}
			delete[] tmp;

			// Then the brush entities

			for (i = 1; i < NumModels; i++)
			{
				tmp = buildMesh(i);
				BrushEntities[i] = tmp[0];

				// We only care about the main geometry here
				/*for (j = 1; j < E_Q3_MESH_SIZE; j++)
				{
					delete tmp[j];
				}
				delete[] tmp;*/
			}

			if (LoadParam.verbose > 0)
			{

				snprintf(buf, sizeof(buf),
					"quake3::constructMesh needed %04d ms to create %d faces, %d vertices,%d mesh vertices",
					LoadParam.endTime - LoadParam.startTime,
					NumFaces,
					NumVertices,
					NumMeshVerts
				);
			}

		}


		void CQ3LevelMesh::S3DVertex2TCoords_64::copy(video::S3DVertex2TCoords &dest) const
		{
			dest.Pos.X = core::round_((f32)Pos.X);
#if defined (TJUNCTION_SOLVER_ROUND)
			dest.Pos.X = core::round_((f32)Pos.X);
			dest.Pos.Y = core::round_((f32)Pos.Y);
			dest.Pos.Z = core::round_((f32)Pos.Z);
#elif defined (TJUNCTION_SOLVER_0125)
			dest.Pos.X = (f32)(floor(Pos.X * 8.f + 0.5) * 0.125);
			dest.Pos.Y = (f32)(floor(Pos.Y * 8.f + 0.5) * 0.125);
			dest.Pos.Z = (f32)(floor(Pos.Z * 8.f + 0.5) * 0.125);
#else
			dest.Pos.X = (f32)Pos.X;
			dest.Pos.Y = (f32)Pos.Y;
			dest.Pos.Z = (f32)Pos.Z;
#endif

			dest.Normal.X = (f32)Normal.X;
			dest.Normal.Y = (f32)Normal.Y;
			dest.Normal.Z = (f32)Normal.Z;
			dest.Normal.normalize();

			dest.Color = Color.toSColor();

			dest.TCoords.X = (f32)TCoords.X;
			dest.TCoords.Y = (f32)TCoords.Y;

			dest.TCoords2.X = (f32)TCoords2.X;
			dest.TCoords2.Y = (f32)TCoords2.Y;
		}


		void CQ3LevelMesh::copy(S3DVertex2TCoords_64 * dest, const tBSPVertex * source, s32 vertexcolor) const
		{
#if defined (TJUNCTION_SOLVER_ROUND)
			dest->Pos.X = core::round_(source->vPosition[0]);
			dest->Pos.Y = core::round_(source->vPosition[2]);
			dest->Pos.Z = core::round_(source->vPosition[1]);
#elif defined (TJUNCTION_SOLVER_0125)
			dest->Pos.X = (f32)(floor(source->vPosition[0] * 8.f + 0.5) * 0.125);
			dest->Pos.Y = (f32)(floor(source->vPosition[2] * 8.f + 0.5) * 0.125);
			dest->Pos.Z = (f32)(floor(source->vPosition[1] * 8.f + 0.5) * 0.125);
#else
			dest->Pos.X = source->vPosition[0];
			dest->Pos.Y = source->vPosition[2];
			dest->Pos.Z = source->vPosition[1];
#endif

			dest->Normal.X = source->vNormal[0];
			dest->Normal.Y = source->vNormal[2];
			dest->Normal.Z = source->vNormal[1];
			dest->Normal.normalize();

			dest->TCoords.X = source->vTextureCoord[0];
			dest->TCoords.Y = source->vTextureCoord[1];
			dest->TCoords2.X = source->vLightmapCoord[0];
			dest->TCoords2.Y = source->vLightmapCoord[1];

			if (vertexcolor)
			{
				//u32 a = core::s32_min( source->color[3] * LoadParam.defaultModulate, 255 );
				u32 a = source->color[3];
				u32 r = core::s32_min(source->color[0] * LoadParam.defaultModulate, 255);
				u32 g = core::s32_min(source->color[1] * LoadParam.defaultModulate, 255);
				u32 b = core::s32_min(source->color[2] * LoadParam.defaultModulate, 255);

				dest->Color.set(a * 1.f / 255.f, r * 1.f / 255.f,
					g * 1.f / 255.f, b * 1.f / 255.f);
			}
			else
			{
				dest->Color.set(1.f, 1.f, 1.f, 1.f);
			}
		}


		inline void CQ3LevelMesh::copy(video::S3DVertex2TCoords * dest, const tBSPVertex * source, s32 vertexcolor) const
		{
#if defined (TJUNCTION_SOLVER_ROUND)
			dest->Pos.X = core::round_(source->vPosition[0]);
			dest->Pos.Y = core::round_(source->vPosition[2]);
			dest->Pos.Z = core::round_(source->vPosition[1]);
#elif defined (TJUNCTION_SOLVER_0125)
			dest->Pos.X = (f32)(floor(source->vPosition[0] * 8.f + 0.5) * 0.125);
			dest->Pos.Y = (f32)(floor(source->vPosition[2] * 8.f + 0.5) * 0.125);
			dest->Pos.Z = (f32)(floor(source->vPosition[1] * 8.f + 0.5) * 0.125);
#else
			dest->Pos.X = source->vPosition[0];
			dest->Pos.Y = source->vPosition[2];
			dest->Pos.Z = source->vPosition[1];
#endif

			dest->Normal.X = source->vNormal[0];
			dest->Normal.Y = source->vNormal[2];
			dest->Normal.Z = source->vNormal[1];
			dest->Normal.normalize();

			dest->TCoords.X = source->vTextureCoord[0];
			dest->TCoords.Y = source->vTextureCoord[1];
			dest->TCoords2.X = source->vLightmapCoord[0];
			dest->TCoords2.Y = source->vLightmapCoord[1];

			if (vertexcolor)
			{
				//u32 a = core::s32_min( source->color[3] * LoadParam.defaultModulate, 255 );
				u32 a = source->color[3];
				u32 r = core::s32_min(source->color[0] * LoadParam.defaultModulate, 255);
				u32 g = core::s32_min(source->color[1] * LoadParam.defaultModulate, 255);
				u32 b = core::s32_min(source->color[2] * LoadParam.defaultModulate, 255);

				dest->Color.set(a << 24 | r << 16 | g << 8 | b);
			}
			else
			{
				dest->Color.set(0xFFFFFFFF);
			}
		}


		void CQ3LevelMesh::SBezier::tesselate(s32 level)
		{
			//Calculate how many vertices across/down there are
			s32 j, k;

			column[0].set_used(level + 1);
			column[1].set_used(level + 1);
			column[2].set_used(level + 1);

			const f64 w = 0.0 + (1.0 / (f64)level);

			//Tesselate along the columns
			for (j = 0; j <= level; ++j)
			{
				const f64 f = w * (f64)j;

				column[0][j] = control[0].getInterpolated_quadratic(control[3], control[6], f);
				column[1][j] = control[1].getInterpolated_quadratic(control[4], control[7], f);
				column[2][j] = control[2].getInterpolated_quadratic(control[5], control[8], f);
			}

			const u32 idx = Patch->Vertices.size();
			Patch->Vertices.reallocate(idx + level*level);
			//Tesselate across the rows to get final vertices
			video::S3DVertex2TCoords v;
			S3DVertex2TCoords_64 f;
			for (j = 0; j <= level; ++j)
			{
				for (k = 0; k <= level; ++k)
				{
					f = column[0][j].getInterpolated_quadratic(column[1][j], column[2][j], w * (f64)k);
					f.copy(v);
					Patch->Vertices.push_back(v);
				}
			}

			Patch->Indices.reallocate(Patch->Indices.size() + 6 * level*level);
			// connect
			for (j = 0; j < level; ++j)
			{
				for (k = 0; k < level; ++k)
				{
					const s32 inx = idx + (k * (level + 1)) + j;

					Patch->Indices.push_back(inx + 0);
					Patch->Indices.push_back(inx + (level + 1) + 0);
					Patch->Indices.push_back(inx + (level + 1) + 1);

					Patch->Indices.push_back(inx + 0);
					Patch->Indices.push_back(inx + (level + 1) + 1);
					Patch->Indices.push_back(inx + 1);
				}
			}
		}


		/*!
		no subdivision
		*/
		void CQ3LevelMesh::createCurvedSurface_nosubdivision(KhMeshBuffer* meshBuffer,
			s32 faceIndex,
			s32 patchTesselation,
			s32 storevertexcolor)
		{
			tBSPFace * face = &Faces[faceIndex];
			u32 j, k, m;

			// number of control points across & up
			const u32 controlWidth = face->size[0];
			const u32 controlHeight = face->size[1];
			if (0 == controlWidth || 0 == controlHeight)
				return;

			video::S3DVertex2TCoords v;

			m = meshBuffer->Vertices.size();
			meshBuffer->Vertices.reallocate(m + controlHeight * controlWidth);
			for (j = 0; j != controlHeight * controlWidth; ++j)
			{
				copy(&v, &Vertices[face->vertexIndex + j], storevertexcolor);
				meshBuffer->Vertices.push_back(v);
			}

			meshBuffer->Indices.reallocate(meshBuffer->Indices.size() + 6 * (controlHeight - 1) * (controlWidth - 1));
			for (j = 0; j != controlHeight - 1; ++j)
			{
				for (k = 0; k != controlWidth - 1; ++k)
				{
					meshBuffer->Indices.push_back(m + k + 0);
					meshBuffer->Indices.push_back(m + k + controlWidth + 0);
					meshBuffer->Indices.push_back(m + k + controlWidth + 1);

					meshBuffer->Indices.push_back(m + k + 0);
					meshBuffer->Indices.push_back(m + k + controlWidth + 1);
					meshBuffer->Indices.push_back(m + k + 1);
				}
				m += controlWidth;
			}
		}


		/*!
		*/
		void CQ3LevelMesh::createCurvedSurface_bezier(KhMeshBuffer* meshBuffer,
			s32 faceIndex,
			s32 patchTesselation,
			s32 storevertexcolor)
		{

			tBSPFace * face = &Faces[faceIndex];
			u32 j, k;

			// number of control points across & up
			const u32 controlWidth = face->size[0];
			const u32 controlHeight = face->size[1];

			if (0 == controlWidth || 0 == controlHeight)
				return;

			// number of biquadratic patches
			const u32 biquadWidth = (controlWidth - 1) / 2;
			const u32 biquadHeight = (controlHeight - 1) / 2;

			if (LoadParam.verbose > 1)
			{
			}

			// Create space for a temporary array of the patch's control points
			core::array<S3DVertex2TCoords_64> controlPoint;
			controlPoint.set_used(controlWidth * controlHeight);

			for (j = 0; j < controlPoint.size(); ++j)
			{
				copy(&controlPoint[j], &Vertices[face->vertexIndex + j], storevertexcolor);
			}

			// create a temporary patch
			Bezier.Patch = new KhMeshBuffer();
			Bezier.Patch->texture = this->Textures[face->textureID].strName;
			Bezier.Patch->textureID = face->textureID;
			Bezier.Patch->lightmapID = face->lightmapID;
			//Loop through the biquadratic patches
			for (j = 0; j < biquadHeight; ++j)
			{
				for (k = 0; k < biquadWidth; ++k)
				{
					// set up this patch
					const s32 inx = j*controlWidth * 2 + k * 2;

					// setup bezier control points for this patch
					Bezier.control[0] = controlPoint[inx + 0];
					Bezier.control[1] = controlPoint[inx + 1];
					Bezier.control[2] = controlPoint[inx + 2];
					Bezier.control[3] = controlPoint[inx + controlWidth + 0];
					Bezier.control[4] = controlPoint[inx + controlWidth + 1];
					Bezier.control[5] = controlPoint[inx + controlWidth + 2];
					Bezier.control[6] = controlPoint[inx + controlWidth * 2 + 0];
					Bezier.control[7] = controlPoint[inx + controlWidth * 2 + 1];
					Bezier.control[8] = controlPoint[inx + controlWidth * 2 + 2];

					Bezier.tesselate(patchTesselation);
				}
			}

			// stitch together with existing geometry
			// TODO: only border needs to be checked
			const u32 bsize = Bezier.Patch->Vertices.size();
			const u32 msize = meshBuffer->Vertices.size();
			/*
			for ( j = 0; j!= bsize; ++j )
			{
			const core::vector3df &v = Bezier.Patch->Vertices[j].Pos;

			for ( k = 0; k!= msize; ++k )
			{
			const core::vector3df &m = meshBuffer->Vertices[k].Pos;

			if ( !v.equals( m, tolerance ) )
			continue;

			meshBuffer->Vertices[k].Pos = v;
			//Bezier.Patch->Vertices[j].Pos = m;
			}
			}
			*/

			// add Patch to meshbuffer
			meshBuffer->Vertices.reallocate(msize + bsize);
			for (j = 0; j != bsize; ++j)
			{
				meshBuffer->Vertices.push_back(Bezier.Patch->Vertices[j]);
			}

			// add indices to meshbuffer
			meshBuffer->Indices.reallocate(meshBuffer->Indices.size() + Bezier.Patch->Indices.size());
			for (j = 0; j != Bezier.Patch->Indices.size(); ++j)
			{
				meshBuffer->Indices.push_back(msize + Bezier.Patch->Indices[j]);
			}

			delete Bezier.Patch;

			if (LoadParam.verbose > 1)
			{

				snprintf(buf, sizeof(buf),
					"quake3::createCurvedSurface_bezier needed %04d ms to create bezier patch.(%dx%d)",
					LoadParam.endTime - LoadParam.startTime,
					biquadWidth,
					biquadHeight
				);
			}

		}



		
		//! get's an interface to the entities
		tQ3EntityList & CQ3LevelMesh::getEntityList()
		{
			//	Entity.sort();
			return Entity;
		}

		//! returns the requested brush entity
		KhMesh* CQ3LevelMesh::getBrushEntityMesh(s32 num) const
		{
			if (num < 1 || num >= NumModels)
				return 0;

			return BrushEntities[num];
		}

		//! returns the requested brush entity
		KhMesh* CQ3LevelMesh::getBrushEntityMesh(quake3::IEntity &ent) const
		{
			// This is a helper function to parse the entity,
			// so you don't have to.

			s32 num;

			const quake3::SVarGroup* group = ent.getGroup(1);
			const core::stringc& modnum = group->get("model");

			if (!group->isDefined("model"))
				return 0;

			const char *temp = modnum.c_str() + 1; // We skip the first character.
			num = core::strtol10(temp);

			return getBrushEntityMesh(num);
		}
		/*!
		*/
		void CQ3LevelMesh::ReleaseEntity()
		{
			for (u32 i = 0; i != Entity.size(); ++i)
			{
				Entity[i].VarGroup->drop();
			}
			Entity.clear();
		}


		// config in simple (quake3) and advanced style
		void CQ3LevelMesh::scriptcallback_config(SVarGroupList *& grouplist, eToken token)
		{
			IShader element;

			if (token == Q3_TOKEN_END_LIST)
			{
				if (0 == grouplist->VariableGroup[0].Variable.size())
					return;

				element.name = grouplist->VariableGroup[0].Variable[0].name;
			}
			else
			{
				if (grouplist->VariableGroup.size() != 2)
					return;

				element.name = "configuration";
			}

			grouplist->grab();
			element.VarGroup = grouplist;
			element.ID = Entity.size();
			Entity.push_back(element);
		}


		// entity only has only one valid level.. and no assoziative name..
		void CQ3LevelMesh::scriptcallback_entity(SVarGroupList *& grouplist, eToken token)
		{
			if (token != Q3_TOKEN_END_LIST || grouplist->VariableGroup.size() != 2)
				return;

			grouplist->grab();

			IEntity element;
			element.VarGroup = grouplist;
			element.ID = Entity.size();
			element.name = grouplist->VariableGroup[1].get("classname");


			Entity.push_back(element);
		}


		//!. script callback for shaders
		void CQ3LevelMesh::scriptcallback_shader(SVarGroupList *& grouplist, eToken token)
		{
			if (token != Q3_TOKEN_END_LIST || grouplist->VariableGroup[0].Variable.size() == 0)
				return;


			IShader element;

			grouplist->grab();
			element.VarGroup = grouplist;
			element.name = element.VarGroup->VariableGroup[0].Variable[0].name;
			element.ID = Shader.size();
			/*
			core::stringc s;
			dumpShader ( s, &element );
			printf ( s.c_str () );
			*/
			Shader.push_back(element);
		}


		/*!
		delete all buffers without geometry in it.
		*/
		void CQ3LevelMesh::cleanMeshes()
		{
			if (0 == LoadParam.cleanUnResolvedMeshes)
				return;

			s32 i;

			// First the main level
			for (i = 0; i < E_Q3_MESH_SIZE; i++)
			{
				//bool texture0important = (i == 0);
				bool texture0important = false;

				cleanMesh(Mesh[i], texture0important);
			}

			// Then the brush entities
			for (i = 1; i < NumModels; i++)
			{
				cleanMesh(BrushEntities[i], true);
			}
		}

		void CQ3LevelMesh::cleanMesh(KhMesh *m, const bool texture0important)
		{
			// delete all buffers without geometry in it.
			u32 run = 0;
			u32 remove = 0;

			KhMeshBuffer *b;

			run = 0;
			remove = 0;

			if (LoadParam.verbose > 0)
			{
				if (LoadParam.verbose > 1)
				{
					snprintf(buf, sizeof(buf),
						"quake3::cleanMeshes start for %d meshes",
						m->buffers.size()
					);
				}
			}

			u32 i = 0;
			s32 blockstart = -1;
			s32 blockcount = 0;

			while (i < m->buffers.size())
			{
				run += 1;

				b = m->buffers[i];

				if (b->Vertices.size() == 0 || b->Indices.size() == 0 ||
					(texture0important && b->texture == 0)
					)
				{
					if (blockstart < 0)
					{
						blockstart = i;
						blockcount = 0;
					}
					blockcount += 1;
					i += 1;

					// delete Meshbuffer
					i -= 1;
					remove += 1;
					//b->drop();//todo delete b
					m->buffers.erase(m->buffers.begin() + i);
				}
				else
				{
					// clean blockwise
					if (blockstart >= 0)
					{
						if (LoadParam.verbose > 1)
						{
							snprintf(buf, sizeof(buf),
								"quake3::cleanMeshes cleaning mesh %d %d size",
								blockstart,
								blockcount
							);
						}
						blockstart = -1;
					}
					i += 1;
				}
			}

			if (LoadParam.verbose > 0)
			{
				snprintf(buf, sizeof(buf),
					"quake3::cleanMeshes needed %04d ms to clean %d of %d meshes",
					LoadParam.endTime - LoadParam.startTime,
					remove,
					run
				);
			}
		}

} // end namespace scene

#endif // _IRR_COMPILE_WITH_BSP_LOADER_
