#include "he_mesh.h"
#include "sim.h"
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

void write_test_mesh(int row,int col)
{
	int vct=(row+1)*(col+1),fct=2*row*col;
	FILE *fp;
	char fname[256];
	sprintf(fname,"plane%dx%d.off",row,col);
	if (!(fp = fopen(fname, "w"))) {
		fprintf(stderr, "Unable to write file %s\n", "plane.off");
		return;
	}

	fprintf(fp,"OFF\n");
	fprintf(fp,"%d %d %d\n",vct,fct,0);
	for(int i=0;i<=row;++i)
		for(int j=0;j<=col;++j)
		{
			fprintf(fp,"%g %g %g\n",j*1.0/col,i*1.0/row,0.0f);
		}
		for(int i=0;i<row;++i)
			for(int j=0;j<col;++j)
			{
				fprintf(fp,"3 %d %d %d\n",i*(col+1)+j,(i+1)*(col+1)+j,i*(col+1)+j+1);
				fprintf(fp,"3 %d %d %d\n",i*(col+1)+j+1,(i+1)*(col+1)+j,(i+1)*(col+1)+j+1);
			}
			fclose(fp);
}

int main()
{
	//write_test_mesh(10,10);
	//MeshData mesh_data=MeshLoader::off("plane10x10.off");


	// boundary in half edge
	// forward energy


	MeshData mesh_data=mesh1();

	//MeshData mesh_data=MeshLoader::off("bunny.off");

	he_mesh mesh;
	tic();
	mesh.construct(mesh_data);
	toc();

	mesh_data.clear();

	MeshSim mesh_simer;
	mesh_simer.simplify(&mesh,1);

	return 0;
}