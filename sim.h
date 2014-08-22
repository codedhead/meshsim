#ifndef MESH_SIM_H
#define MESH_SIM_H

#include "he_mesh.h"

class MeshSim
{
public:
	void simplify(he_mesh* mesh);
private:
	void preprocess(he_mesh* mesh); // compute Q for all vertices and build pairs heap
	void run();

	he_mesh* mesh;

	float threshold2;
};

#endif