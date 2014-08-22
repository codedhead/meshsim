#ifndef _MESHLOADER_H_
#define _MESHLOADER_H_

#include <math.h>

#define M_PI       3.14159265358979323846f

class objLoader;

template<class T>
struct T3
{
	T3(T* _a):x(_a[0]),y(_a[1]),z(_a[2]){}
	T3(T _a1,T _a2,T _a3):x(_a1),y(_a2),z(_a3){}
	T3(T _a1):x(_a1),y(_a1),z(_a1){}
	T3():x(0),y(0),z(0){}

	T3& operator+=(const T3& b)
	{
		x+=b.x;
		y+=b.y;
		z+=b.z;
		return *this;
	}
	T3& operator-=(const T3& b)
	{
		x-=b.x;
		y-=b.y;
		z-=b.z;
		return *this;
	}
	T3& operator*=(float s)
	{
		x*=s;y*=s;z*=s;
		return *this;
	}


	T x,y,z;
};

typedef T3<float> float3;

__forceinline float3 operator+(const float3& a,const float3& b)
{
	return float3(a.x+b.x,a.y+b.y,a.z+b.z);
}

__forceinline float3 operator-(const float3& a,const float3& b)
{
	return float3(a.x-b.x,a.y-b.y,a.z-b.z);
}

__forceinline float3 operator*(float s,const float3& a)
{
	return float3(s*a.x,s*a.y,s*a.z);
}

__forceinline float dot(const float3& a,const float3& b)
{
	return a.x*b.x+a.y*b.y+a.z*b.z;
}

__forceinline float length2(const float3& a)
{
	return dot(a,a);
}

__forceinline float length(const float3& a)
{
	float l2=length2(a);
	return l2==0.f?0.f:sqrtf(l2);
}

__forceinline float3 normalize(const float3& a)
{
	float l=length(a);
	return l>0.f?(1.f/l)*a:float3(0.f);
}

__forceinline float3 cross(const float3& a, const float3& b)
{ 
	return float3(a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x); 
}

struct MeshData
{
	MeshData();

	//typedef unsigned int MeshDataIndexType;

	// the array pointers are directly copied, no allocation
	MeshData(
		int ntriangles, int nv, int nn, int nt,
		float3 *P, float3 *N, float *UV,
		int *viptr, int *niptr=0, int *tiptr=0, int *miptr=0);

	void clear();

	int ntris, 
		nverts, nnorms, ntexs;
	int *vi,*ni,*ti; // *3*ntris
	int *mi; // material index * ntris
	float3 *p, *n;float *uv;
};

namespace MeshLoader
{
	MeshData obj(const char* fname);
	MeshData off(const char* fname);
}


#endif