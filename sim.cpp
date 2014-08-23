#include "sim.h"

#include <vector>
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
	//vert_pair(he_vert* p1,he_vert* p2):v1(p1),v2(p2){}
	vert_pair(he_edge* e):edge(e),v1(0),v2(0){}
	he_edge* edge; // if this pair is edge
	he_vert* v1,*v2;
	he_vert v_bar;
	float error;

	void compute_error()
	{
		Matrix44 Qsum(*(Matrix44*)(v1->extra_data));
		Qsum+=*(Matrix44*)(v1->extra_data);

		Matrix44 inv=Qsum.affineInv();
		// extract inv's last column
		v_bar->pos=inv.row(3);

		error=Qsum.quadric(v_bar);
	}

	void change_pointto(he_vert* oldv,he_vert* newv)
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

	he_vert* contract()
	{
		he_vert* new_vert;

		change_pointto(v1,new_vert);
		change_pointto(v2,new_vert);

	}
};


void MeshSim::preprocess(he_mesh* mesh)
{
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

					he_edge* e=edge->face->edge;

					float4 pl=plane(e->vert_from,e->vert_to,e->next->vert_to);

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

	vector<vert_pair> vert_pairs;

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

				float error=

				vert_pairs.push_back(vert_pair(v1,v2));

				edge=edge->pair.next;
			} while (edge!=v1.edge);
		}

		// find verts within threshold
		// todo: how to do better
		he_vert* v2=mesh->verts.begin(1,v1);
		v2=mesh->verts.next(1); // !=v1
		for(;v2;v2=mesh->verts.next(1))
		{
			if(dot(v1->pos,v2->pos)<threshold2)
			{
				vert_pairs.push_back(vert_pair(v1,v2));
			}
		}
	}
}

void MeshSim::simplify(he_mesh* mesh,int target_faces)
{
	set<vert_pair> heap_;

	preprocess(mesh,heap_);

	while(mesh->faces.size()>target_faces)
	{
		heap_.begin();
	}

}