#pragma once
/****************************************************************************
 *
 *      bound.h: Bound<T> and tree api for general raytracing acceleration
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

#ifndef YAFARAY_BOUND_H
#define YAFARAY_BOUND_H

#include "ray.h"
#include "geometry/matrix.h"
#include <limits>

namespace yafaray {

/** Bounding box
 *
 * The bounding box class. A box aligned with the axis used to skip
 * object, photons, and faces intersection when possible.
 *
 */

template<typename T>
class Bound
{
	static_assert(std::is_arithmetic_v<T>, "This class can only be instantiated for arithmetic types like int, float, etc");

	public:
		struct Cross
		{
			bool crossed_ = false;
			T enter_;
			T leave_;
		};
		/*! Main constructor.
		 * The box is defined by two points, this constructor just takes them.
		 *
		 * @param a is the low corner (minx,miny,minz)
		 * @param g is the up corner (maxx,maxy,maxz)
		 */
		Bound<T>(const Point<T, 3> &a, const Point<T, 3> &g) : a_{a}, g_{g} { }
		Bound<T>(Point<T, 3> &&a, Point<T, 3> &&g) : a_{std::move(a)}, g_{std::move(g)} { }
		//! Default constructors and assignments
		Bound<T>() = default;
		Bound<T>(const Bound<T> &bound) = default;
		Bound<T>(Bound<T> &&bound) = default;
		Bound<T>& operator=(const Bound<T> &bound) = default;
		Bound<T>& operator=(Bound<T> &&bound) = default;
		/*! Two child constructor.
		 * This creates a bound that includes the two given bounds. It's used when
		 * building a bounding tree
		 *
		 * @param r is one child bound
		 * @param l is another child bound
		 */
		Bound<T>(const Bound<T> &r, const Bound<T> &l);
		//! Returns true if the given ray crosses the bound
		//bool cross(const point3d_t &from,const vector3d_t &ray) const;
		//! Returns true if the given ray crosses the bound closer than dist
		//bool cross(const point3d_t &from, const vector3d_t &ray, T dist) const;
		//bool cross(const point3d_t &from, const vector3d_t &ray, T &where, T dist) const;
		Cross cross(const Ray &ray, T t_max) const;
		//! Returns the volume of the bound
		T vol() const;
		//! Returns the length along a certain axis
		T length(Axis axis) const { return g_[axis] - a_[axis]; }
		//! Returns the longest length among all axes
		T longestLength() const { return math::max(length(Axis::X), length(Axis::Y), length(Axis::Z)); }
		//! Cuts the bound in a certain axis to have the given max value
		void setAxisMax(Axis axis, T val) { g_[axis] = val;};
		//! Cuts the bound in a certain axis to have the given min value
		void setAxisMin(Axis axis, T val) { a_[axis] = val;};
		//! Adjust bound size to include point p
		void include(const Point<T, 3> &p);
		void include(const Bound<T> &b);
		//! Returns true if the point is inside the bound
		bool includes(const Point<T, 3> &pn) const
		{
			return ((pn[Axis::X] >= a_[Axis::X]) && (pn[Axis::X] <= g_[Axis::X]) &&
					(pn[Axis::Y] >= a_[Axis::Y]) && (pn[Axis::Y] <= g_[Axis::Y]) &&
					(pn[Axis::Z] >= a_[Axis::Z]) && (pn[Axis::Z] <= g_[Axis::Z]));
		};
		Axis largestAxis() const
		{
			const Vec<T, 3> d{g_ - a_};
			return (d[Axis::X] > d[Axis::Y]) ? ((d[Axis::X] > d[Axis::Z]) ? Axis::X : Axis::Z) : ((d[Axis::Y] > d[Axis::Z]) ? Axis::Y : Axis::Z);
		}

		//	protected: // Lynx; need these to be public.
		//! Two points define the box
		Point<T, 3> a_, g_;
};

template<typename T>
inline void Bound<T>::include(const Point<T, 3> &p)
{
	a_ = {{
			std::min(a_[Axis::X], p[Axis::X]),
			std::min(a_[Axis::Y], p[Axis::Y]),
			std::min(a_[Axis::Z], p[Axis::Z])
	}};
	g_ = {{
			std::max(g_[Axis::X], p[Axis::X]),
			std::max(g_[Axis::Y], p[Axis::Y]),
			std::max(g_[Axis::Z], p[Axis::Z])
	}};
}

template<typename T>
inline void Bound<T>::include(const Bound<T> &b)
{
	include(b.a_);
	include(b.g_);
}

template<typename T>
inline typename Bound<T>::Cross Bound<T>::cross(const Ray &ray, T t_max) const
{
	// Smits method
	const Point<T, 3> p{ray.from_ - a_};
	//infinity check initial values
	T lmin{T{std::numeric_limits<T>::lowest()}};
	T lmax{T{std::numeric_limits<T>::max()}};
	for(const Axis axis : axis::spatial)
	{
		if(ray.dir_[axis] != T{0})
		{
			const T inv_dir{T{1} / ray.dir_[axis]};
			T ltmin, ltmax;
			if(inv_dir > T{0})
			{
				ltmin = -p[axis] * inv_dir;
				ltmax = ((g_[axis] - a_[axis]) - p[axis]) * inv_dir;
			}
			else
			{
				ltmin = ((g_[axis] - a_[axis]) - p[axis]) * inv_dir;
				ltmax = -p[axis] * inv_dir;
			}
			if(axis == Axis::X)
			{
				lmin = ltmin;
				lmax = ltmax;
			}
			else
			{
				lmin = std::max(ltmin, lmin);
				lmax = std::min(ltmax, lmax);
			}
			if((lmax < 0) || (lmin > t_max)) return {};
		}
	}
	if((lmin <= lmax) && (lmax >= T{0}) && (lmin <= t_max))
	{
		return {true, lmin, lmax};
	}
	else return {};
}

template<typename T>
inline Bound<T> operator * (const Bound<T> &b, const SquareMatrix<T, 4> &m)
{
	return { m * b.a_, m * b.g_ };
}

} //namespace yafaray

#endif // YAFARAY_BOUND_H
