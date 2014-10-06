#ifndef _HALFEDGE_MESH_H_
#define _HALFEDGE_MESH_H_

#include "meshloader.h"
#include "mlist.h"

// CLOCKWISE

struct he_edge;

struct he_vert
{
	he_vert():edge(0),extra_data(0),id(-1){}//,index(-1)

	float3 pos;
	float3 normal;
	he_edge* edge; // one edge that starts from vert
	//int index; // used when write to file
	void* extra_data; // store anything you want
	int id;
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
	he_edge():vert_from(0),vert_to(0),pair(0),face(0),prev(0),next(0){}
	he_vert* vert_from,*vert_to;
	he_edge* pair;
	he_face* face; // bordering face
	he_edge* prev,*next;
};

inline he_edge* is_edge(he_vert* v1,he_vert* v2)
{
	// checking one side suffice
	he_edge* edge=v1->edge;
	if(!edge) return 0;
	do 
	{
		if(v2==edge->vert_to) return edge;
		edge=edge->pair->next;
	} while (edge!=v1->edge);

	return 0;
}

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

	bool dumpOFF(const char* fpath);

	// if v_from->v_to is_edge, replace the edge
	he_edge* merge_duplicate(he_edge* edge,he_vert* v_from,he_vert* v_to)
	{
		he_edge* exist_edge=is_edge(v_from,v_to);
		if(exist_edge)
		{
			if(edge->face&&edge->face->edge==edge)
				edge->face->edge=exist_edge;
			//edges.remove(edge);
			return exist_edge;
		}
		else
			return edge;
	}

	mlist<he_vert> verts;
	mlist<he_edge> edges;
	mlist<he_face> faces;
};

#endif