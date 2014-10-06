#include "sim.h"

#include <vector>
#include <set>
#include <map>
using namespace std;


// counter-clockwise order
__forceinline float4 plane(const float3& p1,const float3& p2,const float3& p3)
{
	float3 v2=p2-p1,v3=p3-p1;
	float3 n=normalize(cross(v2,v3));
	// n*(x-p1)=0
	// => n.x*x + n.y*y + n.z*z -(n.x*p1.x+n.y*p1.y+n.z*p1.z)=0;
	return float4(n.x,n.y,n.z,
		-(n.x*p1.x+n.y*p1.y+n.z*p1.z)
		);
}

__forceinline float4 plane(const he_vert* v1,const he_vert* v2,const he_vert* v3)
{
	return plane(v1->pos,v2->pos,v3->pos);
}


// todo: set, no order
struct vert_pair
{
	vert_pair(he_vert* p1,he_vert* p2):v1(p1),v2(p2){
		compute_error();
	}
	//vert_pair(he_edge* e):edge(e),v1(0),v2(0){}
	//he_edge* edge; // if this pair is edge
	he_vert* v1,*v2;
	float3 optimal_pos;
	float error;

	void compute_error()
	{
		Matrix44 Qsum(*(Matrix44*)(v1->extra_data));
		Qsum+=*(Matrix44*)(v2->extra_data);
		
		bool can_inv;
		Matrix44 inv=Qsum.affine_inv(can_inv);

		if(can_inv)
		{
			optimal_pos=inv.last_row();
			error=Qsum.quadric(optimal_pos);
		}
		else // select from v1, v2 or mid point
		{
			float3 vm=0.5f*(v1->pos+v2->pos);

			optimal_pos=vm;
			error=Qsum.quadric(optimal_pos);

			float e=Qsum.quadric(v1->pos);
			if(e<error)
			{
				error=e;
				optimal_pos=v1->pos;
			}

			e=Qsum.quadric(v2->pos);
			if(e<error)
			{
				error=e;
				optimal_pos=v2->pos;
			}
		}
		// normal dont care
	}
	/*
	void change_pointto(he_vert* oldv,he_vert* newv)const
	{
		// find all edges from oldv
		he_edge* edge=oldv->edge;
		if(edge)
		{
			do 
			{
				edge->vert_from=newv;
				edge=edge->pair->next;
			} while (edge!=oldv->edge);
		}

		// find all edges to oldv
		edge=oldv->edge->pair;
		if(edge)
		{
			do
			{
				edge->vert_to=newv;
				edge=edge->next->pair; // todo: check
			} while (edge!=oldv->edge->pair);
		}
	}

	he_vert* contract(MeshSim& mesh_sim)const
	{
// 		he_vert* new_vert=mesh_sim.mesh->verts.append();
// 		new_vert->pos=optimal_pos;
// 
// 		change_pointto(v1,new_vert);
// 		change_pointto(v2,new_vert);
// 
// 		return new_vert;
		return 0;
	}
	*/
};

struct vert_pair_CMP
{
	bool operator()(const vert_pair& a,const vert_pair& b)
	{
		return a.error<b.error;
	}
};

typedef multiset<vert_pair,vert_pair_CMP> PairHeap;

void MeshSim::preprocess(he_mesh* mesh,void* hh)
{
	PairHeap* heap_=(PairHeap*)hh;

	// store initial Q matrix
	for(he_vert* vert=mesh->verts.begin();vert;vert=mesh->verts.next())
	{
		he_edge* edge=vert->edge;
		if(edge)
		{
			Matrix44* Ksum=new Matrix44((const float*)0); // Ksum is also symmetric
			// iterate surrounding faces
			do 
			{
				if(edge->face) // may be boundary edge
				{
					Matrix44 Kp;

					float4 pl=plane(edge->vert_from,edge->vert_to,edge->next->vert_to);

					float mat_lower_tri[]={
						pl.a*pl.a,
						pl.a*pl.b,
						pl.a*pl.c,
						pl.a*pl.d,

						pl.b*pl.b,
						pl.b*pl.c,
						pl.b*pl.d,

						pl.c*pl.c,
						pl.c*pl.d,

						pl.d*pl.d,
					};

					Kp.loadSymmetric(mat_lower_tri);

					*Ksum += Kp;
				}
				edge=edge->pair->next;
			} while (edge!=vert->edge);

			vert->extra_data=Ksum;
		}
	}

	// select valid pairs
	for(he_vert* v1=mesh->verts.begin();v1;v1=mesh->verts.next())
	{
		// traverse edges
		he_edge* edge=v1->edge;
		if(edge)
		{
			do 
			{
				he_vert* v2=edge->vert_to;

				// avoid duplicate
				if(v1<v2)
					heap_->insert(vert_pair(v1,v2));

				edge=edge->pair->next;
			} while (edge!=v1->edge);
		}

		/*
		// find verts within threshold
		// todo: how to do better
		he_vert* v2=mesh->verts.begin(SECOND_ITR,v1);
		v2=mesh->verts.next(SECOND_ITR); // !=v1
		for(;v2;v2=mesh->verts.next(SECOND_ITR))
		{
			if(!is_edge(v1,v2)&&length2(v1->pos-v2->pos)<threshold2)
			{
				heap_->insert(vert_pair(v1,v2));
			}
		}
		*/
	}
}

