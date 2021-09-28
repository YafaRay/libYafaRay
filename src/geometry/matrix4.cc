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

Matrix4::Matrix4(const float init)
{
	for(int i = 0; i < 4; i++)
		for(int j = 0; j < 4; ++j)
		{
			if(i == j)
				matrix_[i][j] = init;
			else
				matrix_[i][j] = 0.f;
		}
}

Matrix4::Matrix4(const Matrix4 &source): invalid_(source.invalid_)
{
	for(int i = 0; i < 4; i++)
		for(int j = 0; j < 4; j++)
			matrix_[i][j] = source[i][j];
}

Matrix4::Matrix4(float m_00, float m_01, float m_02, float m_03, float m_10, float m_11, float m_12, float m_13, float m_20, float m_21, float m_22, float m_23, float m_30, float m_31, float m_32, float m_33) : matrix_{m_00, m_01, m_02, m_03, m_10, m_11, m_12, m_13, m_20, m_21, m_22, m_23, m_30, m_31, m_32, m_33}
{
}

Matrix4::Matrix4(const float source[4][4])
{
	for(int i = 0; i < 4; i++)
		for(int j = 0; j < 4; j++)
			matrix_[i][j] = source[i][j];
}

template<typename T>
void Matrix4::swapRows(T& matrix, int row_a, int row_b)
{
	for(int j = 0; j < 4; ++j)
	{
		std::swap(matrix[row_a][j], matrix[row_b][j]);
	}
}

template<typename T>
void Matrix4::subtractScaledRow(T& matrix, int row_a, int row_b, float factor)
{
	for(int j = 0; j < 4; ++j)
	{
		matrix[row_a][j] -= matrix[row_b][j] * factor;
	}
}

template<typename T>
void Matrix4::divideRow(T& matrix, int row, float divisor)
{
	for(int j = 0; j < 4; ++j)
	{
		matrix[row][j] /= divisor;
	}
}

Matrix4 &Matrix4::inverse()
{
	Matrix4 iden(1);
	for(int i = 0; i < 4; ++i)
	{
		float max = 0.f;
		int ci = 0;
		for(int k = i; k < 4; ++k)
		{
			if(std::abs(matrix_[k][i]) > max)
			{
				max = std::abs(matrix_[k][i]);
				ci = k;
			}
		}
		if(max == 0.f) invalid_ = true;
		swapRows(matrix_, i, ci);
		swapRows(iden, i, ci);
		float factor = matrix_[i][i];
		divideRow(matrix_, i, factor);
		divideRow(iden, i, factor);
		for(int k = 0; k < 4; ++k)
		{
			if(k != i)
			{
				factor = matrix_[k][i];
				subtractScaledRow(matrix_, k, i, factor);
				subtractScaledRow(iden, k, i, factor);
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
	temp = fmod(temp, 360.f);
	if(temp < 0.f) temp = 360.f - temp;
	temp *= math::div_pi_by_180;

	Matrix4 aux(1.f);
	aux[0][0] = math::cos(temp);
	aux[0][1] = -math::sin(temp);
	aux[1][0] = math::sin(temp);
	aux[1][1] = math::cos(temp);

	(*this) = aux * (*this);
}

void Matrix4::rotateX(float degrees)
{
	float temp = degrees;
	temp = fmod(temp, 360.f);
	if(temp < 0.f) temp = 360.f - temp;
	temp *= math::div_pi_by_180;

	Matrix4 aux(1.f);
	aux[1][1] = math::cos(temp);
	aux[1][2] = -math::sin(temp);
	aux[2][1] = math::sin(temp);
	aux[2][2] = math::cos(temp);

	(*this) = aux * (*this);
}

void Matrix4::rotateY(float degrees)
{
	float temp = degrees;
	temp = fmod(temp, 360.f);
	if(temp < 0.f) temp = 360.f - temp;
	temp *= math::div_pi_by_180;

	Matrix4 aux(1.f);
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
				matrix_[i][j] = 1.f;
			else
				matrix_[i][j] = 0.f;
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
