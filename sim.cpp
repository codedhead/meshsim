#include "sim.h"

#include <vector>
#include <set>
#include <map>
#include <algorithm>
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
	//vert_pair(he_vert* p1,he_vert* p2):v1(p1),v2(p2){	compute_error();	}
	//he_vert* v1,*v2;

	vert_pair(he_edge* e):edge(e){compute_error();}
	he_edge* edge;
	
	float3 optimal_pos;
	float3 optimal_nrm; // corresponding normal
	float error;

	/* TODO
	When considering
	a possible contraction, we compare the normal of each neighboring
	face before and after the contraction. If the normal flips, that 
	contraction can be either heavily penalized or disallowed.
	*/

	void compute_error()
	{
		he_vert* v1=edge->vert_from,*v2=edge->vert_to;

		Matrix44 Qsum(*(Matrix44*)(v1->extra_data));
		Qsum+=*(Matrix44*)(v2->extra_data);
		
		bool can_inv;
		Matrix44 inv=Qsum.affine_inv(can_inv);

		if(can_inv)
		{
			optimal_pos=inv.last_col();
			optimal_nrm=v1->normal;// todo: ???;
			error=Qsum.quadric(optimal_pos);
		}
		else // select from v1, v2 or mid point
		{
			float3 vm=0.5f*(v1->pos+v2->pos);

			optimal_pos=vm;
			optimal_nrm=normalize(v1->normal+v2->normal);
			error=Qsum.quadric(optimal_pos);

			float e=Qsum.quadric(v1->pos);
			if(e<error)
			{
				error=e;
				optimal_pos=v1->pos;
				optimal_nrm=v1->normal;
			}

			e=Qsum.quadric(v2->pos);
			if(e<error)
			{
				error=e;
				optimal_pos=v2->pos;
				optimal_nrm=v2->normal;
			}
		}
	}

	// keep v1, remove v2
	void movetoOptimal()
	{
		he_vert* v1=edge->vert_from,*v2=edge->vert_to;

		// update v1 as the new vert
		v1->pos=optimal_pos;
		v1->normal=optimal_nrm;
		*(Matrix44*)(v1->extra_data) += *(Matrix44*)(v2->extra_data);

		// update edges_with_v2 to point to new vert
		he_edge* v2_edge=v2->edge;
		if(v2_edge)
		{
			do 
			{
				he_edge* next_edge=v2_edge->pair->next;
				he_vert* v3=v2_edge->vert_to;

				if(v3!=v1) // keep this edge untouched
				{
					v2_edge->pair->vert_to=v1;
					v2_edge->vert_from=v1;
				}

				v2_edge=next_edge;
			} while (v2_edge!=v2->edge);
		}
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
				//if(v1<v2)
				if(v1->id<v2->id)
					heap_->insert(vert_pair(edge));

				edge=edge->pair->next;
			} while (edge!=v1->edge);
		}

		//if(heap_->size()>0) break;

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

typedef PairHeap::iterator HeapItr;
struct HeapItrCMP
{
	bool operator()(const HeapItr& a,const HeapItr& b)
	{
		return a->edge<b->edge;
	}
};

void MeshSim::simplify(he_mesh* mesh,int target_faces)
{
	PairHeap heap_;	

	map<he_vert*,vector<HeapItr> > vert2pair; // if pair contains the vert

	preprocess(mesh,&heap_);

	int hi=0;
	for(HeapItr itr=heap_.begin();itr!=heap_.end();++itr)
	{
		vert2pair[itr->edge->vert_from].push_back(itr);
		vert2pair[itr->edge->vert_to].push_back(itr);

		printf("\r %d/%d  ",++hi,heap_.size());
	}
	printf("#\n");

	while(!heap_.empty()&&mesh->faces.size()>target_faces)
	{
		vert_pair min_cost_pair=*(heap_.begin());
		he_vert* v1=min_cost_pair.edge->vert_from;
		he_vert* v2=min_cost_pair.edge->vert_to;
		min_cost_pair.movetoOptimal();

		vector<HeapItr>& pairs_with_v1=vert2pair[v1];
		vector<HeapItr>& pairs_with_v2=vert2pair[v2];
		set<HeapItr,HeapItrCMP> union_itr;
		union_itr.insert(pairs_with_v1.begin(),pairs_with_v1.end());
		union_itr.insert(pairs_with_v2.begin(),pairs_with_v2.end());
		/*vector<HeapItr> union_itr;
		sort(pairs_with_v1.begin(),pairs_with_v1.end());
		sort(pairs_with_v2.begin(),pairs_with_v2.end());
		set_union(pairs_with_v1.begin(),pairs_with_v1.end(),
				pairs_with_v2.begin(),pairs_with_v2.end(),
				union_itr.begin());
		*/

		for(set<HeapItr,HeapItrCMP>::iterator it=union_itr.begin();it!=union_itr.end();++it)
		{
			HeapItr heapitr=*it;

			he_edge* edge=heapitr->edge;
			he_vert* v = (edge->vert_from!=v1&&edge->vert_from!=v2)?edge->vert_from:edge->vert_to;
			if(v==v1||v==v2) continue;

			vector<HeapItr>& related_v=vert2pair[v];
			for(int j=0;j<related_v.size();++j)
			{
				if(related_v[j]==heapitr)
				{
					related_v.erase(related_v.begin()+j);
					break;
				}
			}

			heap_.erase(heapitr);
		}
		heap_.erase(heap_.begin());

		// only support edge contraction currently
		he_edge* edge_v1v2=min_cost_pair.edge;
		
		mesh->remove_edge_related(edge_v1v2->pair,false); // keep v1 (pair's v_to)
		mesh->remove_edge_related(edge_v1v2,true); // keep v1 (v_from)

		mesh->verts.remove(v2);


		vert2pair.erase(v2);
		pairs_with_v1.clear();
		// reinsert all edges with v1
		{
			he_edge* v1_edge=v1->edge;
			if(v1_edge)
			{
				do 
				{
					HeapItr new_itr= (v1_edge->vert_from->id<v1_edge->vert_to->id)?
						heap_.insert(vert_pair(v1_edge)):
						heap_.insert(vert_pair(v1_edge->pair));

					vert2pair[v1_edge->vert_from].push_back(new_itr);
					vert2pair[v1_edge->vert_to].push_back(new_itr);

					v1_edge=v1_edge->pair->next;
				} while (v1_edge!=v1->edge);
			}
		}

		printf("\r cur faces: %d    ",mesh->faces.size());
	}
	printf("#\n");
}