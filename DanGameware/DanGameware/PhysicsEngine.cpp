#pragma once
#include <math.h>
#include <stdlib.h>
#include <vector>
#include <functional>
#include <algorithm>
#include "PhysicsEngine.h"
#include "khmath.h"
#include "slidingsphere.h";
#include <Horde3D.h>
#include "collision.h"
#include "Gameplay.h"
#include "enemy.h"

const int MinimalPolysPerNode = 64;
int NodeCount = 0;


physicEngine g_phyis;

struct SOctreeNode
{
	SOctreeNode* child[8];
	Aabbox3d box;
	Triangle* Triangles;
	u32 triangle_size;
	SOctreeNode()
	{
		for (int i = 0; i < 8; i++)
		{
			child[i] = nullptr;
		}
		triangle_size = 0;
	}
	~SOctreeNode()
	{
		for (int i = 0; i < 8; i++)
		{
			if (child[i] != nullptr)
				delete child[i];
		}
	}
};

SOctreeNode* g_octree_root = nullptr;
void constructOctree(SOctreeNode* node);
void getTrianglesFromOctree(SOctreeNode* node, s32& trianglesWritten, s32 maximumSize, const Aabbox3d& box, const Matrix* mat, Triangle* triangles);


SOctreeNode* GenerateTheTree(std::vector<Triangle> mesh)
{
	auto root = new SOctreeNode();
	root->Triangles = new Triangle[mesh.size()];
	root->triangle_size = mesh.size();
	for (int i = 0; i < root->triangle_size; i++)
	{
		root->Triangles[i] = mesh[i];
	}
	constructOctree(root);
	return root;
}

void constructOctree(SOctreeNode* node)
{
	++NodeCount;

	node->box.reset(node->Triangles[0].p1);

	// get bounding box
	const u32 cnt = node->triangle_size;
	for (u32 i = 0; i<cnt; ++i)
	{
		node->box.addInternalPoint(node->Triangles[i].p1);
		node->box.addInternalPoint(node->Triangles[i].p2);
		node->box.addInternalPoint(node->Triangles[i].p3);
	}

	const Vector3df& middle = (node->box.minEdge + node->box.maxEdge) / 2;
	Vector3df edges[8];
	node->box.getEdges(edges);

	Aabbox3d box;

	// calculate children
	if (!node->box.isEmpty() && (s32)node->triangle_size > MinimalPolysPerNode)
		for (s32 ch = 0; ch<8; ++ch)
		{
			std::vector<Triangle> keepTriangles;

			box.reset(middle);
			box.addInternalPoint(edges[ch]);
			node->child[ch] = new SOctreeNode();
			std::vector<Triangle> child_tris;
			for (s32 i = 0; i<(s32)node->triangle_size; ++i)
			{
				if (node->Triangles[i].isTotalInsideBox(box))
				{
					child_tris.push_back(node->Triangles[i]);
				}
				else
				{
					keepTriangles.push_back(node->Triangles[i]);
				}
			}
			if (child_tris.size() > 0)
			{
				node->child[ch]->Triangles = new Triangle[child_tris.size()];
				node->child[ch]->triangle_size = child_tris.size();
				for (int i = 0; i < node->child[ch]->triangle_size; i++)
				{
					node->child[ch]->Triangles[i] = child_tris[i];
				}
			}
			else
			{
				delete node->child[ch];
				node->child[ch] = nullptr;
			}
			
			

			delete(node->Triangles);
			node->Triangles = new Triangle[keepTriangles.size()];
			node->triangle_size = keepTriangles.size();
			for (int i = 0; i < node->triangle_size; i++)
			{
				node->Triangles[i] = keepTriangles[i];
			}


			if (node->child[ch] != nullptr)
			{
				constructOctree(node->child[ch]);
			}
		}
}

//! Gets all triangles which lie within a specific bounding box.
void getTriangles(Triangle* triangles,s32 arraySize, s32& outTriangleCount,const Aabbox3d& box,const Matrix* transform)
{
	Matrix mat;
	auto invbox = box;

	if (transform)
		mat = *transform;
	else
		mat.makeIdentity();

	s32 trianglesWritten = 0;

	if (g_octree_root)
		getTrianglesFromOctree(g_octree_root, trianglesWritten,arraySize, invbox, &mat, triangles);
	outTriangleCount = trianglesWritten;
}


