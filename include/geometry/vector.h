#pragma once
/****************************************************************************
 *
 *      vector3d.h: Vector 3d and point representation and manipulation api
 *      This is part of the libYafaRay package
 *      Copyright (C) 2002 Alejandro Conty Estévez
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
#ifndef YAFARAY_VECTOR_H
#define YAFARAY_VECTOR_H

#include "math/math.h"
#include "math/random.h"
#include "geometry/axis.h"
#include "uv.h"
#include <iomanip>
#include <array>

namespace yafaray {

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif

template <typename T, size_t N>
class Vec
{
	static_assert(N >= 2, "This class can only be instantiated with 2 or more components");
	static_assert(std::is_arithmetic_v<T>, "This class can only be instantiated for arithmetic types like int, float, etc");

	public:
		Vec<T, N>() = default;
		constexpr Vec<T, N>(const Vec<T, N> &vec) = default;
		constexpr Vec<T, N>(Vec<T, N> &&vec) = default;
		constexpr Vec<T, N> &operator = (const Vec<T, N> &vec) = default;
		constexpr Vec<T, N> &operator = (Vec<T, N> &&vec) = default;
		constexpr explicit Vec<T, N>(T val): array_{makeArray(val)} { }
		constexpr Vec<T, N>(const std::array<T, N> &array): array_{array} { }
		constexpr Vec<T, N>(std::array<T, N> &&array): array_{std::move(array)} { }
		Vec<T, N> &normalize();
		Vec<T, N> normalized() const;
		[[nodiscard]] std::pair<size_t, T> findLargestComponent() const;
		constexpr void reflect(const Vec<T, N> &normal);
		[[nodiscard]] T normalizeAndReturnLength();
		[[nodiscard]] T normalizeAndReturnLengthSquared();
		[[nodiscard]] constexpr T lengthSquared() const { return (*this) * (*this); }
		[[nodiscard]] T length() const { return math::sqrt(lengthSquared()); }
		[[nodiscard]] T sinFromVectors(const Vec<T, N> &vec) const;
		constexpr Vec<T, N> &operator +=(const Vec<T, N> &vec) { std::transform(array_.begin(), array_.end(), vec.begin(), array_.begin(), [](T array_component, T vec_component){ return array_component + vec_component;}); return *this; }
		constexpr Vec<T, N> &operator -=(const Vec<T, N> &vec) { return std::transform(array_.begin(), array_.end(), vec.begin(), array_.begin(), [](T array_component, T vec_component){ return array_component - vec_component;}); return *this; }
		constexpr Vec<T, N> &operator /=(T val) { std::for_each(array_.begin(), array_.end(), [val](T &component) { component /= val; }); return *this; }
		constexpr Vec<T, N> &operator *=(T val) { std::for_each(array_.begin(), array_.end(), [val](T &component) { component *= val; }); return *this; }
		[[nodiscard]] constexpr T operator[](Axis axis) const { return array_[axis::getId(axis)]; }
		[[nodiscard]] constexpr T &operator[](Axis axis) { return array_[axis::getId(axis)]; }
		[[nodiscard]] constexpr T operator[](size_t component) const { return array_[component]; }
		[[nodiscard]] constexpr T &operator[](size_t component) { return array_[component]; }
		[[nodiscard]] constexpr std::array<T, N> getArray() const { return array_; }
		[[nodiscard]] static constexpr inline Vec<T, N> reflectDir(const Vec<T, N> &normal, const Vec<T, N> &vec);
		[[nodiscard]] static bool refract(Vec<T, N> n, const Vec<T, N> &wi, Vec<T, N> &wo, T ior);
		[[nodiscard]] static constexpr inline std::array<T, 2> fresnel(const Vec<T, N> &i, const Vec<T, N> &n, T ior);
		[[nodiscard]] static constexpr inline std::array<T, 2> fastFresnel(const Vec<T, N> &i, const Vec<T, N> &n, T iorf);
		[[nodiscard]] static constexpr inline Uv<Vec<T, 3>> createCoordsSystem(const Vec<T, 3> &normal);
		[[nodiscard]] static Uv<T> shirleyDisk(T r_1, T r_2);
		[[nodiscard]] static Vec<T, 3> randomSpherical(FastRandom &fast_random);
		[[nodiscard]] static Vec<T, N> randomVectorCone(const Vec<T, N> &d, const Uv<Vec<T, N>> &uv, T cos_angle, T z_1, T z_2);
		[[nodiscard]] static Vec<T, N> randomVectorCone(const Vec<T, N> &dir, T cos_angle, T r_1, T r_2);
		[[nodiscard]] static Vec<T, 3> discreteVectorCone(const Vec<T, 3> &dir, T cos_angle, int sample, int square);
		[[nodiscard]] static constexpr inline std::array<T, N> makeArray(T val);
		[[nodiscard]] std::string print() const;
		typename std::array<T, N>::iterator begin() { return array_.begin(); }
		typename std::array<T, N>::iterator end() { return array_.end(); }
		typename std::array<T, N>::const_iterator begin() const { return array_.begin(); }
		typename std::array<T, N>::const_iterator end() const { return array_.end(); }

	private:
		std::array<T, N> array_;
};

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

using Vec2i = Vec<int, 2>;
using Size2i = Vec2i;
using Vec3f = Vec<float, 3>;
using Vec3d = Vec<double, 3>;

template <typename T, size_t N>
class Point final : public Vec<T, N>
{
	public:
		using Vec<T, N>::Vec;
		Point(const Vec<T, N> &vec) : Vec<T, N>{vec} {}
		Point(Vec<T, N> &&vec) : Vec<T, N>{std::move(vec)} {}
};

using Point2i = Point<int, 2>;
using Point3i = Point<int, 3>;
using Point3f = Point<float, 3>;

template <typename T, size_t N>
constexpr inline std::array<T, N> Vec<T, N>::makeArray(T val)
{
	if constexpr (N == 2)
	{
		return {{val, val}};
	}
	else if constexpr (N == 3)
	{
		return {{val, val, val}};
	}
	else
	{
		std::array<T, N> result{val, val, val, val};
		std::fill(result.begin() + 4, result.end(), val);
		return result;
	}
}

template <typename T, size_t N>
inline constexpr bool operator == (const Vec<T, N> &vec_a, const Vec<T, N> &vec_b)
{
	if constexpr (N == 2)
	{
		return vec_a[0] == vec_b[0] && vec_a[1] == vec_b[1];
	}
	else if constexpr (N == 3)
	{
		return vec_a[0] == vec_b[0] && vec_a[1] == vec_b[1] && vec_a[2] == vec_b[2];
	}
	else
	{
		bool result{vec_a[0] == vec_b[0] && vec_a[1] == vec_b[1] && vec_a[2] == vec_b[2] && vec_a[3] == vec_b[3]};
		for(size_t i = 4; i < N; ++i) result = result && (vec_a[i] == vec_b[i]);
		return result;
	}
}

template <typename T, size_t N>
inline constexpr T operator * (const Vec<T, N> &vec_a, const Vec<T, N> &vec_b)
{
	if constexpr (N == 2)
	{
		return vec_a[0] * vec_b[0] + vec_a[1] * vec_b[1];
	}
	else if constexpr (N == 3)
	{
		return vec_a[0] * vec_b[0] + vec_a[1] * vec_b[1] + vec_a[2] * vec_b[2];
	}
	else
	{
		T result{vec_a[0] * vec_b[0] + vec_a[1] * vec_b[1] + vec_a[2] * vec_b[2] + vec_a[3] * vec_b[3]};
		for(size_t i = 4; i < N; ++i) result += vec_a[i] * vec_b[i];
		return result;
		//return std::inner_product(vec_a.begin(), vec_a.end(), vec_b.begin(), 0.f); //Slower
	}
}

template <typename T, size_t N>
inline constexpr Vec<T, N> operator * (T val, const Vec<T, N> &vec)
{
	if constexpr (N == 2)
	{
		return {{val * vec[0], val * vec[1]}};
	}
	else if constexpr (N == 3)
	{
		return {{val * vec[0], val * vec[1], val * vec[2]}};
	}
	else
	{
		std::array<T, N> result{val * vec[0], val * vec[1], val * vec[2], val * vec[3]};
		std::transform(vec.begin() + 4, vec.end(), result.begin() + 4, [val](T vec_component){ return val * vec_component; });
		return result;
	}
}

template <typename T, size_t N>
inline constexpr Vec<T, N> operator * (const Vec<T, N> &vec, T val)
{
	return val * vec;
}

template <typename T, size_t N>
inline constexpr Vec<T, N> multiplyComponents(const Vec<T, N> &vec_a, const Vec<T, N> &vec_b)
{
	if constexpr (N == 2)
	{
		return {{vec_a[0] * vec_b[0], vec_a[1] * vec_b[1]}};
	}
	else if constexpr (N == 3)
	{
		return {{vec_a[0] * vec_b[0], vec_a[1] * vec_b[1], vec_a[2] * vec_b[2]}};
	}
	else
	{
		std::array<T, N> result{vec_a[0] * vec_b[0], vec_a[1] * vec_b[1], vec_a[2] * vec_b[2], vec_a[3] * vec_b[3]};
		std::transform(vec_a.begin() + 4, vec_a.end(), vec_b.begin() + 4, result.begin() + 4, std::multiplies{});
		return result;
	}
}

template <typename T, size_t N>
inline constexpr Vec<T, N> operator / (const Vec<T, N> &vec, T val)
{
	if constexpr (N == 2)
	{
		return {{vec[0] / val, vec[1] / val}};
	}
	else if constexpr (N == 3)
	{
		return {{vec[0] / val, vec[1] / val, vec[2] / val}};
	}
	else
	{
		std::array<T, N> result{vec[0] / val, vec[1] / val, vec[2] / val, vec[3] / val};
		std::transform(vec.begin() + 4, vec.end(), result.begin() + 4, [val](T vec_component){ return vec_component / val; });
		return result;
	}
}

template <typename T>
inline constexpr Vec<T, 3> operator ^ (const Vec<T, 3> &vec_a, const Vec<T, 3> &vec_b)
{
	return {{
		vec_a[1] * vec_b[2] - vec_a[2] * vec_b[1],
		vec_a[2] * vec_b[0] - vec_a[0] * vec_b[2],
		vec_a[0] * vec_b[1] - vec_a[1] * vec_b[0]
	}};
}

template <typename T, size_t N>
inline constexpr Vec<T, N> operator - (const Vec<T, N> &vec_a, const Vec<T, N> &vec_b)
{
	if constexpr (N == 2)
	{
		return {{vec_a[0] - vec_b[0], vec_a[1] - vec_b[1]}};
	}
	else if constexpr (N == 3)
	{
		return {{vec_a[0] - vec_b[0], vec_a[1] - vec_b[1], vec_a[2] - vec_b[2]}};
	}
	else
	{
		std::array<T, N> result{vec_a[0] - vec_b[0], vec_a[1] - vec_b[1], vec_a[2] - vec_b[2], vec_a[3] - vec_b[3]};
		std::transform(vec_a.begin() + 4, vec_a.end(), vec_b.begin() + 4, result.begin() + 4, std::minus{});
		return result;
	}
}

template <typename T, size_t N>
inline constexpr Vec<T, N> operator - (const Vec<T, N> &vec)
{
	if constexpr (N == 2)
	{
		return {{-vec[0], -vec[1]}};
	}
	else if constexpr (N == 3)
	{
		return {{-vec[0], -vec[1], -vec[2]}};
	}
	else
	{
		std::array<T, N> result{-vec[0], -vec[1], -vec[2], -vec[3]};
		std::transform(vec.begin() + 4, vec.end(), result.begin() + 4, [](T vec_component){ return -vec_component; });
		return result;
	}
}

template <typename T, size_t N>
inline constexpr Vec<T, N> operator + (const Vec<T, N> &vec_a, const Vec<T, N> &vec_b)
{
	if constexpr (N == 2)
	{
		return {{vec_a[0] + vec_b[0], vec_a[1] + vec_b[1]}};
	}
	else if constexpr (N == 3)
	{
		return {{vec_a[0] + vec_b[0], vec_a[1] + vec_b[1], vec_a[2] + vec_b[2]}};
	}
	else
	{
		std::array<T, N> result{vec_a[0] + vec_b[0], vec_a[1] + vec_b[1], vec_a[2] + vec_b[2], vec_a[3] + vec_b[3]};
		std::transform(vec_a.begin() + 4, vec_a.end(), vec_b.begin() + 4, result.begin() + 4, std::plus{});
		return result;
	}
}

template <typename T, size_t N>
inline Vec<T, N> &Vec<T, N>::normalize()
{
	const T length_squared{lengthSquared()};
	if(length_squared != T{0})
	{
		const T inverse_length{T{1} / math::sqrt(length_squared)};
		for(auto &component : array_) component *= inverse_length;
	}
	return *this;
}

template <typename T, size_t N>
inline Vec<T, N> Vec<T, N>::normalized() const
{
	Vec<T, N> result{*this};
	result.normalize();
	return result;
}

template<typename T, size_t N>
std::pair<size_t, T> Vec<T, N>::findLargestComponent() const
{
	size_t index_max_value{0};
	T max_value{array_.front()};
	for(size_t index = 1; index < N; ++index)
	{
		if(max_value < array_[index])
		{
			max_value = array_[index];
			index_max_value = index;
		}
	}
	return {index_max_value, max_value};
}

template <typename T, size_t N>
inline T Vec<T, N>::sinFromVectors(const Vec<T, N> &vec) const
{
	const T div{(length() * vec.length()) * T{0.99999} + T{0.00001}};
	T asin_argument{((*this ^ vec).length() / div) * T{0.99999}};
	//Fix to avoid black "nan" areas when this argument goes slightly over +1.0. Why that happens in the first place, maybe floating point rounding errors?
	if(asin_argument > T{1}) asin_argument = T{1};
	return math::asin(asin_argument);
}

/** Vector reflection.
 *  Reflects the vector onto a surface whose normal is \a n
 *  @param	normal Surface normal
 *  @warning	\a n must be unit vector!
 *  @note	Lynn's formula: R = 2*(V dot N)*N -V (http://www.3dkingdoms.com/weekly/weekly.php?a=2)
 */
