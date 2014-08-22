#include "meshloader.h"

#include "3rdparty/objloader/objLoader.h"


#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <map>
#include <sstream>
#include <vector>

using namespace std;

// code from mitsuba
/**
	* \brief Numerically well-behaved routine for computing the angle
	* between two unit direction vectors
	*
	* This should be used wherever one is tempted to compute the
	* arc cosine of a dot product!
	*
	* Proposed by Don Hatch at
	* http://www.plunk.org/~hatch/rightway.php
	*/
__forceinline float unitAngle(const float3 &u, const float3 &v) {
	if (dot(u, v) < 0.f)
		return M_PI - 2.f * asinf(0.5f * length(v+u));
	else
		return 2.f * asinf(0.5f * length(v-u));
}

float3* compute_normals(const float3* verts,const int* indices,int nverts,int nindices)
{
	float3* norms=new float3[nverts];
	int invalid_n=0;
	printf("Computing normal...\n");

	memset(norms,0,sizeof(float3)*nverts);

	// from mitsuba source code
	/* Well-behaved vertex normal computation based on
			   "Computing Vertex Normals from Polygonal Facets"
			   by Grit Thuermer and Charles A. Wuethrich,
			   JGT 1998, Vol 3 */
	/**/
	for (int tri = 0; tri+2 < nindices; tri+=3)
	{
		float3 n(0.f);
		for (int i=0; i<3; ++i) 
		{
			const float3 &v0 = verts[indices[tri+i]];
			const float3 &v1 = verts[indices[tri+(i+1)%3]];
			const float3 &v2 = verts[indices[tri+(i+2)%3]];
			float3 sideA(v1-v0), sideB(v2-v0); // v0v1, v0v2 or v0v1, v1v2

			if (i==0)
			{
				n = cross(sideA, sideB);
				float len = length(n);
				if (len == 0.f)
					break;
				n *= 1.f/len;
			}
			float angle = unitAngle(normalize(sideA), normalize(sideB));
			norms[indices[tri+i]] += angle*n;
		}
	}

	for (int i = 0; i < nverts; ++i)
	{
		float len=length(norms[i]);
		if(len!=0.f)
			norms[i]*= 1.f/len;
		else
			/* Choose some bogus value */
			norms[i] = float3(1.f, 0.f, 0.f);

		//Assert(!is0(norms[i]));
	}

	if(invalid_n>0)
	{
		printf("%d invalid normals\n",invalid_n);
	}
	printf("#\n");

	
	return norms;
}

// copy address
MeshData::MeshData(int ntriangles, int nv, int nn, int nt,
	float3 *P, float3 *N, float *UV,
	int *viptr, int *niptr, int *tiptr, int *miptr)
	:ntris(ntriangles),nverts(nv),nnorms(nn),ntexs(nt),
	vi(viptr),ni(niptr),ti(tiptr),mi(miptr),
	n(N),uv(UV),p(P)
{
	assert(ntris&&viptr&&p);

	if(!n) n=compute_normals(p,vi,nverts,ntris*3);

	// normally normal index same as vert index
	if(!ni) ni=vi;
	if(!ti) ti=vi;
}
MeshData::MeshData()
{
	memset(this,0,sizeof(MeshData));
}
void MeshData::clear()
{
	if(ni!=vi&&ni) delete[] ni;
	if(ti!=vi&&ti) delete[] ti;
	if(vi) delete[] vi;
	if(mi) delete[] mi;

	if(p) delete[] p;
	if(n) delete[] n;
	if(uv) delete[] uv;

	memset(this,0,sizeof(MeshData));
}

namespace MeshLoader
{
	MeshData empty_mesh;