void getTrianglesFromOctree(SOctreeNode* node, s32& trianglesWritten,s32 maximumSize, const Aabbox3d& box,const Matrix* mat, Triangle* triangles)
{
	if (!box.intersectsWithBox(node->box))
		return;
	if (trianglesWritten == maximumSize)
		return;

	const u32 cnt = node->triangle_size;

	for (u32 i = 0; i<cnt; ++i)
	{
		const Triangle& srcTri = node->Triangles[i];
		// This isn't an accurate test, but it's fast, and the 
		// API contract doesn't guarantee complete accuracy.
		if (srcTri.isTotalOutsideBox(box))
			continue;

		Triangle& dstTri = triangles[trianglesWritten];
		mat->transformVect(dstTri.p1, srcTri.p1);
		mat->transformVect(dstTri.p2, srcTri.p2);
		mat->transformVect(dstTri.p3, srcTri.p3);
		++trianglesWritten;

		// Halt when the out array is full.
		if (trianglesWritten == maximumSize)
			return;
	}

	for (u32 i = 0; i<8; ++i)
		if (node->child[i])
			getTrianglesFromOctree(node->child[i], trianglesWritten,
				maximumSize, box, mat, triangles);
}
//====================================================================================================================
struct Brush
{
	Plane* planes;
	size_t numPlanes;
};
struct Trigger
{
	int entity;
	Aabbox3d aabb;
	u32 colisionMask;
	Brush* brushes;
	u32 numBrushes;
};
bool pointInsideTrigger(const Trigger* t,Vector3df point)
{
	if (t->aabb.isPointInside(point) == false)
		return false;
	for (u32 i = 0; i < t->numBrushes; i++)
	{
		bool inside = true;
		for (u32 j = 0; j < t->brushes[i].numPlanes; j++)
		{
			if(t->brushes[i].planes[i].isPointFront(point))
			{
				inside = false;
				break;
			}
		}
		if (inside)
			return true;
	}
}
void physicEngine::loadLevel(float* tri_data, size_t tri_count)
{
	std::vector<Triangle> tris;
	int ind = 0;
	for (int i = 0; i < tri_count; i++)
	{
		Triangle t;
		t.p1.x = tri_data[ind++];
		t.p1.y = tri_data[ind++];
		t.p1.z = tri_data[ind++];

		t.p2.x = tri_data[ind++];
		t.p2.y = tri_data[ind++];
		t.p2.z = tri_data[ind++];

		t.p3.x = tri_data[ind++];
		t.p3.y = tri_data[ind++];
		t.p3.z = tri_data[ind++];
		tris.push_back(t);
	}
	
	g_octree_root = GenerateTheTree(tris);

}

struct Pawn
{
	float x, y, z, rx, ry, rz, vx, vy, vz;
	bool grounded;
	Vector3df FallingVelocity;
	bool FirstUpdate = true;
	Vector3df LastPosition;
	bool Falling;
	void* user_data;
	u32 flags;
	bool dead;
	Pawn()
	{
		FirstUpdate = true;
		FallingVelocity = Vector3df(0, 0, 0);
		user_data = nullptr;
	}
};

