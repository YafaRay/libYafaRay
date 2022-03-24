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

#include "common/yafaray_common.h"
#include "common/visibility.h"
#include "geometry/poly_double.h"
#include "common/logger.h"
#include "geometry/ray.h"
#include "geometry/intersect_data.h"
#include "geometry/bound.h"
#include <vector>
#include <array>

BEGIN_YAFARAY

class Material;
class Ray;
class SurfacePoint;
class Point3;
class ParamMap;
class Scene;
class Matrix4;
class Vec3;
class Object;
class Camera;

class Primitive
{
	public:
		Primitive() = default;
		virtual ~Primitive() = default;
		/*! return the object bound in global ("world") coordinates */
		virtual Bound getBound(const Matrix4 *obj_to_world) const = 0;
		Bound getBound() const { return getBound(nullptr); }
		/*! a possibly more precise check to find out if the primitve really
			intersects the bound of interest, given that the primitive's bound does.
			used e.g. for optimized kd-tree construction */
		virtual bool intersectsBound(const ExBound &b, const Matrix4 *obj_to_world) const { return true; };
		/*! indicate if the object has a clipping implementation */
		virtual bool clippingSupport() const { return false; }
		/*! basic ray primitive interection for raytracing.
			This should NOT skip intersections outside of [tmin,tmax], unless negative.
			The caller decides wether t matters or not.
			\return false if ray misses primitive, true otherwise
			\param t set this to raydepth where hit occurs */
		virtual IntersectData intersect(const Ray &ray, const Matrix4 *obj_to_world) const;
		IntersectData intersect(const Ray &ray) const { return intersect(ray, nullptr); }
		/* fill in surfacePoint_t */
		virtual std::unique_ptr<const SurfacePoint> getSurface(const RayDifferentials *ray_differentials, const Point3 &hit, const IntersectData &data, const Matrix4 *obj_to_world, const Camera *camera) const;
		/* return the material */
		virtual const Material *getMaterial() const { return nullptr; }
		/* calculate surface area */
		virtual float surfaceArea(const Matrix4 *obj_to_world) const = 0;
		float surfaceArea() const { return surfaceArea(nullptr); }
		/* obtains the geometric normal in the surface parametric u,v coordinates */
		virtual Vec3 getGeometricNormal(const Matrix4 *obj_to_world, float u, float v) const = 0;
		Vec3 getGeometricNormal(const Matrix4 *obj_to_world) const { return getGeometricNormal(obj_to_world, 0.f, 0.f); }
		Vec3 getGeometricNormal() const { return getGeometricNormal(nullptr); }
		/* surface sampling */
		virtual std::pair<Point3, Vec3> sample(float s_1, float s_2, const Matrix4 *obj_to_world) const = 0;
		std::pair<Point3, Vec3> sample(float s_1, float s_2) const { return sample(s_1, s_2, nullptr); }
		virtual const Object *getObject() const = 0;
		virtual Visibility getVisibility() const = 0;
		/*! calculate the overlapping box of given bound and primitive
			\return: false:=doesn't overlap bound; true:=valid clip exists */
		virtual PolyDouble::ClipResultWithBound clipToBound(Logger &logger, const std::array<Vec3Double, 2> &bound, const ClipPlane &clip_plane, const PolyDouble &poly, const Matrix4 *obj_to_world) const;
};

END_YAFARAY

#endif // YAFARAY_PRIMITIVE_H
