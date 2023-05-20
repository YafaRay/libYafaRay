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
#ifndef LIBYAFARAY_MATRIX_H
#define LIBYAFARAY_MATRIX_H

#include "public_api/yafaray_c_api.h"
#include "geometry/vector.h"
#include <iostream>

namespace yafaray {

template<typename T, size_t N>
class SquareMatrix
{
	static_assert(N >= 2, "This class can only be instantiated with dimensions N=2 (2x2) or bigger");
	static_assert(std::is_arithmetic_v<T>, "This class can only be instantiated for arithmetic types like int, float, etc");

	public:
		SquareMatrix() = default;
		SquareMatrix(const SquareMatrix &source) = default;
		SquareMatrix(SquareMatrix &&source) = default;
		explicit SquareMatrix(T init);
		explicit SquareMatrix(const std::array<std::array<T, N>, N> &source) : matrix_{source} { }
		explicit SquareMatrix(std::array<std::array<T, N>, N> &&source) : matrix_{std::move(source)} { }
		explicit SquareMatrix(const T *source);
		explicit SquareMatrix(const double *source);
		SquareMatrix<T, N> &operator=(const SquareMatrix<T, N> &matrix) = default;
		SquareMatrix<T, N> &operator=(SquareMatrix<T, N> &&matrix) = default;
		/*! attention, this function can cause the matrix to become invalid!
			unless you are sure the matrix is invertible, check invalid() afterwards! */
		SquareMatrix<T, N> &inverse();
		SquareMatrix<T, N> &transpose();
		void identity();
		void translate(const Vec<T, N - 1> &vec);
//		void rotateX(T degrees);
//		void rotateY(T degrees);
//		void rotateZ(T degrees);
		void scale(const Vec<T, N - 1> &vec);
		bool invalid() const { return invalid_; }
		const std::array<T, N> &operator [](size_t i) const { return matrix_[i]; }
		std::array<T, N> &operator [](size_t i) { return matrix_[i]; }
		void setVal(size_t row, size_t col, T val) { matrix_[row][col] = val; };
		T getVal(size_t row, size_t col) { return matrix_[row][col]; }
		[[nodiscard]] std::string exportToString(yafaray_ContainerExportType container_export_type) const;

	private:
		template<typename MatrixType> static void swapRows(MatrixType& matrix, size_t row_a, size_t row_b);
		template<typename MatrixType> static void subtractScaledRow(MatrixType& matrix, size_t row_a, size_t row_b, T factor);
		template<typename MatrixType> static void divideRow(MatrixType& matrix, size_t row, T divisor);