template <typename T, size_t N>
inline constexpr void Vec<T, N>::reflect(const Vec<T, N> &normal)
{
	const T vn{T{2} * (*this) * normal};
	std::transform(array_.begin(), array_.end(), normal.begin(), array_.begin(), [vn](T array_component, T normal_component){ return vn * normal_component - array_component; });
}

template <typename T, size_t N>
inline T Vec<T, N>::normalizeAndReturnLength()
{
	const T length_squared{lengthSquared()};
	if(length_squared != T{0})
	{
		const T length{math::sqrt(length_squared)};
		const T inverse_length{T{1} / length};
		for(T &component : array_) component *= inverse_length;
		return length;
	}
	return T{0};
}

template <typename T, size_t N>
inline T Vec<T, N>::normalizeAndReturnLengthSquared()
{
	const T length_squared{lengthSquared()};
	if(length_squared != T{0})
	{
		const T inverse_length{T{1} / math::sqrt(length_squared)};
		for(T &component : array_) component *= inverse_length;
	}
	return length_squared;
}

template <typename T, size_t N>
inline constexpr Vec<T, N> Vec<T, N>::reflectDir(const Vec<T, N> &normal, const Vec<T, N> &vec)
{
	const T vn{vec * normal};
	if(vn < T{0}) return -vec;
	return T{2} * vn * normal - vec;
}

