#pragma once
/****************************************************************************
 *
 *      matrix4.h: Transformation matrix api
 *      This is part of the libYafaRay package
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
#ifndef YAFARAY_MATRIX4_H
#define YAFARAY_MATRIX4_H

#include "common/yafaray_common.h"
#include "vector.h"
#include <iostream>

BEGIN_YAFARAY

class Matrix4
{
	public:
		Matrix4() = default;
		Matrix4(const float init);
		Matrix4(const Matrix4 &source);
		Matrix4(const float source[4][4]);
		/*! attention, this function can cause the matrix to become invalid!
			unless you are sure the matrix is invertible, check invalid() afterwards! */
		Matrix4 &inverse();
		Matrix4 &transpose();
		void identity();
		void translate(float dx, float dy, float dz);
		void rotateX(float degrees);
		void rotateY(float degrees);
		void rotateZ(float degrees);
		void scale(float sx, float sy, float sz);
		bool invalid() const { return invalid_; }
		const float *operator [](int i) const { return matrix_[i]; }
		float *operator [](int i) { return matrix_[i]; }
		void setVal(int row, int col, float val) { matrix_[row][col] = val; };
		float getVal(int row, int col) { return matrix_[row][col]; }

	private:
		template<typename T> static void swapRows(T& matrix, int row_a, int row_b);
		template<typename T> static void subtractScaledRow(T& matrix, int row_a, int row_b, float factor);
		template<typename T> static void divideRow(T& matrix, int row, float divisor);

		float matrix_[4][4];
		bool invalid_ = false;
};

inline Matrix4 operator * (const Matrix4 &a, const Matrix4 &b)
{
	Matrix4 aux;
	for(int i = 0; i < 4; i++)
		for(int k = 0; k < 4; k++)
		{
			aux[i][k] = 0;
			for(int j = 0; j < 4; j++)
				aux[i][k] += a[i][j] * b[j][k];
		}
	return aux;
}

inline Vec3 operator * (const Matrix4 &a, const Vec3 &b)
{
	return { a[0][0] * b.x_ + a[0][1] * b.y_ + a[0][2] * b.z_,
	                  a[1][0] * b.x_ + a[1][1] * b.y_ + a[1][2] * b.z_,
	                  a[2][0] * b.x_ + a[2][1] * b.y_ + a[2][2] * b.z_ };
}

inline Point3 operator * (const Matrix4 &a, const Point3 &b)
{
	return {a[0][0] * b.x_ + a[0][1] * b.y_ + a[0][2] * b.z_ + a[0][3],
	                  a[1][0] * b.x_ + a[1][1] * b.y_ + a[1][2] * b.z_ + a[1][3],
	                  a[2][0] * b.x_ + a[2][1] * b.y_ + a[2][2] * b.z_ + a[2][3] };
}

//matrix4x4_t rayToZ(const point3d_t &from,const vector3d_t & ray);
std::ostream &operator << (std::ostream &out, const Matrix4 &m);

END_YAFARAY

#endif