void MeshSim::simplify(he_mesh* mesh,int target_faces)
{
	PairHeap heap_;

	preprocess(mesh,&heap_);

	while(mesh->faces.size()>target_faces)
	{
		vert_pair min_cost_pair=*(heap_.begin());heap_.erase(heap_.begin());
		he_vert* v1=min_cost_pair.v1,*v2=min_cost_pair.v2;
				
		// update v1 as the new vert
		v1->pos=min_cost_pair.optimal_pos;
		// todo //v1->normal=?;

		// update edges_with_v2 to point to new vert
		he_edge* v2_edge=v2->edge;
		if(v2_edge)
		{
			do 
			{
				he_edge* next_edge=v2_edge->pair->next;

				he_vert* v3=v2_edge->vert_to;
				if(v3!=v1)
				{
					// v3->v2, check if already v3->v1
					//mesh->merge_duplicate(v2_edge->pair,v3,v1)->vert_to=v1;
					// v2->v3, check if already v1->v3
					//mesh->merge_duplicate(v2_edge,v1,v3)->vert_from=v1;

					v2_edge->pair->vert_to=v1;
					v2_edge->vert_from=v1;

				}
			
				v2_edge=next_edge;
			} while (v2_edge!=v2->edge);
		}
		

		he_edge* edge_v1v2=is_edge(v1,v2);
		if(!edge_v1v2) // only support edge contraction currently
			continue;
		
		if(edge_v1v2->pair&&edge_v1v2->pair->face)
		{
			he_edge* outer1=edge_v1v2->pair->next->pair;
			he_edge* outer2=edge_v1v2->pair->prev->pair;
			outer1->pair=outer2;
			outer2->pair=outer1;

			if(edge_v1v2->vert_from->edge==outer1->pair)
				edge_v1v2->vert_from->edge=outer2;
			if(outer1->vert_from->edge==outer2->pair)
				outer1->vert_from->edge=outer1;

			mesh->faces.remove(edge_v1v2->pair->face);
			edge_v1v2->pair->face=0;

			mesh->edges.remove(edge_v1v2->pair->prev);
			mesh->edges.remove(edge_v1v2->pair->next);
		}
		else // boundary edge, update next
		{
			edge_v1v2->pair->prev->next=edge_v1v2->pair->next;
		}

		if(edge_v1v2->face)
		{
			he_edge* outer1=edge_v1v2->next->pair;
			he_edge* outer2=edge_v1v2->prev->pair;
			outer1->pair=outer2;
			outer2->pair=outer1;
				
			if(edge_v1v2->vert_from->edge==edge_v1v2)
				edge_v1v2->vert_from->edge=outer2;
			if(outer1->vert_from->edge==outer2->pair)
				outer1->vert_from->edge=outer1;

			mesh->faces.remove(edge_v1v2->face);
			edge_v1v2->face=0;

			mesh->edges.remove(edge_v1v2->prev);
			mesh->edges.remove(edge_v1v2->next);
		}
		else // boundary edge, update next
		{
			edge_v1v2->prev->next=edge_v1v2->next;
		}

		mesh->edges.remove(edge_v1v2->pair);
		mesh->edges.remove(edge_v1v2);

		mesh->verts.remove(v2);

		// todo:  erase all edges with v1,v2, update them with new_v1
	}

}