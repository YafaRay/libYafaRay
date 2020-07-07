#pragma once
/****************************************************************************
 *
 * 			bound.h: Bound and tree api for general raytracing acceleration
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

#ifndef Y_BOUND_H
#define Y_BOUND_H

#include <yafray_constants.h>
#include "ray.h"

__BEGIN_YAFRAY

/** Bounding box
 *
 * The bounding box class. A box aligned with the axis used to skip
 * object, photons, and faces intersection when possible.
 *
 */

class bound_t;
YAFRAYCORE_EXPORT float bound_distance(const bound_t &l,const bound_t &r);
float b_intersect(const bound_t &l,const bound_t &r);

class YAFRAYCORE_EXPORT bound_t 
{
	//friend YAFRAYCORE_EXPORT float bound_distance(const bound_t &l,const bound_t &r);
	//friend float b_intersect(const bound_t &l,const bound_t &r);
	
	public:

		/*! Main constructor.
		 * The box is defined by two points, this constructor just takes them.
		 *
		 * @param _a is the low corner (minx,miny,minz)
		 * @param _g is the up corner (maxx,maxy,maxz)
		 */
		bound_t(const point3d_t & _a,const point3d_t & _g) { a=_a; g=_g; /* null=false; */ };
		//! Default constructor
		bound_t() {};

		/*! Two child constructor.
		 * This creates a bound that includes the two given bounds. It's used when
		 * building a bounding tree
		 * 
		 * @param r is one child bound
		 * @param l is another child bound
		 */
		bound_t(const bound_t &r,const bound_t &l);
		//! Sets the bound like the constructor
		void set(const point3d_t &_a,const point3d_t &_g) { a=_a; g=_g; };
		void get(point3d_t &_a,point3d_t &_g)const { _a=a; _g=g; };

		//! Returns true if the given ray crosses the bound
		//bool cross(const point3d_t &from,const vector3d_t &ray)const;
		//! Returns true if the given ray crosses the bound closer than dist
		//bool cross(const point3d_t &from, const vector3d_t &ray, float dist)const;
		//bool cross(const point3d_t &from, const vector3d_t &ray, float &where, float dist)const;
		bool cross(const ray_t &ray, float &enter, float &leave, const float dist)const;

		//! Returns the volume of the bound
		float vol() const;
		//! Returns the lenght along X axis
		float longX()const {return g.x-a.x;};
		//! Returns the lenght along Y axis
		float longY()const {return g.y-a.y;};
		//! Returns the lenght along Y axis
		float longZ()const {return g.z-a.z;};
		//! Cuts the bound to have the given max X
		void setMaxX(float X) {g.x=X;};
		//! Cuts the bound to have the given min X
		void setMinX(float X) {a.x=X;};
		
		//! Cuts the bound to have the given max Y
		void setMaxY(float Y) {g.y=Y;};
		//! Cuts the bound to have the given min Y
		void setMinY(float Y) {a.y=Y;};

		//! Cuts the bound to have the given max Z
		void setMaxZ(float Z) {g.z=Z;};
		//! Cuts the bound to have the given min Z
		void setMinZ(float Z) {a.z=Z;};
		//! Adjust bound size to include point p
		void include(const point3d_t &p);
		//! Returns true if the point is inside the bound
		bool includes(const point3d_t &pn)const
		{
			return ( ( pn.x >= a.x ) && ( pn.x <= g.x) &&
				 ( pn.y >= a.y ) && ( pn.y <= g.y) &&
				 ( pn.z >= a.z ) && ( pn.z <= g.z) );
		};
		float centerX()const {return (g.x+a.x)*0.5;};
		float centerY()const {return (g.y+a.y)*0.5;};
		float centerZ()const {return (g.z+a.z)*0.5;};
		point3d_t center()const {return (g+a)*0.5;};
		int largestAxis()
		{
			vector3d_t d = g-a;
			return (d.x>d.y) ? ((d.x>d.z) ? 0 : 2) : ((d.y>d.z) ? 1:2 );
		}
		void grow(float d)
		{
			a.x-=d;
			a.y-=d;
			a.z-=d;
			g.x+=d;
			g.y+=d;
			g.z+=d;
		};
		
//	protected: // Lynx; need these to be public.
		//! Two points define the box
		point3d_t a,g;
};

inline void bound_t::include(const point3d_t &p)
{
	a.x = std::min(a.x, p.x);
	a.y = std::min(a.y, p.y);
	a.z = std::min(a.z, p.z);
	g.x = std::max(g.x, p.x);
	g.y = std::max(g.y, p.y);
	g.z = std::max(g.z, p.z);
}

inline bool bound_t::cross(const ray_t &ray, float &enter, float &leave, const float dist)const
{
	// Smits method
	const point3d_t &a0=a,&a1=g;
	const point3d_t &p = ray.from-a0;

	float lmin=-1e38, lmax=1e38, ltmin, ltmax; //infinity check initial values

	if (ray.dir.x != 0){
		float invrx = 1./ray.dir.x;
		if (invrx > 0){
			lmin = -p.x * invrx;
			lmax = ((a1.x - a0.x) - p.x) * invrx;
		}else{
			lmin = ((a1.x - a0.x) - p.x) * invrx;
			lmax = -p.x * invrx;
		}

		if((lmax<0) || (lmin>dist)) return false;
	}
	if (ray.dir.y != 0){
		float invry = 1./ray.dir.y;
		if (invry > 0){
			ltmin = -p.y * invry;
			ltmax = ((a1.y - a0.y) - p.y) * invry;
		}else{
			ltmin = ((a1.y - a0.y) - p.y) * invry;
			ltmax = -p.y * invry;
		}
		lmin = std::max(ltmin,lmin);
		lmax = std::min(ltmax,lmax);

		if((lmax<0) || (lmin>dist)) return false;
	}
	if (ray.dir.z != 0){
		float invrz = 1./ray.dir.z;
		if (invrz > 0){
			ltmin = -p.z * invrz;
			ltmax = ((a1.z - a0.z) - p.z) * invrz;
		}else{
			ltmin = ((a1.z - a0.z) - p.z) * invrz;
			ltmax = -p.z * invrz;
		}
		lmin = std::max(ltmin,lmin);
		lmax = std::min(ltmax,lmax);

		if((lmax<0) || (lmin>dist)) return false;
	}
	if((lmin <= lmax) && (lmax >= 0) && (lmin<=dist))
	{
		enter=lmin;     //(lmin>0) ? lmin : 0;
		leave=lmax;
		return true;
	}
	
	return false;
}


class YAFRAYCORE_EXPORT exBound_t: public bound_t
{
        public:
		exBound_t(const bound_t &b)
		{
			for(int i=0;i<3;++i)
			{
				center[i]   = ((double)a[i] + (double)g[i])*0.5;
				halfSize[i] = ((double)g[i] - (double)a[i])*0.5;
			}
		}

	double center[3];
	double halfSize[3];
};


__END_YAFRAY

#endif // Y_BOUND_H
