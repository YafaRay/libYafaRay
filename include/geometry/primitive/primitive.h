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
#include "geometry/bound.h"
#include <vector>
#include <array>

BEGIN_YAFARAY

class Material;
class SurfacePoint;
class Point3;
class ParamMap;
class Scene;
class Matrix4;
class Vec3;
class Object;
class Camera;
class Rgb;
class Light;

class Primitive
{
	public:
		virtual ~Primitive() = default;
		/*! return the object bound in global ("world") coordinates */
		virtual Bound getBound() const = 0;
		virtual Bound getBound(const Matrix4 &obj_to_world) const = 0;
		virtual bool clippingSupport() const = 0;
		virtual std::pair<float, Uv<float>> intersect(const Point3 &from, const Vec3 &dir, float time) const = 0;
		virtual std::pair<float, Uv<float>> intersect(const Point3 &from, const Vec3 &dir, float time, const Matrix4 &obj_to_world) const = 0;
		virtual std::unique_ptr<const SurfacePoint> getSurface(const RayDifferentials *ray_differentials, const Point3 &hit_point, float time, const Uv<float> &intersect_uv, const Camera *camera) const = 0;
		virtual std::unique_ptr<const SurfacePoint> getSurface(const RayDifferentials *ray_differentials, const Point3 &hit_point, float time, const Uv<float> &intersect_uv, const Camera *camera, const Matrix4 &obj_to_world) const = 0;
		virtual const Material *getMaterial() const = 0;
		virtual float surfaceArea(float time) const = 0;
		virtual float surfaceArea(float time, const Matrix4 &obj_to_world) const = 0;
		virtual float getDistToNearestEdge(const Uv<float> &uv, const Uv<Vec3> &dp_abs) const = 0;
		virtual Vec3 getGeometricNormal(const Uv<float> &uv, float time) const = 0;
		virtual Vec3 getGeometricNormal(const Uv<float> &uv, float time, const Matrix4 &obj_to_world) const = 0;
		Vec3 getGeometricNormal(float time) const { return getGeometricNormal({0.f, 0.f}, time); }
		Vec3 getGeometricNormal(float time, const Matrix4 &obj_to_world) const { return getGeometricNormal({0.f, 0.f}, time, obj_to_world); }
		virtual std::pair<Point3, Vec3> sample(const Uv<float> &uv, float time) const = 0;
		virtual std::pair<Point3, Vec3> sample(const Uv<float> &uv, float time, const Matrix4 &obj_to_world) const = 0;
		virtual const Object *getObject() const = 0;
		virtual Visibility getVisibility() const = 0;
		virtual unsigned int getObjectIndex() const = 0;
		virtual unsigned int getObjectIndexAuto() const = 0;
		virtual Rgb getObjectIndexAutoColor() const = 0;
		virtual const Light *getObjectLight() const = 0;
		virtual bool hasObjectMotionBlur() const = 0;
		virtual PolyDouble::ClipResultWithBound clipToBound(Logger &logger, const std::array<Vec3Double, 2> &bound, const ClipPlane &clip_plane, const PolyDouble &poly) const;
		virtual PolyDouble::ClipResultWithBound clipToBound(Logger &logger, const std::array<Vec3Double, 2> &bound, const ClipPlane &clip_plane, const PolyDouble &poly, const Matrix4 &obj_to_world) const;
};

END_YAFARAY

#endif // YAFARAY_PRIMITIVE_H
