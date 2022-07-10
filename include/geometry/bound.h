#pragma once
/****************************************************************************
 *
 *      bound.h: Bound and tree api for general raytracing acceleration
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

namespace yafaray {

/** Bounding box
 *
 * The bounding box class. A box aligned with the axis used to skip
 * object, photons, and faces intersection when possible.
 *
 */

class Bound
{
	public:
		struct Cross
		{
			bool crossed_ = false;
			float enter_;
			float leave_;
		};
		/*! Main constructor.
		 * The box is defined by two points, this constructor just takes them.
		 *
		 * @param a is the low corner (minx,miny,minz)
		 * @param g is the up corner (maxx,maxy,maxz)
		 */
		Bound(const Point3f &a, const Point3f &g) : a_{a}, g_{g} { }
		Bound(Point3f &&a, Point3f &&g) : a_{std::move(a)}, g_{std::move(g)} { }
		//! Default constructors and assignments
		Bound() = default;
		Bound(const Bound &bound) = default;
		Bound(Bound &&bound) = default;
		Bound& operator=(const Bound &bound) = default;
		Bound& operator=(Bound &&bound) = default;
		/*! Two child constructor.
		 * This creates a bound that includes the two given bounds. It's used when
		 * building a bounding tree
		 *
		 * @param r is one child bound
		 * @param l is another child bound
		 */
		Bound(const Bound &r, const Bound &l);
		//! Returns true if the given ray crosses the bound
		//bool cross(const point3d_t &from,const vector3d_t &ray) const;
		//! Returns true if the given ray crosses the bound closer than dist
		//bool cross(const point3d_t &from, const vector3d_t &ray, float dist) const;
		//bool cross(const point3d_t &from, const vector3d_t &ray, float &where, float dist) const;
		Cross cross(const Ray &ray, float t_max) const;
		//! Returns the volume of the bound
		float vol() const;
		//! Returns the length along a certain axis
		float length(Axis axis) const { return g_[axis] - a_[axis]; }
		//! Returns the longest length among all axes
		float longestLength() const { return math::max(length(Axis::X), length(Axis::Y), length(Axis::Z)); }
		//! Cuts the bound in a certain axis to have the given max value
		void setAxisMax(Axis axis, float val) { g_[axis] = val;};
		//! Cuts the bound in a certain axis to have the given min value
		void setAxisMin(Axis axis, float val) { a_[axis] = val;};
		//! Adjust bound size to include point p
		void include(const Point3f &p);
		void include(const Bound &b);
		//! Returns true if the point is inside the bound
		bool includes(const Point3f &pn) const
		{
			return ((pn[Axis::X] >= a_[Axis::X]) && (pn[Axis::X] <= g_[Axis::X]) &&
					(pn[Axis::Y] >= a_[Axis::Y]) && (pn[Axis::Y] <= g_[Axis::Y]) &&
					(pn[Axis::Z] >= a_[Axis::Z]) && (pn[Axis::Z] <= g_[Axis::Z]));
		};
		Axis largestAxis() const
		{
			const Vec3f d{g_ - a_};
			return (d[Axis::X] > d[Axis::Y]) ? ((d[Axis::X] > d[Axis::Z]) ? Axis::X : Axis::Z) : ((d[Axis::Y] > d[Axis::Z]) ? Axis::Y : Axis::Z);
		}

		//	protected: // Lynx; need these to be public.
		//! Two points define the box
		Point3f a_, g_;
};

inline void Bound::include(const Point3f &p)
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

inline void Bound::include(const Bound &b)
{
	include(b.a_);
	include(b.g_);
}

inline Bound::Cross Bound::cross(const Ray &ray, float t_max) const
{
	// Smits method
	const Point3f p{ray.from_ - a_};
	//infinity check initial values
	float lmin = -1e38f;
	float lmax = 1e38f;
	for(const Axis axis : axis::spatial)
	{
		if(ray.dir_[axis] != 0.f)
		{
			const float inv_dir = 1.f / ray.dir_[axis];
			float ltmin, ltmax;
			if(inv_dir > 0.f)
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
	if((lmin <= lmax) && (lmax >= 0.f) && (lmin <= t_max))
	{
		return {true, lmin, lmax};
	}
	else return {};
}

inline Bound operator * (const Bound &b, const Matrix4f &m)
{
	return { m * b.a_, m * b.g_ };
}

} //namespace yafaray

#endif // YAFARAY_BOUND_H
