#include "he_mesh.h"

#include <set>
#include <map>
#include <vector>
using namespace std;

template<class T>
struct T2
{
	T2(T* _a):x(_a[0]),y(_a[1]){}
	T2(T _a1,T _a2):x(_a1),y(_a2){}
	T2(T _a1):x(_a1),y(_a1){}
	T2():x(0),y(0){}

	T2<T> reverse(){return T2(y,x);}
	const T2<T> reverse()const{return T2(y,x);}

	T2<T>& operator+=(const T2& b)
	{
		x+=b.x;
		y+=b.y;
		return *this;
	}

	bool operator<(const T2& b)const
	{
		return this->x==b.x?this->y<b.y:this->x<b.x;
	}

	T x,y;
};

typedef T2<int> int2;

__forceinline float3 get_tagent(const float3& normal)
{
	float3 c1 = cross(normal, float3(0.0f, 0.0f, 1.0f)); 
	float3 c2 = cross(normal, float3(0.0f, 1.0f, 0.0f)); 

	return length2(c1) > length2(c2)? c1: c2;
}

// on from's tangent plane
float angle_on_tangent_plane(he_vert* from,he_vert* to)
{
 	float3 axis=get_tagent(from->normal);
 	float3 v=to->pos-from->pos;
 	float a=acosf(dot(normalize(v),axis));

	return dot(from->normal,cross(axis,v))>=0.f?
		a:2*M_PI-a;
}

struct BoundaryEdge
{
	BoundaryEdge(he_edge* e):edge(e),from(0),angle(0.f)
	{
		if(e)
		{
			from=e->vert_from;

			angle=angle_on_tangent_plane(e->vert_from,e->vert_to);
		}
	}
	he_edge* edge;
	he_vert* from;
	float angle;
};
struct BoundaryEdgeCMP
{
	bool operator()(const BoundaryEdge& first, const BoundaryEdge& second) const 
	{
		// large angle first because we do clockwise searching, find first one that is less than target
		return (first.from==second.from?first.angle>second.angle:first.from<second.from);
	}
};

bool he_mesh::construct(const MeshData& _mesh)
{
	free();

	vector<he_vert*> vert_map(_mesh.nverts,0);
	map<int2,he_edge*> edge_map;
	typedef map<int2,he_edge*>::iterator edge_map_itr;
	
	int* vi=_mesh.vi;
	for(int fi=0;fi<_mesh.ntris;++fi,vi+=3)
	{
		he_face* face=faces.append();

		he_edge* _e3[3];
		for(int i=0;i<3;++i)
		{
			// assume manifold, then he_edge is always created
			he_edge* e=edges.append();_e3[i]=e;
			if(!face->edge) face->edge=e;

			// debug
			e->id_from=vi[i];e->id_to=vi[(i+1)%3];

			int idx=vi[i];
			he_vert* vfrom=vert_map[idx];
			if(!vfrom)
			{
				vfrom=vert_map[vi[i]]=verts.append();
				vfrom->pos=_mesh.p[idx];
				vfrom->normal=_mesh.n[idx];
			}
			if(!vfrom->edge) vfrom->edge=e;

			idx=vi[(i+1)%3];
			he_vert* vto=vert_map[idx];
			if(!vto)
			{
				vto=vert_map[idx]=verts.append();
				// no edge
				vto->pos=_mesh.p[idx];
				vto->normal=_mesh.n[idx];
			}
			
			e->face=face;
			e->vert_from=vfrom;
			e->vert_to=vto;

			edge_map[int2(vi[i],idx)]=e;
		}
		_e3[0]->next=_e3[1];
		_e3[1]->next=_e3[2];
		_e3[2]->next=_e3[0];
	}

	

	set<BoundaryEdge,BoundaryEdgeCMP> boundary_edges;
	typedef set<BoundaryEdge,BoundaryEdgeCMP>::iterator BEdge_Itr;

	// pair the edges
	for(edge_map_itr itr=edge_map.begin();itr!=edge_map.end();++itr)
	{
		if(itr->second) // set to 0
		{
			edge_map_itr findit=edge_map.find(itr->first.reverse());
			if(findit!=edge_map.end())
			{
				itr->second->pair=findit->second;
				findit->second->pair=itr->second;
				findit->second=0; // no need to precess when encountered
			}
			else
			{
				// add boundary edges
				he_edge* bedge=edges.append();
				itr->second->pair=bedge;
				bedge->pair=itr->second;
				bedge->vert_from=vert_map[itr->first.y];
				bedge->vert_to=vert_map[itr->first.x];
				// no face
				// next filled below

				// debug
				bedge->id_from=itr->second->id_to;
				bedge->id_to=itr->second->id_from;

				boundary_edges.insert(BoundaryEdge(bedge));
			}
		}
		// else its pair already processed
	}

	// create and connect boundary half edges


	// ASSUME triangle verts are listed in CLOCKWISE

	// MY algorithm:
	// 1. build the b-he map, sort by start point, then by the edge angle
	// 2. for each b-he, find all other b-hes that begin with its end point in the map, find the b-he CLOCKWISE
	for(BEdge_Itr bitr=boundary_edges.begin();bitr!=boundary_edges.end();++bitr)
	{
		BoundaryEdge lower_edge(0),upper_edge(0);

		// normal range is [0,360]
		lower_edge.angle=361.f;
		upper_edge.angle=-1.f;

		lower_edge.from=bitr->edge->vert_to;
		upper_edge.from=bitr->edge->vert_to;

		BEdge_Itr itr_begin=boundary_edges.lower_bound(lower_edge),
			itr_end=boundary_edges.lower_bound(upper_edge);

		if(itr_begin==itr_end)
		{
			// wtf??? impossible
		}
		if(std::distance(itr_begin,itr_end)==1) // only one
		{
			bitr->edge->next=itr_begin->edge;
		}
		else // face vertices listed in counter-clockwise, search clockwise, find the first edge
		{
			float cur_angle=angle_on_tangent_plane(bitr->edge->vert_to,bitr->edge->vert_from);

			// todo: *(itr_end-1)>cur_angle, then all > cur_angle

			BEdge_Itr i=itr_begin;
			for(;i!=itr_end;++i)
			{
				if(i->angle<=cur_angle) // == wtf!!
				{
					break;
				}
			}
			if(i==itr_end) // then it's the first one
				i=itr_begin;

			bitr->edge->next=i->edge;
		}
	}

	return true;
}

bool he_mesh::dumpOFF(const char* fpath)
{
	FILE* fp=fopen(fpath,"w");
	fprintf(fp,"OOF\n");
	fprintf(fp,"%d %d %d\n",verts.size(),faces.size(),edges.size());

	map<he_vert*,int> vert2id;
	int id=0;

	for(he_vert* vert=verts.begin();vert;vert=verts.next())
	{
		fprintf(fp,"%f %f %f\n",vert->pos.x,vert->pos.y,vert->pos.z);
		vert2id[vert]=id++;
	}

	for(he_face* face=faces.begin();face;face=faces.next())
	{
		fprintf(fp,"3 %d %d %d\n",
			vert2id[face->edge->vert_from],
			vert2id[face->edge->vert_to],
			vert2id[face->edge->next->vert_to]);
	}

	return true;
}