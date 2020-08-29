#pragma once


#ifndef YAFARAY_STD_PRIMITIVES_H
#define YAFARAY_STD_PRIMITIVES_H

#include <yafray_constants.h>
#include <core_api/primitive.h>

BEGIN_YAFRAY

class RenderEnvironment;
class ParamMap;
class Object3D;

class YAFRAYCORE_EXPORT Sphere: public Primitive
{
	public:
		Sphere(Point3 centr, float rad, const Material *m): center_(centr), radius_(rad), material_(m) {}
		virtual Bound getBound() const;
		virtual bool intersectsBound(ExBound &b) const { return true; };
		//virtual bool clippingSupport() const { return false; }
		//virtual bool clipToBound(double bound[2][3], int axis, bound_t &clipped, void *d_old, void *d_new) const {return false;}
		virtual bool intersect(const Ray &ray, float *t, IntersectData &data) const;
		virtual void getSurface(SurfacePoint &sp, const Point3 &hit, IntersectData &data) const;
		virtual const Material *getMaterial() const { return material_; }
		virtual const TriangleObject *getMesh() const { return nullptr; }
	protected:
		Point3 center_;
		float radius_;
		const Material *material_ = nullptr;
};

Object3D *sphereFactory__(ParamMap &params, RenderEnvironment &env);

END_YAFRAY

#endif //YAFARAY_STD_PRIMITIVES_H
