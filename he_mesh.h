#ifndef _HALFEDGE_MESH_H_
#define _HALFEDGE_MESH_H_

#include "meshloader.h"
#include "mlist.h"

// CLOCKWISE

struct he_edge;

struct he_vert
{
	he_vert():edge(0){}//,index(-1)

	float3 pos;
	float3 normal;
	he_edge* edge; // one edge that starts from vert
	//int index; // used when write to file
	void* extra_data; // store anything you want
};

struct he_face
{
	he_face():edge(0){}
	//float3 normal;
	he_edge* edge; // one bordering half-edge
	//int index;
};
struct he_edge
{
	he_edge():vert_from(0),vert_to(0),pair(0),face(0),next(0){}
	he_vert* vert_from,*vert_to;
	he_edge* pair;
	he_face* face; // bordering face
	he_edge* next;

	// debug
	int id_from,id_to;
};

class he_mesh
{
public:
	he_mesh(){}
	bool construct(const MeshData& _mesh);
	void free()
	{
		verts.clear();
		edges.clear();
		faces.clear();
	}

	mlist<he_vert> verts;
	mlist<he_edge> edges;
	mlist<he_face> faces;
};

#endif