struct SphereProjectile
{
	Vector3df pos,vel;
	float radius;
	std::function<void(const contanctInfo&)> callback;
	bool dead;
	Hndl ignorePawn;
	SphereProjectile():dead(false)
	{
		
	}
};
Hndl physicEngine::createPawn(float radius, float height, float *pos,void* userdata,u32 flags)
{
	auto pw = new Pawn();
	Pawn& p = *pw;
	p.x = pos[0];
	p.y = pos[1];
	p.z = pos[2];
	p.rx = radius;
	p.ry = height/2;
	p.rz = radius;
	p.vx = 0;
	p.vy = 0;
	p.vz = 0;
	p.user_data = userdata;
	p.flags = flags;
	p.dead = false;
	pawns.push_back(pw);
	return pawns.size() - 1;
}
void physicEngine::removePawn(Hndl pawn)
{
	pawns[pawn]->dead = true;
}
Hndl physicEngine::createSphereProjectile(float radius, float *pos, std::function<void(const contanctInfo&)> callback,Hndl ignore_pawn)
{
	auto sc = new SphereProjectile();
	sc->pos.x = pos[0];
	sc->pos.y = pos[1];
	sc->pos.z = pos[2];
	
	sc->radius = radius;
	sc->callback = callback;
	sc->ignorePawn = ignore_pawn;
	sphereProjectiles.push_back(sc);
	return sphereProjectiles.size() - 1;
}
void physicEngine::update(float dt)
{
	for (const auto& p : pawns)
	{
		if (p->dead)
			continue;
		Vector3df pos(p->x, p->y, p->z);
		Vector3df rad(p->rx, p->ry, p->rz);
		Vector3df vel(p->vx, p->vy, p->vz);
		vel *= dt;
		
		if (p->FirstUpdate)
		{
			p->LastPosition = pos;
			p->Falling = false;
			p->FallingVelocity.set(0, 0, 0);
			p->FirstUpdate = false;
		}

		p->FallingVelocity += Vector3df(0, -800, 0)*dt;//gravity

		Triangle CollisionTriangle;
		Vector3df CollisionPoint;
		bool falling=false;
		
		braynzar::CollisionPacket camcp;
		camcp.w_Position = pos;
		camcp.w_Velocity = vel;
		camcp.ellipsoidSpace = rad;
		Vector3df CollisionResultPosition = braynzar::CollisionSlide(camcp,p->FallingVelocity*dt,falling);
		if (!falling)
		{
			p->FallingVelocity.set(0, 0, 0);
		}
		bool dont_move = false;
		for (const auto& p2 : pawns)
		{
			if (p2->dead)
				continue;
			if (p2 == p)
				continue;
			Vector3df p2pos(p2->x, p2->y, p2->z);
			f32 r2 = (p2->rx + p->rx);
			r2 *= r2;
			if ((p2pos - CollisionResultPosition).getLengthSQ() < r2)
			{
				dont_move = true;
				break;
			}
		}
		if (!dont_move)
		{
			p->x = CollisionResultPosition.x;
			p->y = CollisionResultPosition.y;
			p->z = CollisionResultPosition.z;
		}

		p->grounded = !falling;
	}
	Triangle tlist[256];
	Aabbox3d sphere_bb;
	for (const auto& sc : sphereProjectiles)
	{
		if (sc->dead)
			continue;
		sphere_bb.minEdge = sc->pos - sc->radius;
		sphere_bb.maxEdge = sc->pos + sc->radius;
		Matrix scaleMatrix;
		//scaleMatrix.setScale(Vector3df(1.0f / sc->radius, 1.0f / sc->radius, 1.0f / sc->radius));
		scaleMatrix.setScale(Vector3df(1,1,1));

		s32 uc;
		getTriangles(tlist, 256, uc, sphere_bb, &scaleMatrix);
		for (int i = 0; i < uc; i++)
		{
			
			auto t = tlist[i];
			contanctInfo ci;
			ci.isOtherPawn = false;
			bool col = ColShpereTriangle(t, sc->pos, sc->radius,ci);
			if (col)
			{
				sc->callback(ci);
				sc->dead = true;
				//delete sc;
				break;
			}
		}
		if (sc->dead)
			continue;
		//pawn collision
		for(Hndl i=0;i<pawns.size();i++)
		{
			if (sc->ignorePawn == i)
				continue;
			auto p = pawns[i];
			if (p->dead)
				continue;
			Vector3df pos(p->x, p->y, p->z);
			auto rr = Vector3df(0, p->ry, 0);
			contanctInfo ci;
			ci.isOtherPawn = true;
			bool col = ColShpereCapsule(sc->pos, sc->radius, pos + rr, pos - rr, __max(p->rx, p->rz)/2,ci);
			if (col)
			{
				sc->callback(ci);
				sc->dead = true;
				if (p->flags&PAWN_FLAG_ENEMY)
				{
					((Enemy*)p->user_data)->onProjectileHit(ci.contactPoint);
				}
				if (p->flags&PAWN_FLAG_PLAYER)
				{
					((FPSCharacter*)p->user_data)->onProjectileHit(ci.contactPoint);
				}
				break;
			}
		}
	}
	//sphereProjectiles.erase(std::remove_if(sphereProjectiles.begin(), sphereProjectiles.end(), [](SphereProjectile* x) {return x->dead; }), sphereProjectiles.end());
}

