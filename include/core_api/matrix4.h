/****************************************************************************
 *
 * 			matrix4.h: Transformation matrix api 
 *      This is part of the yafray package
 *      Copyright (C) 2002  Alejandro Conty Estévez
 *
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public
 *      License as published by the Free Software Foundation; either
 *      version 2.1 of the License, or (at your option) any later version.
 *
 *      This library is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      Lesser General Public License for more details.
 *
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library; if not, write to the Free Software
 *      Foundation,Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *      
 */
#ifndef __MATRIX4_H
#define __MATRIX4_H

#include<yafray_config.h>

#include<iostream>
//#include<cmath>
#include "vector3d.h"

__BEGIN_YAFRAY

class YAFRAYCORE_EXPORT matrix4x4_t
{
public:
	matrix4x4_t() {};
	matrix4x4_t(const PFLOAT init);
	matrix4x4_t(const matrix4x4_t & source);
	matrix4x4_t(const float source[4][4]);
	matrix4x4_t(const double source[4][4]);
	~matrix4x4_t() {};
	/*! attention, this function can cause the matrix to become invalid!
		unless you are sure the matrix is invertible, check invalid() afterwards! */
	matrix4x4_t & inverse();
	matrix4x4_t & transpose();
	void identity();
	void translate(PFLOAT dx,PFLOAT dy,PFLOAT dz);
	void rotateX(PFLOAT degrees);
	void rotateY(PFLOAT degrees);
	void rotateZ(PFLOAT degrees);
	void scale(PFLOAT sx, PFLOAT sy, PFLOAT sz);
	int invalid() const { return _invalid; }
	const PFLOAT * operator [] (int i) const { return matrix[i]; }
	PFLOAT * operator [] (int i) { return matrix[i]; }

	// only used for rotation matrix in mesh, sets matrix[i] from a vector (elem0-2) and a float (elem3)
	// maybe a bit clumsy this
	void setRow(int i, const vector3d_t &v, PFLOAT e3) { matrix[i][0]=v.x;  matrix[i][1]=v.y;  matrix[i][2]=v.z;  matrix[i][3]=e3; }
	void setColumn(int i, const vector3d_t &v, PFLOAT e3) { matrix[0][i]=v.x;  matrix[1][i]=v.y;  matrix[2][i]=v.z;  matrix[3][i]=e3; }

protected:

	PFLOAT  matrix[4][4];
	int _invalid;
};

inline matrix4x4_t  operator * (const matrix4x4_t &a,const matrix4x4_t &b)
{
	matrix4x4_t aux;
	
	for(int i=0;i<4;i++)
		for(int k=0;k<4;k++)
		{
			aux[i][k]=0;
			for(int j=0;j<4;j++)
				aux[i][k]+=a[i][j]*b[j][k];
		}
	return aux;
}

inline vector3d_t  operator * (const matrix4x4_t &a, const vector3d_t &b)
{
	return vector3d_t(a[0][0]*b.x+a[0][1]*b.y+a[0][2]*b.z,
										a[1][0]*b.x+a[1][1]*b.y+a[1][2]*b.z,
										a[2][0]*b.x+a[2][1]*b.y+a[2][2]*b.z);
}

inline point3d_t  operator * (const matrix4x4_t &a, const point3d_t &b)
{
	return  point3d_t(a[0][0]*b.x+a[0][1]*b.y+a[0][2]*b.z+a[0][3],
										a[1][0]*b.x+a[1][1]*b.y+a[1][2]*b.z+a[1][3],
										a[2][0]*b.x+a[2][1]*b.y+a[2][2]*b.z+a[2][3]);
}
//matrix4x4_t rayToZ(const point3d_t &from,const vector3d_t & ray);
YAFRAYCORE_EXPORT std::ostream & operator << (std::ostream &out,matrix4x4_t &m);

__END_YAFRAY

#endif
