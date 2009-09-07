
#ifndef Y_PRIMITIVE_H
#define Y_PRIMITIVE_H

#include <yafray_config.h>

#include "surface.h"
#include "bound.h"
#include "ray.h"

__BEGIN_YAFRAY

class triangleObject_t;
class material_t;

class YAFRAYCORE_EXPORT primitive_t
{
	public:
	/*! return the object bound in global ("world") coordinates */
	virtual bound_t getBound() const = 0;
	/*! a possibly more precise check to find out if the primitve really
		intersects the bound of interest, given that the primitive's bound does.
		used e.g. for optimized kd-tree construction */
	virtual bool intersectsBound(exBound_t &b) const { return true; };
	/*! indicate if the object has a clipping implementation */
	virtual bool clippingSupport() const { return false; }
	/*! calculate the overlapping box of given bound and primitive
		\return: false:=doesn't overlap bound; true:=valid clip exists */
	virtual bool clipToBound(double bound[2][3], int axis, bound_t &clipped, void *d_old, void *d_new) const {return false;}
	/*! basic ray primitive interection for raytracing.
		This should NOT skip intersections outside of [tmin,tmax], unless negative.
		The caller decides wether t matters or not.
		\return false if ray misses primitive, true otherwise
		\param t set this to raydepth where hit occurs */
	virtual bool intersect(const ray_t &ray, PFLOAT *t, void *userdata) const = 0;
	/* fill in surfacePoint_t */
	virtual void getSurface(surfacePoint_t &sp, const point3d_t &hit, void *userdata) const = 0;
	/* return the material */
	virtual const material_t* getMaterial() const = 0;
	virtual ~primitive_t(){};
};

__END_YAFRAY

#endif // Y_PRIMITIVE_H
