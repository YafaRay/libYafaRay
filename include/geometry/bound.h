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

#include "constants.h"
#include "ray.h"

BEGIN_YAFARAY

/** Bounding box
 *
 * The bounding box class. A box aligned with the axis used to skip
 * object, photons, and faces intersection when possible.
 *
 */

class Bound
{
		//friend float bound_distance(const bound_t &l,const bound_t &r);
		//friend float b_intersect(const bound_t &l,const bound_t &r);

	public:

		/*! Main constructor.
		 * The box is defined by two points, this constructor just takes them.
		 *
		 * @param a is the low corner (minx,miny,minz)
		 * @param g is the up corner (maxx,maxy,maxz)
		 */
		Bound(const Point3 &a, const Point3 &g) { a_ = a; g_ = g; /* null=false; */ };
		//! Default constructor
		Bound() = default;
		/*! Two child constructor.
		 * This creates a bound that includes the two given bounds. It's used when
		 * building a bounding tree
		 *
		 * @param r is one child bound
		 * @param l is another child bound
		 */
		Bound(const Bound &r, const Bound &l);
		//! Sets the bound like the constructor
		void set(const Point3 &a, const Point3 &g) { a_ = a; g_ = g; };
		void get(Point3 &a, Point3 &g) const { a = a_; g = g_; };

		//! Returns true if the given ray crosses the bound
		//bool cross(const point3d_t &from,const vector3d_t &ray) const;
		//! Returns true if the given ray crosses the bound closer than dist
		//bool cross(const point3d_t &from, const vector3d_t &ray, float dist) const;
		//bool cross(const point3d_t &from, const vector3d_t &ray, float &where, float dist) const;
		bool cross(const Ray &ray, float &enter, float &leave, const float dist) const;

		//! Returns the volume of the bound
		float vol() const;
		//! Returns the lenght along X axis
		float longX() const {return g_.x_ - a_.x_;};
		//! Returns the lenght along Y axis
		float longY() const {return g_.y_ - a_.y_;};
		//! Returns the lenght along Y axis
		float longZ() const {return g_.z_ - a_.z_;};
		//! Cuts the bound to have the given max X
		void setMaxX(float x) { g_.x_ = x;};
		//! Cuts the bound to have the given min X
		void setMinX(float x) { a_.x_ = x;};

		//! Cuts the bound to have the given max Y
		void setMaxY(float y) { g_.y_ = y;};
		//! Cuts the bound to have the given min Y
		void setMinY(float y) { a_.y_ = y;};

		//! Cuts the bound to have the given max Z
		void setMaxZ(float z) { g_.z_ = z;};
		//! Cuts the bound to have the given min Z
		void setMinZ(float z) { a_.z_ = z;};
		//! Adjust bound size to include point p
		void include(const Point3 &p);
		//! Returns true if the point is inside the bound
		bool includes(const Point3 &pn) const
		{
			return ((pn.x_ >= a_.x_) && (pn.x_ <= g_.x_) &&
					(pn.y_ >= a_.y_) && (pn.y_ <= g_.y_) &&
					(pn.z_ >= a_.z_) && (pn.z_ <= g_.z_));
		};
		float centerX() const { return (g_.x_ + a_.x_) * 0.5f; }
		float centerY() const { return (g_.y_ + a_.y_) * 0.5f; }
		float centerZ() const { return (g_.z_ + a_.z_) * 0.5f; }
		Point3 center() const { return (g_ + a_) * 0.5f; }
		int largestAxis()
		{
			Vec3 d = g_ - a_;
			return (d.x_ > d.y_) ? ((d.x_ > d.z_) ? 0 : 2) : ((d.y_ > d.z_) ? 1 : 2);
		}
		void grow(float d)
		{
			a_.x_ -= d;
			a_.y_ -= d;
			a_.z_ -= d;
			g_.x_ += d;
			g_.y_ += d;
			g_.z_ += d;
		};

		//	protected: // Lynx; need these to be public.
		//! Two points define the box
		Point3 a_, g_;
};

inline void Bound::include(const Point3 &p)
{
	a_.x_ = std::min(a_.x_, p.x_);
	a_.y_ = std::min(a_.y_, p.y_);
	a_.z_ = std::min(a_.z_, p.z_);
	g_.x_ = std::max(g_.x_, p.x_);
	g_.y_ = std::max(g_.y_, p.y_);
	g_.z_ = std::max(g_.z_, p.z_);
}

inline bool Bound::cross(const Ray &ray, float &enter, float &leave, const float dist) const
{
	// Smits method
	const Point3 &a_0 = a_, &a_1 = g_;
	const Point3 &p = ray.from_ - a_0;

	float lmin = -1e38f, lmax = 1e38f, ltmin, ltmax; //infinity check initial values

	if(ray.dir_.x_ != 0)
	{
		float invrx = 1.f / ray.dir_.x_;
		if(invrx > 0)
		{
			lmin = -p.x_ * invrx;
			lmax = ((a_1.x_ - a_0.x_) - p.x_) * invrx;
		}
		else
		{
			lmin = ((a_1.x_ - a_0.x_) - p.x_) * invrx;
			lmax = -p.x_ * invrx;
		}

		if((lmax < 0) || (lmin > dist)) return false;
	}
	if(ray.dir_.y_ != 0)
	{
		float invry = 1.f / ray.dir_.y_;
		if(invry > 0)
		{
			ltmin = -p.y_ * invry;
			ltmax = ((a_1.y_ - a_0.y_) - p.y_) * invry;
		}
		else
		{
			ltmin = ((a_1.y_ - a_0.y_) - p.y_) * invry;
			ltmax = -p.y_ * invry;
		}
		lmin = std::max(ltmin, lmin);
		lmax = std::min(ltmax, lmax);

		if((lmax < 0) || (lmin > dist)) return false;
	}
	if(ray.dir_.z_ != 0)
	{
		float invrz = 1.f / ray.dir_.z_;
		if(invrz > 0)
		{
			ltmin = -p.z_ * invrz;
			ltmax = ((a_1.z_ - a_0.z_) - p.z_) * invrz;
		}
		else
		{
			ltmin = ((a_1.z_ - a_0.z_) - p.z_) * invrz;
			ltmax = -p.z_ * invrz;
		}
		lmin = std::max(ltmin, lmin);
		lmax = std::min(ltmax, lmax);

		if((lmax < 0) || (lmin > dist)) return false;
	}
	if((lmin <= lmax) && (lmax >= 0) && (lmin <= dist))
	{
		enter = lmin;   //(lmin>0) ? lmin : 0;
		leave = lmax;
		return true;
	}

	return false;
}


class ExBound: public Bound
{
	public:
		ExBound(const Bound &b)
		{
			for(int i = 0; i < 3; ++i)
			{
				center_[i] = (static_cast<double>(a_[i]) + static_cast<double>(g_[i])) * 0.5;
				half_size_[i] = (static_cast<double>(g_[i]) - static_cast<double>(a_[i])) * 0.5;
			}
		}

		double center_[3];
		double half_size_[3];
};


END_YAFARAY

#endif // YAFARAY_BOUND_H
