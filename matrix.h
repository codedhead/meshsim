#ifndef MATRIX_H_
#define MATRIX_H_

#include <string.h>


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

template<class T>
struct T4
{
	T4(T* _a):x(_a[0]),y(_a[1]),z(_a[2]),w(_a[3]){}
	T4(T _a1,T _a2,T _a3,T _a4):x(_a1),y(_a2),z(_a3),w(_a4){}
	T4(T _a1):x(_a1),y(_a1),z(_a1),w(_a1){}
	T4():x(0),y(0),z(0),w(0){}

	union{
		struct{
			T x,y,z,w;
		};
		struct{
			T a,b,c,d;
		};
	};
};

typedef T4<float> float4;

class Matrix44
{
public:
	float m[16]; // column major

	Matrix44(int i=1/*dummy*/)
	{
		loadI();
	}

	Matrix44(const float* mm)
	{
		if(!mm)
			memset(m,0,sizeof(float)*16);
		else
			memcpy(m,mm,sizeof(float)*16);
	}

	void loadI()
	{
		m[0]=1.0f;m[4]=0.0f;m[8]=0.0f;m[12]=0.0f;
		m[1]=0.0f;m[5]=1.0f;m[9]=0.0f;m[13]=0.0f;
		m[2]=0.0f;m[6]=0.0f;m[10]=1.0f;m[14]=0.0f;
		m[3]=0.0f;m[7]=0.0f;m[11]=0.0f;m[15]=1.0f;
	}

#define M_(i,j) m[((j)*4+(i))]
	void loadSymmetric(const float* mm)
	{
		M_(0,0)=*mm++;
		M_(1,0)=M_(0,1)=*mm++;
		M_(2,0)=M_(0,2)=*mm++;
		M_(3,0)=M_(0,3)=*mm++;
		M_(1,1)=*mm++;
		M_(2,1)=M_(1,2)=*mm++;
		M_(3,1)=M_(1,3)=*mm++;
		M_(2,2)=*mm++;
		M_(3,2)=M_(2,3)=*mm++;
		M_(3,3)=*mm++;
	}
#undef M_
	
	bool isAffine() const
	{
		return m[3]==0.f&&m[7]==0.f&&m[11]==0.f&&m[15]==1.f;
	}

	// element-wise add
	Matrix44& operator+=(const Matrix44& b)
	{
		for(int i=0;i<16;++i)
			m[i]+=b.m[i];
		return *this;
	}
	

	Matrix44 operator*(const Matrix44& b)const
	{
		Matrix44 ret;
		mul(this->m,b.m,ret.m);
		return ret;
	}
	Matrix44& operator*=(const Matrix44& b)
	{
		mul(this->m,b.m,this->m);
		return *this;
	}

	// m right_mul n
	static void mul(const float* m,const float* n,float* dst)
	{
		float res[16];
		res[0]=m[0]*n[0]+m[4]*n[1]+m[8]*n[2]+m[12]*n[3];
		res[4]=m[0]*n[4]+m[4]*n[5]+m[8]*n[6]+m[12]*n[7];
		res[8]=m[0]*n[8]+m[4]*n[9]+m[8]*n[10]+m[12]*n[11];
		res[12]=m[0]*n[12]+m[4]*n[13]+m[8]*n[14]+m[12]*n[15];
		res[1]=m[1]*n[0]+m[5]*n[1]+m[9]*n[2]+m[13]*n[3];
		res[5]=m[1]*n[4]+m[5]*n[5]+m[9]*n[6]+m[13]*n[7];
		res[9]=m[1]*n[8]+m[5]*n[9]+m[9]*n[10]+m[13]*n[11];
		res[13]=m[1]*n[12]+m[5]*n[13]+m[9]*n[14]+m[13]*n[15];
		res[2]=m[2]*n[0]+m[6]*n[1]+m[10]*n[2]+m[14]*n[3];
		res[6]=m[2]*n[4]+m[6]*n[5]+m[10]*n[6]+m[14]*n[7];
		res[10]=m[2]*n[8]+m[6]*n[9]+m[10]*n[10]+m[14]*n[11];
		res[14]=m[2]*n[12]+m[6]*n[13]+m[10]*n[14]+m[14]*n[15];
		res[3]=m[3]*n[0]+m[7]*n[1]+m[11]*n[2]+m[15]*n[3];
		res[7]=m[3]*n[4]+m[7]*n[5]+m[11]*n[6]+m[15]*n[7];
		res[11]=m[3]*n[8]+m[7]*n[9]+m[11]*n[10]+m[15]*n[11];
		res[15]=m[3]*n[12]+m[7]*n[13]+m[11]*n[14]+m[15]*n[15];
		memcpy(dst,res,sizeof(float)*16);
	}

	// only for affine matrix
	Matrix44 inverse() const
	{
		Matrix44 ret;
		assert(isAffine());
		affine_inv(ret.m);
		return ret;
	}

	float affine_det() const
	{
		return m[0]*m[5]*m[10]+m[1]*m[6]*m[8]+m[2]*m[4]*m[9]
		-m[2]*m[5]*m[8]-m[0]*m[6]*m[9]-m[1]*m[4]*m[10];
	}

	// todo: http://stackoverflow.com/questions/2624422/efficient-4x4-matrix-inverse-affine-transform
	// and inv(M)*b

	// treat it as affine, if need check, check outside
	void affine_inv(float* dst) const // possibly not const, see memcpy
	{
		float det=affine_det();
		assert(det!=0.f&&!isnan(det));
		float invDet=1.f/det;
		float dstm[16];

		dstm[0]=invDet*(m[5]*m[10]-m[6]*m[9]);
		dstm[5]=invDet*(m[0]*m[10]-m[2]*m[8]);
		dstm[10]=invDet*(m[5]*m[0]-m[1]*m[4]);
		dstm[1]=-invDet*(m[1]*m[10]-m[2]*m[9]);
		dstm[2]=invDet*(m[1]*m[6]-m[2]*m[5]);
		dstm[4]=-invDet*(m[4]*m[10]-m[6]*m[8]);
		dstm[6]=-invDet*(m[0]*m[6]-m[2]*m[4]);
		dstm[8]=invDet*(m[4]*m[9]-m[5]*m[8]);
		dstm[9]=-invDet*(m[0]*m[9]-m[1]*m[8]);

		dstm[3]=dstm[7]=dstm[11]=0.0;dstm[15]=1.0;

		dstm[12]=-(dstm[0]*m[12]+dstm[4]*m[13]+dstm[8]*m[14]);
		dstm[13]=-(dstm[1]*m[12]+dstm[5]*m[13]+dstm[9]*m[14]);
		dstm[14]=-(dstm[2]*m[12]+dstm[6]*m[13]+dstm[10]*m[14]);
		memcpy(dst,dstm,sizeof(float)*16);
	}

	Matrix44& operator=(Matrix44& n)
	{
		memcpy(m,n.m,sizeof(float)*16);
		return *this;
	}

};



#endif