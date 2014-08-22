#ifndef _MESHLOADER_H_
#define _MESHLOADER_H_

#include "matrix.h"

#include <math.h>

#define M_PI       3.14159265358979323846f

class objLoader;


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