template <typename T, size_t N>
inline constexpr Uv<Vec<T, 3>> Vec<T, N>::createCoordsSystem(const Vec<T, 3> &normal)
{
	if((normal.array_[0] == T{0}) && (normal.array_[1] == T{0}))
	{
		return {
			(
				normal.array_[2] < T{0} ?
				Vec<T, 3>{{T{-1}, T{0}, T{0}}}
				:
				Vec<T, 3>{{T{1}, T{0}, T{0}}}
			),
			Vec<T, 3>{{T{0}, T{1}, T{0}}}
		};
	}
	else
	{
		// Note: The root cannot become zero if
		// N.x==0 && N.y==0.
		const T d{T{1} / math::sqrt(normal.array_[1] * normal.array_[1] + normal.array_[0] * normal.array_[0])};
		const Vec<T, 3> u{{normal.array_[1] * d, -normal.array_[0] * d, T{0}}};
		return {u, normal ^ u};
	}
}

template <typename T, size_t N>
inline Vec<T, 3> Vec<T, N>::randomSpherical(FastRandom &fast_random)
{
	Vec<T, 3> v{{T{0}, T{0}, fast_random.getNextFloatNormalized()}};
	T r{T{1} - v.array_[2] * v.array_[2]};
	if(r > T{0})
	{
		const T a = math::mult_pi_by_2<T> * fast_random.getNextFloatNormalized();
		r = math::sqrt(r);
		v.array_[0] = r * math::cos(a); v.array_[1] = r * math::sin(a);
	}
	else v.array_[2] = T{1};
	return v;
}