	MeshData off(const char* filename)
	{
		// Open file
		FILE *fp;
		if (!(fp = fopen(filename, "r"))) {
			printf("-MeshLoader::off \n\tUnable to open file %s", filename);
			return empty_mesh;
		}

		// Read file
		int nverts = 0,vert_read=0;
		int nfaces = 0,face_read=0;
		int line_count = 0;
		char buffer[1024];

		float3* verts=NULL;
		int* faces=NULL;
		float3 vt;
		
		int i;
		while (fgets(buffer, 1023, fp)) {
			// Increment line counter
			line_count++;

			// Skip white space
			char *bufferp = buffer;
			while (isspace(*bufferp)) bufferp++;

			// Skip blank lines and comments
			if (*bufferp == '#') continue;
			if (*bufferp == '\0') continue;

			// Check section
			if (nverts == 0) {
				// Read header 
				if (!strstr(bufferp, "OFF")) {
					// Read mesh counts
					int nedges = 0;
					if ((sscanf(bufferp, "%d%d%d", &nverts, &nfaces, &nedges) != 3) || (nverts == 0)) {
						printf("-MeshLoader::off \n\tSyntax error reading header on line %d in file %s", line_count, filename);
						fclose(fp);
						return empty_mesh;
					}
					verts=new float3[nverts];
					faces=new int[nfaces*3];
				}
			}
			else if (vert_read < nverts) {
				// Read vertex coordinates
				if (sscanf(bufferp, "%f%f%f", &(vt.x), &(vt.y), &(vt.z)) != 3) {
					printf("-MeshLoader::off \n\tSyntax error with vertex coordinates on line %d in file %s", line_count, filename);
					goto READ_OFF_FAIL;
				}
				verts[vert_read++]=vt;
			}
			else if (face_read < nfaces) {
				// Get next face
				int fverts=0,ivert;

				// Read number of vertices in face 
				bufferp = strtok(bufferp, " \t");
				if (bufferp) fverts = atoi(bufferp);
				else {
					printf("-MeshLoader::off \n\tSyntax error with face on line %d in file %s", line_count, filename);
					goto READ_OFF_FAIL;
				}
				assert(fverts==3);
				if(fverts!=3)
				{
					printf("-MeshLoader::off \n\tFail read OFF file, not triangle polygon found on line %d in file %s",line_count,filename);
					goto READ_OFF_FAIL;
				}

				for (i = 0; i < 3; i++) {
					bufferp = strtok(NULL, " \t");
					if (bufferp) ivert = atoi(bufferp);
					else
					{
						printf("-MeshLoader::off \n\tSyntax error with face on line %d in file %s", line_count, filename);
						goto READ_OFF_FAIL;
					}
					faces[face_read*3+i]=ivert;
				}
				++face_read;
			}
			else {
				// Should never get here
				printf("-MeshLoader::off \t\nFound extra text starting at line %d in file %s", line_count, filename);
				break;
			}
		}

		// Close file
		fclose(fp);
		
		// Check whether read all faces
		if (nverts != vert_read) {
			printf("-MeshLoader::off \n\tExpected %d verts, but read only %d verts in file %s", nverts,vert_read, filename);
			goto READ_OFF_FAIL;
		}
		if (nfaces != face_read) {
			printf("-MeshLoader::off \n\tExpected %d faces, but read only %d faces in file %s", nfaces, face_read, filename);
			goto READ_OFF_FAIL;
		}
		
		return MeshData(
			nfaces,	nverts, 0, 0,
			verts, NULL, NULL,
			faces
			);

READ_OFF_FAIL:
		if(verts) delete [] verts;
		if(faces) delete [] faces;
		fclose(fp);
		return empty_mesh;
	}

	
	inline void copy3_double2float(float* dst,double* src)
	{
		*dst++=(float)(*src++);
		*dst++=(float)(*src++);
		*dst++=(float)(*src++);
	}
	inline void copy3_double2float(float3* dst,double* src)
	{
		dst->x=(float)(*src++);
		dst->y=(float)(*src++);
		dst->z=(float)(*src++);
	}

	
	MeshData obj(const char* filename)
	{
		objLoader objloader;
		if(!objloader.load((char*)filename))
		{
			printf("-MeshLoader::obj \n\tfailed to load model %s",filename);
			return empty_mesh;
		}

		int ntris=objloader.faceCount,nv=objloader.vertexCount,nn=objloader.normalCount,nt=objloader.textureCount;
		int *ni=NULL,*vi=NULL,*ti=NULL,*mi=NULL;
		float3 *p=NULL,*n=NULL;
		float *uv=NULL;

		assert(nv>0);
		p=new float3[nv];
		for(int i=0;i<nv;++i)
		{
			copy3_double2float(p+i,objloader.vertexList[i]->e);
		}
		if(nn>0)
		{
			n=new float3[nn];
			for(int i=0;i<nn;++i)
			{
				copy3_double2float(n+i,objloader.normalList[i]->e);
			}
		}
		if(nt>0)
		{
			uv=new float[nt*2];
			for(int i=0;i<nt;++i)
			{
				uv[i+i]=(float)objloader.textureList[i]->e[0];
				uv[i+i+1]=(float)objloader.textureList[i]->e[1];
			}
		}

		struct __copy_int3
		{
			int x[3];
		};
		// index from 0, -1 for no index
#define __COPY_INT3(dst,src) *((__copy_int3*)(dst))=*((__copy_int3*)(src));

		// index from 1
		//#define __COPY_INT3(dst,src) (dst)[0]=(src)[0]-1;(dst)[1]=(src)[1]-1;(dst)[2]=(src)[2]-1;

		// mandatory
		vi=new int[ntris*3];
		for(int i=0;i<ntris;++i)
		{
			obj_face* f=objloader.faceList[i];
			assert(f->vertex_count==3);
			__COPY_INT3(vi+3*i,f->vertex_index);
		}

		// has normal&index
		if(nn>0&&objloader.faceList[0]->normal_index[0]>=0)
		{
			ni=new int[ntris*3];
			for(int i=0;i<ntris;++i)
			{
				obj_face* f=objloader.faceList[i];
				assert(f->vertex_count==3);
				__COPY_INT3(ni+3*i,f->normal_index);
			}
		}
		
		// has tex&index
		if(nt>0&&objloader.faceList[0]->texture_index[0]>=0)
		{
			ti=new int[ntris*3];
			for(int i=0;i<ntris;++i)
			{
				obj_face* f=objloader.faceList[i];
				assert(f->vertex_count==3);
				__COPY_INT3(ti+3*i,f->texture_index);
			}
		}

		return MeshData(ntris,nv,nn,nt,p,n,uv,vi,ni,ti,mi);
	}

}