void physicEngine::setVelocity(Hndl hndl, float* velocity)
{
	Pawn& p = *(pawns[hndl]);
	p.vx = velocity[0];
	p.vy = velocity[1];
	p.vz = velocity[2];
}
void physicEngine::setPosition(Hndl hndl, float* velocity)
{
	Pawn& p = *(pawns[hndl]);
	p.x = velocity[0];
	p.y = velocity[1];
	p.z = velocity[2];
}
void physicEngine::pawnJump(Hndl hndl, float velocity)
{
	Pawn& p = *(pawns[hndl]);
	p.FallingVelocity.y = velocity;
}
physObjectTransform physicEngine::getTransform(Hndl hndl)
{
	physObjectTransform t;
	Pawn& p= *(pawns[hndl]);
	t.x = p.x;
	t.y = p.y;
	t.z = p.z;
	t.grounded = p.grounded;
	return t;
}
void physicEngine::setProjectilePosition(Hndl proj, Vector3df pos)
{
	this->sphereProjectiles[proj]->pos = pos;
}

float* physicEngine::createLevelPhysTriData(H3DRes level_mesh_res, int& tri_count)
{


	auto vert_cnt = h3dGetResParamI(level_mesh_res, H3DGeoRes::GeometryElem, 0, H3DGeoRes::GeoVertexCountI);
	auto  ff = (float*)h3dMapResStream(level_mesh_res, H3DGeoRes::GeometryElem, 0, H3DGeoRes::GeoVertPosStream, true, false);
	float *vertices = new float[3 * vert_cnt];
	memcpy(vertices, ff, sizeof(float)*vert_cnt * 3);
	h3dUnmapResStream(level_mesh_res);


	auto ind_cnt = h3dGetResParamI(level_mesh_res, H3DGeoRes::GeometryElem, 0, H3DGeoRes::GeoIndexCountI);
	int *indices = new int[ind_cnt];

	if (h3dGetResParamI(level_mesh_res, H3DGeoRes::GeometryElem, 0, H3DGeoRes::GeoIndices16I))
	{
		unsigned short* tb = (unsigned short*)h3dMapResStream(level_mesh_res, H3DGeoRes::GeometryElem, 0, H3DGeoRes::GeoIndexStream, true, false);
		auto tmp = new unsigned short[ind_cnt];
		memcpy(tmp, tb, sizeof(unsigned short)*ind_cnt);
		h3dUnmapResStream(level_mesh_res);
		int last = 0;
		for (int i = 0; i < ind_cnt; i++)
		{
			indices[i] = tmp[i];
			last = tmp[i];
		}
		delete[] tmp;
	}
	else
	{
		unsigned short* tb = (unsigned short*)h3dMapResStream(level_mesh_res, H3DGeoRes::GeometryElem, 0, H3DGeoRes::GeoIndexStream, true, false);
		auto tmp = new unsigned int[ind_cnt];
		memcpy(indices, tb, sizeof(unsigned int)*ind_cnt);
		h3dUnmapResStream(level_mesh_res);
	}
	tri_count = ind_cnt / 3;
	auto result_data = (float*)malloc(tri_count * 3 * 3 * sizeof(float));
	auto result_ind = 0;
	for (int i = 0; i < ind_cnt; i += 3)
	{
		float face[3][3];
		for (int j = 0; j < 3; j++) {
			int index = indices[i + j] * 3;
			result_data[result_ind++] = vertices[index + 0];
			result_data[result_ind++] = vertices[index + 1];
			result_data[result_ind++] = vertices[index + 2];
		}
	}
	return result_data;
}

bool physicEngine::trace(Vector3df from, Vector3df to,Vector3df& hit)
{
	Triangle tris[1024];
	s32 sz;
	Aabbox3d aabb(from);
	aabb.addInternalPoint(to);
	getTriangles(tris, 1024, sz, aabb, nullptr);
	Vector3df hitpoint;

	f32 min_dist;
	bool first = true;
	Triangle* min_dist_tri = nullptr;
	for (int i = 0; i < sz; i++)
	{
		if (colTriLine(&tris[i], &from, &to, hitpoint))
		{
			f32 dist = (hitpoint - from).getLengthSQ();
			if (first || dist < min_dist)
			{
				min_dist_tri = &tris[i];
				min_dist = dist;
				hit = hitpoint;
				first = false;
			}
		}
	}
	return min_dist_tri != nullptr;
}

void physicEngine::reset()
{
	pawns.clear();
	sphereProjectiles.clear();
	NodeCount = 0;
	delete g_octree_root;
	g_octree_root = nullptr;
}