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
		he_vert* new_vert=mesh_sim.verts.append();
		new_vert->pos=optimal_pos;

		change_pointto(v1,new_vert);
		change_pointto(v2,new_vert);

		return new_vert;
	}
};

struct vert_pair_CMP
{
	bool operator()(const vert_pair& a,const vert_pair& b)
	{
		return a.error<b.error;
	}
};

typedef multiset<vert_pair,vert_pair_CMP> PairHeap;

he_edge* is_edge(he_vert* v1,he_vert* v2)
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
	}
}

void MeshSim::simplify(he_mesh* mesh,int target_faces)
{
	PairHeap heap_;
	typedef PairHeap::iterator HeapItr;

	map<he_vert*,vector<HeapItr> > vert2pair; // if pair contains the vert

	preprocess(mesh,&heap_);

	for(HeapItr itr=heap_.begin();itr!=heap_.end();++itr)
	{
		vert2pair[itr->v1].push_back(itr);
		vert2pair[itr->v2].push_back(itr);
	}

	while(mesh->faces.size()>target_faces)
	{
		HeapItr min_cost_pair=heap_.begin();
		he_vert* v1=min_cost_pair->v1,*v2=min_cost_pair->v2;

		

		he_vert* new_vert=min_cost_pair->contract();
		vector<HeapItr> pairs_with_newvert;

		heap_.erase(min_cost_pair);

		vector<HeapItr>& has_v1_pairs=vert2pair[v1];
		for(int i=0,iLen=has_v1_pairs.size();i<iLen;++i)
		if(has_v1_pairs[i]!=min_cost_pair)// the only conflict?
		{
			he_vert* old_v2=has_v1_pairs[i]->v2;
			
			heap_.erase(has_v1_pairs[i]);

			pairs_with_newvert.push_back(heap_.insert(vert_pair(new_vert,old_v2)));
		}


		vector<HeapItr>& has_v2_pairs=vert2pair[v2];
		for(int i=0,iLen=has_v2_pairs.size();i<iLen;++i)
		if(has_v2_pairs[i]!=min_cost_pair)// the only conflict?
		{
			he_vert* old_v1=has_v2_pairs[i]->v1;

			heap_.erase(has_v2_pairs[i]);

			// do a linear search to find if already pushed
			bool duplicated=false;
			for(int j=0,jLen=has_v1_pairs.size();j<jLen;++j)
			{
				if(old_v1==has_v1_pairs[j]->v2)
				{
					duplicated=true;
					// todo: remove the collapsed face
					break;
				}
			}

			if(!duplicated)
				pairs_with_newvert.push_back(heap_.insert(vert_pair(old_v1,new_vert)));
		}

		vert2pair[new_vert]=pairs_with_newvert;

		// remove
		
		// can ignore the edges?
		he_edge* the_edge=is_edge(v1,v2);
		if(the_edge) mesh->edges.remove(the_edge);
		the_edge=is_edge(v2,v1);
		if(the_edge) mesh->edges.remove(the_edge);

		mesh->verts.remove(v1);
		mesh->verts.remove(v2);

		vert2pair.erase(v1);
		vert2pair.erase(v2);
	}

}