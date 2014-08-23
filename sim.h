#ifndef MESH_SIM_H
#define MESH_SIM_H

#include "he_mesh.h"

class MeshSim
{
public:
	MeshSim(float t=0.f):threshold2(t){}
	void simplify(he_mesh* mesh,int target_faces);
private:
	void preprocess(he_mesh* mesh,void* heap_); // compute Q for all vertices and build pairs heap

	he_mesh* mesh;
	float threshold2;
};

#endif