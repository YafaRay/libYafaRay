#pragma once
/****************************************************************************
 *      This is part of the libYafaRay package
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
 */

#ifndef YAFARAY_PRIMITIVE_H
#define YAFARAY_PRIMITIVE_H

#include <yafray_constants.h>
#include "surface.h"
#include "bound.h"
#include "ray.h"

BEGIN_YAFRAY

class TriangleObject;
class Material;

class YAFRAYCORE_EXPORT Primitive
{
	public:
		/*! return the object bound in global ("world") coordinates */
		virtual Bound getBound() const = 0;
		/*! a possibly more precise check to find out if the primitve really
			intersects the bound of interest, given that the primitive's bound does.
			used e.g. for optimized kd-tree construction */
		virtual bool intersectsBound(ExBound &b) const { return true; };
		/*! indicate if the object has a clipping implementation */
		virtual bool clippingSupport() const { return false; }
		/*! calculate the overlapping box of given bound and primitive
			\return: false:=doesn't overlap bound; true:=valid clip exists */
		virtual bool clipToBound(double bound[2][3], int axis, Bound &clipped, void *d_old, void *d_new) const {return false;}
		/*! basic ray primitive interection for raytracing.
			This should NOT skip intersections outside of [tmin,tmax], unless negative.
			The caller decides wether t matters or not.
			\return false if ray misses primitive, true otherwise
			\param t set this to raydepth where hit occurs */
		virtual bool intersect(const Ray &ray, float *t, IntersectData &data) const = 0;
		/* fill in surfacePoint_t */
		virtual void getSurface(SurfacePoint &sp, const Point3 &hit, IntersectData &data) const = 0;
		/* return the material */
		virtual const Material *getMaterial() const = 0;
		virtual ~Primitive() {};
};

END_YAFRAY

#endif // YAFARAY_PRIMITIVE_H
