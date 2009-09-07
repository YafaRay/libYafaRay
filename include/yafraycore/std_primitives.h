
#ifndef Y_STD_PRIMITIVE_H
#define Y_STD_PRIMITIVE_H

#include <core_api/primitive.h>

__BEGIN_YAFRAY

class renderEnvironment_t;
class paraMap_t;
class object3d_t;

class YAFRAYCORE_EXPORT sphere_t: public primitive_t
{
	public:
		sphere_t(point3d_t centr, PFLOAT rad, const material_t *m): center(centr), radius(rad), material(m) {}
		virtual bound_t getBound() const;
		virtual bool intersectsBound(exBound_t &b) const { return true; };
		//virtual bool clippingSupport() const { return false; }
		//virtual bool clipToBound(double bound[2][3], int axis, bound_t &clipped, void *d_old, void *d_new) const {return false;}
		virtual bool intersect(const ray_t &ray, PFLOAT *t, void *userdata) const;
		virtual void getSurface(surfacePoint_t &sp, const point3d_t &hit, void *userdata) const;
		virtual const material_t* getMaterial() const { return material; }
	protected:
		point3d_t center;
		PFLOAT radius;
		const material_t *material;
};

object3d_t* sphere_factory(paraMap_t &params, renderEnvironment_t &env);

__END_YAFRAY

#endif //Y_STD_PRIMITIVE_H
