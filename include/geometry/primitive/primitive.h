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
		virtual ~Primitive() = default;
		/*! return the object bound in global ("world") coordinates */
		virtual Bound getBound() const = 0;
		virtual Bound getBound(const Matrix4 *obj_to_world) const = 0;
		virtual bool clippingSupport() const { return false; }
		virtual IntersectData intersect(const Ray &ray) const = 0;
		virtual IntersectData intersect(const Ray &ray, const Matrix4 *obj_to_world) const;
		virtual std::unique_ptr<const SurfacePoint> getSurface(const RayDifferentials *ray_differentials, const Point3 &hit, const IntersectData &data, const Matrix4 *obj_to_world, const Camera *camera) const;
		virtual const Material *getMaterial() const { return nullptr; }
		virtual float surfaceArea(const Matrix4 *obj_to_world) const = 0;
		float surfaceArea() const { return surfaceArea(nullptr); }
		virtual float getDistToNearestEdge(float u, float v, const Vec3 &dp_du_abs, const Vec3 &dp_dv_abs) const = 0;
		virtual Vec3 getGeometricFaceNormal(const Matrix4 *obj_to_world, float u, float v) const = 0;
		Vec3 getGeometricFaceNormal(const Matrix4 *obj_to_world) const { return getGeometricFaceNormal(obj_to_world, 0.f, 0.f); }
		Vec3 getGeometricFaceNormal() const { return getGeometricFaceNormal(nullptr); }
		virtual std::pair<Point3, Vec3> sample(float s_1, float s_2, const Matrix4 *obj_to_world) const = 0;
		std::pair<Point3, Vec3> sample(float s_1, float s_2) const { return sample(s_1, s_2, nullptr); }
		virtual const Object *getObject() const = 0;
		virtual Visibility getVisibility() const = 0;
		virtual PolyDouble::ClipResultWithBound clipToBound(Logger &logger, const std::array<Vec3Double, 2> &bound, const ClipPlane &clip_plane, const PolyDouble &poly, const Matrix4 *obj_to_world) const;
};

END_YAFARAY

#endif // YAFARAY_PRIMITIVE_H