		alignas(std::max(size_t{8}, sizeof(T))) std::array<std::array<T, N>, N> matrix_;
		bool invalid_ = false;
};

using Matrix4f = SquareMatrix<float, 4>;

template<typename T, size_t N>
inline SquareMatrix<T, N> operator * (const SquareMatrix<T, N> &a, const SquareMatrix<T, N> &b)
{
	SquareMatrix<T, N> aux;
	for(size_t i = 0; i < N; ++i)
	{
		for(size_t k = 0; k < N; ++k)
		{
			aux[i][k] = 0;
			for(size_t j = 0; j < N; ++j)
			{
				aux[i][k] += a[i][j] * b[j][k];
			}
		}
	}
	return aux;
}

template<typename T, size_t N>
inline SquareMatrix<T, N> operator + (const SquareMatrix<T, N> &a, const SquareMatrix<T, N> &b)
{
	SquareMatrix<T, N> aux;
	for(size_t i = 0; i < N; ++i)
		for(size_t j = 0; j < N; ++j)
			aux[i][j] = a[i][j] + b[i][j];
	return aux;
}

template<typename T, size_t N>
inline SquareMatrix<T, N> operator * (const SquareMatrix<T, N> &m, T f)
{
	SquareMatrix<T, N> aux;
	for(size_t i = 0; i < N; ++i)
		for(size_t j = 0; j < N; ++j)
			aux[i][j] = f * m[i][j];
	return aux;
}

template<typename T, size_t N>
inline SquareMatrix<T, N> operator * (T f, const SquareMatrix<T, N> &m)
{
	return m * f;
}

template<typename T, size_t N>
inline Vec<T, N - 1> operator * (const SquareMatrix<T, N> &m, const Vec<T, N - 1> &v)
{
	Vec<T, N - 1> aux{T{0}};
	for(size_t i = 0; i < N - 1; ++i)
		for(size_t j = 0; j < N - 1; ++j)
			aux[i] += m[i][j] * v[j];
	return aux;
}

template<typename T, size_t N>
inline Point<T, N - 1> operator * (const SquareMatrix<T, N> &m, const Point<T, N - 1> &v)
{
	Point<T, N - 1> aux{T{0}};
	for(size_t i = 0; i < N - 1; ++i)
	{
		for(size_t j = 0; j < N - 1; ++j)
		{
			aux[i] += m[i][j] * v[j];
		}
		aux[i] += m[i][N - 1];
	}
	return aux;
}

template<typename T, size_t N>
inline SquareMatrix<T, N>::SquareMatrix(const T init)
{
	for(size_t i = 0; i < N; ++i)
	{
		for(size_t j = 0; j < N; ++j)
		{
			if(i == j) matrix_[i][j] = init;
			else matrix_[i][j] = T{0};
		}
	}
}

template<typename T, size_t N>
inline SquareMatrix<T, N>::SquareMatrix(const T *source)
{
	for(size_t i = 0; i < N; ++i)
		for(size_t j = 0; j < N; ++j)
			matrix_[i][j] = source[N * i + j];
}

template<typename T, size_t N>
inline SquareMatrix<T, N>::SquareMatrix(const double *source)
{
	for(size_t i = 0; i < N; ++i)
		for(size_t j = 0; j < N; ++j)
			matrix_[i][j] = source[N * i + j];
}

template<typename T, size_t N>
template<typename MatrixType>
inline void SquareMatrix<T, N>::swapRows(MatrixType& matrix, size_t row_a, size_t row_b)
{
	for(size_t j = 0; j < N; ++j)
	{
		std::swap(matrix[row_a][j], matrix[row_b][j]);
	}
}

template<typename T, size_t N>
template<typename MatrixType>
inline void SquareMatrix<T, N>::subtractScaledRow(MatrixType& matrix, size_t row_a, size_t row_b, T factor)
{
	for(size_t j = 0; j < N; ++j)
	{
		matrix[row_a][j] -= matrix[row_b][j] * factor;
	}
}

template<typename T, size_t N>
template<typename MatrixType>
inline void SquareMatrix<T, N>::divideRow(MatrixType& matrix, size_t row, T divisor)
{
	for(size_t j = 0; j < N; ++j)
	{
		matrix[row][j] /= divisor;
	}
}

template<typename T, size_t N>
inline SquareMatrix<T, N> &SquareMatrix<T, N>::inverse()
{
	SquareMatrix<T, N> iden(1);
	for(size_t i = 0; i < N; ++i)
	{
		T max{T{0}};
		size_t ci = 0;
		for(size_t k = i; k < N; ++k)
		{
			if(std::abs(matrix_[k][i]) > max)
			{
				max = std::abs(matrix_[k][i]);
				ci = k;
			}
		}
		if(max == T{0}) invalid_ = true;
		swapRows(matrix_, i, ci);
		swapRows(iden, i, ci);
		T factor = matrix_[i][i];
		divideRow(matrix_, i, factor);
		divideRow(iden, i, factor);
		for(size_t k = 0; k < N; ++k)
		{
			if(k != i)
			{
				factor = matrix_[k][i];
				subtractScaledRow(matrix_, k, i, factor);
				subtractScaledRow(iden, k, i, factor);
			}
		}
	}
	for(size_t i = 0; i < N; ++i)
		for(size_t j = 0; j < N; ++j)
			matrix_[i][j] = iden[i][j];
	return *this;
}

template<typename T, size_t N>
inline SquareMatrix<T, N> &SquareMatrix<T, N>::transpose()
{
	for(size_t i = 0; i < (N - 1); ++i)
		for(size_t j = i + 1; j < N; ++j)
			std::swap(matrix_[i][j], matrix_[j][i]);
	return *this;
}

template<typename T, size_t N>
inline void SquareMatrix<T, N>::translate(const Vec<T, N - 1> &vec)
{
	SquareMatrix<T, N> aux{T{1}};
	for(size_t i = 0; i < N - 1; ++i)
	{
		aux[i][N - 1] = vec[i];
	}
	(*this) = aux * (*this);
}
/*
template<typename T, size_t N>
inline void SquareMatrix<T, N>::rotateZ(T degrees)
{
	//FIXME: generalize properly
	T temp{std::fmod(degrees, T{360})};
	if(temp < T{0}) temp = T{360} - temp;
	temp *= math::div_pi_by_180<T>;

	SquareMatrix<T, N> aux{T{1}};
	aux[0][0] = math::cos(temp);
	aux[0][1] = -math::sin(temp);
	aux[1][0] = math::sin(temp);
	aux[1][1] = math::cos(temp);

	(*this) = aux * (*this);
}

template<typename T, size_t N>
inline void SquareMatrix<T, N>::rotateX(T degrees)
{
	//FIXME: generalize properly
	T temp{std::fmod(degrees, T{360})};
	if(temp < T{0}) temp = T{360} - temp;
	temp *= math::div_pi_by_180<T>;

	SquareMatrix<T, N> aux{T{1}};
	aux[1][1] = math::cos(temp);
	aux[1][2] = -math::sin(temp);
	aux[2][1] = math::sin(temp);
	aux[2][2] = math::cos(temp);

	(*this) = aux * (*this);
}

template<typename T, size_t N>
inline void SquareMatrix<T, N>::rotateY(T degrees)
{
	//FIXME: generalize properly
	T temp{std::fmod(degrees, T{360})};
	if(temp < T{0}) temp = T{360} - temp;
	temp *= math::div_pi_by_180<T>;

	SquareMatrix<T, N> aux{T{1}};
	aux[0][0] = math::cos(temp);
	aux[0][2] = math::sin(temp);
	aux[2][0] = -math::sin(temp);
	aux[2][2] = math::cos(temp);

	(*this) = aux * (*this);
}*/

template<typename T, size_t N>
inline void SquareMatrix<T, N>::scale(const Vec<T, N - 1> &vec)
{
	for(size_t i = 0; i < N - 1; ++i)
		for(size_t j = 0; j < N - 1; ++j)
			matrix_[i][j] *= vec[j];
}

template <typename T, size_t N>
inline constexpr bool operator == (const SquareMatrix<T, N> &matrix_a, const SquareMatrix<T, N> &matrix_b)
{
	for(size_t i = 0; i < N - 1; ++i)
		for(size_t j = 0; j < N - 1; ++j)
			if(matrix_a[i][j] != matrix_b[i][j]) return false;
	return true;
}

template<typename T, size_t N>
inline void SquareMatrix<T, N>::identity()
{
	for(size_t i = 0; i < N; ++i)
	{
		for(size_t j = 0; j < N; ++j)
		{
			if(i == j) matrix_[i][j] = T{1};
			else matrix_[i][j] = T{0};
		}
	}
}

template<typename T, size_t N>
inline std::ostream &operator << (std::ostream &out, const SquareMatrix<T, N> &m)
{
	for(size_t i = 0; i < N; ++i)
	{
		if(i == 0) out << "/ ";
		else if(i == N - 1) out << "\\ ";
		else out << "| ";
		for(size_t j = 0; j < N; ++j)
		{
			out << m[i][j];
			if(j < N - 1) out << " ";
		}
		if(i == 0) out << " \\\n";
		else if(i == N - 1) out << " /\n";
		else out << " |\n";
	}
	return out;
}

template<typename T, size_t N>
inline std::string SquareMatrix<T, N>::exportToString(yafaray_ContainerExportType container_export_type) const
{
	std::stringstream ss;
	for(size_t i = 0; i < N; ++i)
	{
		for(size_t j = 0; j < N; ++j)
		{
			if(i > 0 || j > 0)
			{
				if(container_export_type == YAFARAY_CONTAINER_EXPORT_XML) ss << " ";
				else ss << ", ";
			}
			ss << "m" << static_cast<char>('0' + i) << static_cast<char>('0' + j) << "=\"" << matrix_[i][j] << "\"";
		}
	}
	return ss.str();
}

} //namespace yafaray

#endif
