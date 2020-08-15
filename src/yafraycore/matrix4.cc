/****************************************************************************
 *
 * 			matrix4.cc: Transformation matrix implementation
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

#include <core_api/matrix4.h>
using namespace std;
#include <cstdlib>
#include <algorithm>

__BEGIN_YAFRAY

matrix4x4_t::matrix4x4_t(const float init): _invalid(0)
{
	for(int i = 0; i < 4; i++)
		for(int j = 0; j < 4; ++j)
		{
			if(i == j)
				matrix[i][j] = (float)init;
			else
				matrix[i][j] = 0;
		}
}

matrix4x4_t::matrix4x4_t(const matrix4x4_t &source): _invalid(source._invalid)
{
	for(int i = 0; i < 4; i++)
		for(int j = 0; j < 4; j++)
			matrix[i][j] = source[i][j];
}

matrix4x4_t::matrix4x4_t(const float source[4][4])
{
	for(int i = 0; i < 4; i++)
		for(int j = 0; j < 4; j++)
			matrix[i][j] = source[i][j];
}

matrix4x4_t::matrix4x4_t(const double source[4][4])
{
	for(int i = 0; i < 4; i++)
		for(int j = 0; j < 4; j++)
			matrix[i][j] = source[i][j];
}

#define SWAP(m,i,b)\
for(int j=0; j<4; ++j) std::swap(m[i][j], m[b][j])

#define RES(m,i,b,f)\
for(int j=0; j<4; ++j) m[i][j] -= m[b][j]*f

#define DIV(m,i,f)\
for(int j=0; j<4; ++j) m[i][j] /= f

matrix4x4_t &matrix4x4_t::inverse()
{
	matrix4x4_t iden(1);
	for(int i = 0; i < 4; ++i)
	{
		float max = 0;
		int ci = 0;
		for(int k = i; k < 4; ++k)
		{
			if(std::fabs(matrix[k][i]) > max)
			{
				max = std::fabs(matrix[k][i]);
				ci = k;
			}
		}
		if(max == 0)
		{
			cout << "Error mu grave invirtiendo matriz\n";
			cout << i << "\n";
			_invalid = 1;
		}
		SWAP(matrix, i, ci); SWAP(iden, i, ci);
		float factor = matrix[i][i];
		DIV(matrix, i, factor); DIV(iden, i, factor);
		for(int k = 0; k < 4; ++k)
		{
			if(k != i)
			{
				factor = matrix[k][i];
				RES(matrix, k, i, factor); RES(iden, k, i, factor);
			}
		}
	}
	for(int i = 0; i < 4; ++i)
		for(int j = 0; j < 4; ++j)
			matrix[i][j] = iden[i][j];
	return *this;
}

matrix4x4_t &matrix4x4_t::transpose()
{
	for(int i = 0; i < 3; ++i)
		for(int j = i + 1; j < 4; ++j)
			std::swap(matrix[i][j], matrix[j][i]);
	return *this;
}

void matrix4x4_t::translate(float dx, float dy, float dz)
{
	matrix4x4_t aux(1);

	aux[0][3] = dx;
	aux[1][3] = dy;
	aux[2][3] = dz;

	(*this) = aux * (*this);
}

void matrix4x4_t::rotateZ(float degrees)
{
	float temp = degrees;
	temp = fmod(temp, (float)360.0);
	if(temp < 0) temp = ((float)360.0) - temp;
	temp = temp * (M_PI / ((float)180));

	matrix4x4_t aux(1);
	aux[0][0] = fCos(temp);
	aux[0][1] = -fSin(temp);
	aux[1][0] = fSin(temp);
	aux[1][1] = fCos(temp);

	(*this) = aux * (*this);
}

void matrix4x4_t::rotateX(float degrees)
{
	float temp = degrees;
	temp = fmod(temp, (float)360.0);
	if(temp < 0) temp = ((float)360.0) - temp;
	temp = temp * (M_PI / ((float)180));

	matrix4x4_t aux(1);
	aux[1][1] = fCos(temp);
	aux[1][2] = -fSin(temp);
	aux[2][1] = fSin(temp);
	aux[2][2] = fCos(temp);

	(*this) = aux * (*this);
}

void matrix4x4_t::rotateY(float degrees)
{
	float temp = degrees;
	temp = fmod(temp, (float)360.0);
	if(temp < 0) temp = ((float)360.0) - temp;
	temp = temp * (M_PI / ((float)180));

	matrix4x4_t aux(1);
	aux[0][0] = fCos(temp);
	aux[0][2] = fSin(temp);
	aux[2][0] = -fSin(temp);
	aux[2][2] = fCos(temp);

	(*this) = aux * (*this);
}


void matrix4x4_t::scale(float sx, float sy, float sz)
{
	matrix[0][0] *= sx;  matrix[1][0] *= sx;  matrix[2][0] *= sx;
	matrix[0][1] *= sy;  matrix[1][1] *= sy;  matrix[2][1] *= sy;
	matrix[0][2] *= sz;  matrix[1][2] *= sz;  matrix[2][2] *= sz;
}


void matrix4x4_t::identity()
{
	for(int i = 0; i < 4; i++)
		for(int j = 0; j < 4; ++j)
		{
			if(i == j)
				matrix[i][j] = 1.0;
			else
				matrix[i][j] = 0;
		}
}

ostream &operator << (ostream &out, matrix4x4_t &m)
{
	out << "/ " << m[0][0] << " " << m[0][1] << " " << m[0][2] << " " << m[0][3] << " \\\n";
	out << "| " << m[1][0] << " " << m[1][1] << " " << m[1][2] << " " << m[1][3] << " |\n";
	out << "| " << m[2][0] << " " << m[2][1] << " " << m[2][2] << " " << m[2][3] << " |\n";
	out << "\\ " << m[3][0] << " " << m[3][1] << " " << m[3][2] << " " << m[3][3] << " /\n";
	return out;
}

__END_YAFRAY
