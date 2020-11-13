/****************************************************************************
 *
 *      matrix4.cc: Transformation matrix implementation
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

#include "geometry/matrix4.h"
#include "common/logger.h"

BEGIN_YAFARAY

Matrix4::Matrix4(const float init): invalid_(0)
{
	for(int i = 0; i < 4; i++)
		for(int j = 0; j < 4; ++j)
		{
			if(i == j)
				matrix_[i][j] = init;
			else
				matrix_[i][j] = 0;
		}
}

Matrix4::Matrix4(const Matrix4 &source): invalid_(source.invalid_)
{
	for(int i = 0; i < 4; i++)
		for(int j = 0; j < 4; j++)
			matrix_[i][j] = source[i][j];
}

Matrix4::Matrix4(const float source[4][4])
{
	for(int i = 0; i < 4; i++)
		for(int j = 0; j < 4; j++)
			matrix_[i][j] = source[i][j];
}

Matrix4::Matrix4(const double source[4][4])
{
	for(int i = 0; i < 4; i++)
		for(int j = 0; j < 4; j++)
			matrix_[i][j] = source[i][j];
}

#define SWAP(m,i,b)\
for(int j=0; j<4; ++j) std::swap(m[i][j], m[b][j])

#define RES(m,i,b,f)\
for(int j=0; j<4; ++j) m[i][j] -= m[b][j]*f

#define DIV(m,i,f)\
for(int j=0; j<4; ++j) m[i][j] /= f

Matrix4 &Matrix4::inverse()
{
	Matrix4 iden(1);
	for(int i = 0; i < 4; ++i)
	{
		float max = 0;
		int ci = 0;
		for(int k = i; k < 4; ++k)
		{
			if(std::abs(matrix_[k][i]) > max)
			{
				max = std::abs(matrix_[k][i]);
				ci = k;
			}
		}
		if(max == 0)
		{
			Y_ERROR << "Serious error inverting matrix" << YENDL;
			Y_ERROR << i << YENDL;
			invalid_ = 1;
		}
		SWAP(matrix_, i, ci); SWAP(iden, i, ci);
		float factor = matrix_[i][i];
		DIV(matrix_, i, factor); DIV(iden, i, factor);
		for(int k = 0; k < 4; ++k)
		{
			if(k != i)
			{
				factor = matrix_[k][i];
				RES(matrix_, k, i, factor); RES(iden, k, i, factor);
			}
		}
	}
	for(int i = 0; i < 4; ++i)
		for(int j = 0; j < 4; ++j)
			matrix_[i][j] = iden[i][j];
	return *this;
}

Matrix4 &Matrix4::transpose()
{
	for(int i = 0; i < 3; ++i)
		for(int j = i + 1; j < 4; ++j)
			std::swap(matrix_[i][j], matrix_[j][i]);
	return *this;
}

void Matrix4::translate(float dx, float dy, float dz)
{
	Matrix4 aux(1);

	aux[0][3] = dx;
	aux[1][3] = dy;
	aux[2][3] = dz;

	(*this) = aux * (*this);
}

void Matrix4::rotateZ(float degrees)
{
	float temp = degrees;
	temp = fmod(temp, (float)360.0);
	if(temp < 0) temp = ((float)360.0) - temp;
	temp = temp * (M_PI / ((float)180));

	Matrix4 aux(1);
	aux[0][0] = math::cos(temp);
	aux[0][1] = -math::sin(temp);
	aux[1][0] = math::sin(temp);
	aux[1][1] = math::cos(temp);

	(*this) = aux * (*this);
}

void Matrix4::rotateX(float degrees)
{
	float temp = degrees;
	temp = fmod(temp, (float)360.0);
	if(temp < 0) temp = ((float)360.0) - temp;
	temp = temp * (M_PI / ((float)180));

	Matrix4 aux(1);
	aux[1][1] = math::cos(temp);
	aux[1][2] = -math::sin(temp);
	aux[2][1] = math::sin(temp);
	aux[2][2] = math::cos(temp);

	(*this) = aux * (*this);
}

void Matrix4::rotateY(float degrees)
{
	float temp = degrees;
	temp = fmod(temp, (float)360.0);
	if(temp < 0) temp = ((float)360.0) - temp;
	temp = temp * (M_PI / ((float)180));

	Matrix4 aux(1);
	aux[0][0] = math::cos(temp);
	aux[0][2] = math::sin(temp);
	aux[2][0] = -math::sin(temp);
	aux[2][2] = math::cos(temp);

	(*this) = aux * (*this);
}


void Matrix4::scale(float sx, float sy, float sz)
{
	matrix_[0][0] *= sx; matrix_[1][0] *= sx; matrix_[2][0] *= sx;
	matrix_[0][1] *= sy; matrix_[1][1] *= sy; matrix_[2][1] *= sy;
	matrix_[0][2] *= sz; matrix_[1][2] *= sz; matrix_[2][2] *= sz;
}


void Matrix4::identity()
{
	for(int i = 0; i < 4; i++)
		for(int j = 0; j < 4; ++j)
		{
			if(i == j)
				matrix_[i][j] = 1.0;
			else
				matrix_[i][j] = 0;
		}
}

std::ostream &operator << (std::ostream &out, const Matrix4 &m)
{
	out << "/ " << m[0][0] << " " << m[0][1] << " " << m[0][2] << " " << m[0][3] << " \\\n";
	out << "| " << m[1][0] << " " << m[1][1] << " " << m[1][2] << " " << m[1][3] << " |\n";
	out << "| " << m[2][0] << " " << m[2][1] << " " << m[2][2] << " " << m[2][3] << " |\n";
	out << "\\ " << m[3][0] << " " << m[3][1] << " " << m[3][2] << " " << m[3][3] << " /\n";
	return out;
}

END_YAFARAY
