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

#include <array>
#include "constants.h"
#include "common/visibility.h"

BEGIN_YAFARAY

class Material;
struct IntersectData;
class Bound;
class ExBound;
class Ray;
class SurfacePoint;
class Point3;
class ParamMap;
class Scene;
class Matrix4;
class Vec3;
class Object;

class Primitive
{
	public:
		Primitive(const Object &base_object) : base_object_(base_object) { }
		virtual ~Primitive() = default;
		/*! return the object bound in global ("world") coordinates */
		virtual Bound getBound(const Matrix4 *obj_to_world = nullptr) const = 0;
		/*! a possibly more precise check to find out if the primitve really
			intersects the bound of interest, given that the primitive's bound does.
			used e.g. for optimized kd-tree construction */
		virtual bool intersectsBound(const ExBound &b, const Matrix4 *obj_to_world) const { return true; };
		/*! indicate if the object has a clipping implementation */
		virtual bool clippingSupport() const { return false; }
		/*! calculate the overlapping box of given bound and primitive
			\return: false:=doesn't overlap bound; true:=valid clip exists */
		virtual bool clipToBound(const std::array<std::array<double, 3>, 2> &bound, int axis, Bound &clipped, const void *d_old, void *d_new, const Matrix4 *obj_to_world) const { return false; }
		/*! basic ray primitive interection for raytracing.
			This should NOT skip intersections outside of [tmin,tmax], unless negative.
			The caller decides wether t matters or not.
			\return false if ray misses primitive, true otherwise
			\param t set this to raydepth where hit occurs */
		virtual IntersectData intersect(const Ray &ray, const Matrix4 *obj_to_world = nullptr) const;
		/* fill in surfacePoint_t */
		virtual SurfacePoint getSurface(const Point3 &hit, const IntersectData &data, const Matrix4 *obj_to_world = nullptr) const;
		/* return the material */
		virtual const Material *getMaterial() const { return nullptr; }
		/* calculate surface area */
		virtual float surfaceArea(const Matrix4 *obj_to_world = nullptr) const = 0;
		/* obtains the geometric normal in the surface parametric u,v coordinates */
		virtual Vec3 getGeometricNormal(const Matrix4 *obj_to_world = nullptr, float u = 0.f, float v = 0.f) const = 0;
		/* surface sampling */
		virtual void sample(float s_1, float s_2, Point3 &p, Vec3 &n, const Matrix4 *obj_to_world = nullptr) const = 0;
		const Object *getObject() const { return &base_object_; }
		Visibility getVisibility() const;

	protected:
		const Object &base_object_;
};

END_YAFARAY

#endif // YAFARAY_PRIMITIVE_H
