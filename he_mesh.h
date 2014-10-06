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

	void remove_edge_related(he_edge* edge,bool keep_vfrom)
	{
		if(!edge) return;
		if(edge->face)
		{
			he_edge* outer1=edge->next->pair;
			he_edge* outer2=edge->prev->pair;

			if(keep_vfrom&&edge->vert_from->edge==edge)
			{
				edge->vert_from->edge=outer2;
			}
			else if(!keep_vfrom&&edge->vert_to->edge==edge->next)
			{
				edge->vert_to->edge=outer2;
			}

			//if(edge->prev->vert_from->edge==edge->prev)
			//	edge->prev->vert_from->edge=outer1;
			if(outer1->vert_from->edge==outer2->pair)
				outer1->vert_from->edge=outer1;

			outer1->pair=outer2;
			outer2->pair=outer1;

			this->faces.remove(edge->face);
			edge->face=0;

			this->edges.remove(edge->prev);
			this->edges.remove(edge->next);
		}
		else // boundary edge, update next
		{
			if(keep_vfrom&&edge->vert_from->edge==edge)
				edge->vert_from->edge=edge->next;

			edge->prev->next=edge->next;
			edge->next->prev=edge->prev;
		}
		this->edges.remove(edge);//???
	}

	mlist<he_vert> verts;
	mlist<he_edge> edges;
	mlist<he_face> faces;
};

#endif