// P.Shirley's concentric disk algorithm, maps square to disk
template <typename T, size_t N>
inline Uv<T> Vec<T, N>::shirleyDisk(T r_1, T r_2)
{
	T phi;
	T r;
	const T a{T{2} * r_1 - T{1}};
	const T b{T{2} * r_2 - T{1}};
	if(a > -b)
	{
		if(a > b)  	// Reg.1
		{
			r = a;
			phi = math::div_pi_by_4<T> * (b / a);
		}
		else  			// Reg.2
		{
			r = b;
			phi = math::div_pi_by_4<T> * (T{2} - a / b);
		}
	}
	else
	{
		if(a < b)  	// Reg.3
		{
			r = -a;
			phi = math::div_pi_by_4<T> * (T{4} + b / a);
		}
		else  			// Reg.4
		{
			r = -b;
			if(b != T{0})
				phi = math::div_pi_by_4<T> * (T{6} - a / b);
			else
				phi = T{0};
		}
	}
	return {r * math::cos(phi), r * math::sin(phi)};
}

/*! refract a ray given the IOR. All directions (n, wi and wo) point away from the intersection point.
	\return true when refraction was possible, false when total inner reflrection occurs (wo is not computed then)
	\param ior Index of refraction, or precisely the ratio of eta_t/eta_i, where eta_i is by definition the
				medium in which n points. E.g. "outside" is air, "inside" is water, the normal points outside,
				IOR = eta_air / eta_water = 1.33
*/
template <typename T, size_t N>
inline bool Vec<T, N>::refract(Vec<T, N> n, const Vec<T, N> &wi, Vec<T, N> &wo, T ior)
{
	T eta;
	T cos_v_n{wi * n};
	if(cos_v_n < T{0})
	{
		eta = ior;
		n = -n;
		cos_v_n = -cos_v_n;
	}
	else
	{
		eta = T{1} / ior;
	}
	const T k{T{1} - eta * eta * (T{1} - cos_v_n * cos_v_n)};
	if(k <= T{0}) return false;

	wo = eta * -wi + (eta * cos_v_n - math::sqrt(k)) * n;
	wo.normalize();

	return true;
}

