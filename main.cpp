#include "he_mesh.h"
#include "timer.h"

#include <stdio.h>
#include <math.h>

MeshData mesh1()
{
//   3  /\
//   2 /__\ 1
//     \  /
//   0  \/

	float3* p=new float3[4];
	int* vi=new int[2*3];

	p[0]=float3(0.f,-1.f,0.f);
	p[1]=float3(1.f,0.f,0.f);
	p[2]=float3(-1.f,0.f,0.f);
	p[3]=float3(0.f,1.f,0.f);

	// counter-clockwise
	vi[0]=0;vi[1]=1;vi[2]=2;
	vi[3]=2;vi[4]=1;vi[5]=3;

	return MeshData(2,4,4,0,p,0,0,vi);
}

MeshData mesh2()
{

	//   2 ____ 1
	//     \  /
	//      \/0 
	//      /\
	//   3 /__\ 4

	float3* p=new float3[5];
	int* vi=new int[2*3];

	p[0]=float3(0.f,0.f,0.f);
	p[1]=float3(1.f,1.f,0.f);
	p[2]=float3(-1.f,1.f,0.f);
	p[3]=float3(-1.f,-1.f,0.f);
	p[4]=float3(1.f,-1.f,0.f);

	// counter-clockwise
	vi[0]=0;vi[1]=1;vi[2]=2;
	vi[3]=0;vi[4]=3;vi[5]=4;

	return MeshData(2,5,5,0,p,0,0,vi);
}

MeshData mesh3()
{

	//   2 ____ 1
	//     \  /
	//    0 \/_/|6
	//      /\ \|5
	//   3 /__\ 4

	float3* p=new float3[7];
	int* vi=new int[3*3];

	p[0]=float3(0.f,0.f,0.f);
	p[1]=float3(1.f,1.f,0.f);
	p[2]=float3(-1.f,1.f,0.f);
	p[3]=float3(-1.f,-1.f,0.f);
	p[4]=float3(1.f,-1.f,0.f);
	p[5]=float3(2.f,-1.f,0.f);
	p[6]=float3(2.f,1.f,0.f);

	// counter-clockwise
	vi[0]=0;vi[1]=1;vi[2]=2;
	vi[3]=0;vi[4]=3;vi[5]=4;
	vi[6]=0;vi[7]=5;vi[8]=6;

	return MeshData(3,7,7,0,p,0,0,vi);
}

int main()
{




	MeshData mesh_data=mesh3();

	//MeshData mesh_data=MeshLoader::off("bunny.off");

	he_mesh mesh;
	tic();
	mesh.construct(mesh_data);
	toc();

	mesh_data.clear();
	return 0;
}