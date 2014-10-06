#include "he_mesh.h"
#include "sim.h"
#include "timer.h"

#include <stdio.h>
#include <math.h>

#include <string>
using namespace std;

MeshData plane(int rows,int cols)
{
#define LIN_IDX(_i,_j) ((_i)*cols+(_j))

	// n*n vert
	float3* p=new float3[rows*cols];
	for(int i=0;i<rows;++i)
	{
		for(int j=0;j<cols;++j)
		{
			p[LIN_IDX(i,j)]=float3(i,j,0.f);
		}
	}


	int* vi=new int[2*(rows-1)*(cols-1)*3];
	int* pvi=vi;
	for(int i=0;i<rows-1;++i)
	{
		for(int j=0;j<cols-1;++j)
		{
			*pvi++=LIN_IDX(i,j);
			*pvi++=LIN_IDX(i+1,j);
			*pvi++=LIN_IDX(i,j+1);

			*pvi++=LIN_IDX(i+1,j);
			*pvi++=LIN_IDX(i+1,j+1);
			*pvi++=LIN_IDX(i,j+1);
		}
	}

	return MeshData(2*(rows-1)*(cols-1),rows*cols,rows*cols,0,p,0,0,vi);
}

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

void print_usage()
{
	printf("meshsim input_file [-faces %%d] [-ratio %%f]\n");
}

// TRY
// boundary in half edge
// forward energy
int main(int argc,char** argv)
{
	if(argc<2)
	{
		print_usage();
		return 0;
	}
	string input_file=argv[1];

	// parse options
	int faces=0;
	float ratio=0.5f;	
	for(int i=2;i<argc;++i)
	{
		if(!strcmp(argv[i],"-faces"))
		{
			if(i+1<argc)
			{
				faces=atoi(argv[++i]);
			}
		}
		else if(!strcmp(argv[i],"-ratio"))
		{
			if(i+1<argc)
			{
				ratio=atof(argv[++i]);
				if(ratio<=0.f||ratio>=1.f)
					ratio=0.5f;
			}
		}
	}

	he_mesh mesh;
	
	//MeshData mesh_data=mesh1();
	//MeshData mesh_data=plane(1000,1000);
	MeshData mesh_data;
	string fext=input_file.substr(input_file.length()-4,4);
	if(fext==".off")
		mesh_data=MeshLoader::off(input_file.c_str());
	else if(fext==".obj")
		mesh_data=MeshLoader::obj(input_file.c_str());
	else
	{
		printf("cannot load file format: %s\n",input_file.c_str());
		return 0;
	}
	
	tic();
	mesh.construct(mesh_data);
	toc("convert to half-edge mesh");

	//mesh.dumpOFF("input.off");

	mesh_data.clear();

	if(faces==0||faces>=mesh.faces.size())
	{
		faces=ratio*mesh.faces.size();
	}
	printf("input faces: %d, target faces: %d\n",mesh.faces.size(),faces);

	MeshSim mesh_simer;
	tic();
	mesh_simer.simplify(&mesh,faces);//mesh.faces.size()*0.01);
	toc("simplify");

	char fsuffix[50];
	sprintf(fsuffix,"_%d.off",mesh.faces.size());
	string output_file=input_file.substr(0,input_file.length()-4)+fsuffix;
	mesh.dumpOFF(output_file.c_str());

	return 0;
}