template <typename T, size_t N>
inline constexpr std::array<T, 2> Vec<T, N>::fresnel(const Vec<T, N> &i, const Vec<T, N> &n, T ior)
{
	const bool negative = ((i * n) < T{0});
	const T c = i * (negative ? (-n) : n);
	T g = ior * ior + c * c - T{1};
	if(g <= T{0}) g = T{0};
	else g = math::sqrt(g);
	const T aux = c * (g + c);
	const T kr = ((T{0.5} * (g - c) * (g - c)) / ((g + c) * (g + c))) * (T{1} + ((aux - T{1}) * (aux - T{1})) / ((aux + T{1}) * (aux + T{1})));
	const T kt = (kr < T{1}) ? T{1} - kr : T{0};
	return {kr, kt};
}

// 'Faster' Schlick fresnel approximation,
template <typename T, size_t N>
inline constexpr std::array<T, 2> Vec<T, N>::fastFresnel(const Vec<T, N> &i, const Vec<T, N> &n, T iorf)
{
	const T t{T{1} - (i * n)};
	//t = (t<0)?0:((t>1)?1:t);
	const T t_2{t * t};
	const T kr{iorf + (T{1} - iorf) * t_2 * t_2 * t};
	const T kt {T{1} - kr};
	return {kr, kt};
}

template <typename T, size_t N>
inline Vec<T, N> Vec<T, N>::randomVectorCone(const Vec<T, N> &d, const Uv<Vec<T, N>> &uv, T cos_angle, T z_1, T z_2)
{
	const T t_1{math::mult_pi_by_2<T> * z_1};
	const T t_2{T{1} - (T{1} - cos_angle) * z_2};
	return (uv.u_ * math::cos(t_1) + uv.v_ * math::sin(t_1)) * math::sqrt(T{1} - t_2 * t_2) + d * t_2;
}

template <typename T, size_t N>
inline Vec<T, N> Vec<T, N>::randomVectorCone(const Vec<T, N> &dir, T cos_angle, T r_1, T r_2)
{
	const auto uv{createCoordsSystem(dir)};
	return Vec<T, N>::randomVectorCone(dir, uv, cos_angle, r_1, r_2);
}

template <typename T, size_t N>
inline std::ostream &operator << (std::ostream &out, const Vec<T, N> &v)
{
	out << std::setprecision(std::numeric_limits<T>::digits10 + 1) << "(";
	for(size_t i = 0; i < N; ++i)
	{
		out << v[i];
		if((i + 1) < N) out << ", ";
	}
	out << ")";
	return out;
}

template <typename T, size_t N>
inline std::string Vec<T, N>::print() const
{
	std::stringstream ss;
	ss << *this;
	return ss.str();
}

} //namespace yafaray

#endif // YAFARAY_VECTOR_H
