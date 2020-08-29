#pragma once

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
