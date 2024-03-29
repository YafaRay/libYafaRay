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

#ifndef LIBYAFARAY_PRIMITIVE_H
#define LIBYAFARAY_PRIMITIVE_H

#include "common/visibility.h"
#include "geometry/poly_double.h"
#include "common/logger.h"
#include "geometry/bound.h"
#include <vector>
#include <array>

namespace yafaray {

class Material;
class SurfacePoint;
template <typename T, size_t N> class Point;
typedef Point<float, 3> Point3f;
class ParamMap;
class Scene;
template <typename T, size_t N> class SquareMatrix;
typedef SquareMatrix<float, 4> Matrix4f;
template <typename T, size_t N> class Vec;
typedef Vec<float, 3> Vec3f;
class Camera;
class Rgb;
class Light;
class ParamMeta;

enum class MotionBlurType : unsigned char { None, Bezier };

class Primitive
{
	public:
		virtual ~Primitive() = default;
		[[nodiscard]] virtual std::map<std::string, const ParamMeta *> getParamMetaMap() const { return {}; }
		[[nodiscard]] virtual std::string exportToString(size_t indent_level, yafaray_ContainerExportType container_export_type, bool only_export_non_default_parameters) const = 0;
		/*! return the object bound in global ("world") coordinates */
		virtual Bound<float> getBound() const = 0;
		virtual Bound<float> getBound(const Matrix4f &obj_to_world) const = 0;
		virtual bool clippingSupport() const = 0;
		virtual std::pair<float, Uv<float>> intersect(const Point3f &from, const Vec3f &dir, float time) const = 0;
		virtual std::pair<float, Uv<float>> intersect(const Point3f &from, const Vec3f &dir, float time, const Matrix4f &obj_to_world) const = 0;
		virtual std::unique_ptr<const SurfacePoint> getSurface(const RayDifferentials *ray_differentials, const Point3f &hit_point, float time, const Uv<float> &intersect_uv, const Camera *camera) const = 0;
		virtual std::unique_ptr<const SurfacePoint> getSurface(const RayDifferentials *ray_differentials, const Point3f &hit_point, float time, const Uv<float> &intersect_uv, const Camera *camera, const Matrix4f &obj_to_world) const = 0;
		virtual const Material *getMaterial() const = 0;
		virtual float surfaceArea(float time) const = 0;
		virtual float surfaceArea(float time, const Matrix4f &obj_to_world) const = 0;
		virtual float getDistToNearestEdge(const Uv<float> &uv, const Uv<Vec3f> &dp_abs) const = 0;
		virtual Vec3f getGeometricNormal(const Uv<float> &uv, float time, bool) const = 0;
		virtual Vec3f getGeometricNormal(const Uv<float> &uv, float time, const Matrix4f &obj_to_world) const = 0;
		virtual std::pair<Point3f, Vec3f> sample(const Uv<float> &uv, float time) const = 0;
		virtual std::pair<Point3f, Vec3f> sample(const Uv<float> &uv, float time, const Matrix4f &obj_to_world) const = 0;
		virtual uintptr_t getObjectHandle() const = 0;
		virtual Visibility getVisibility() const = 0;
		virtual int getObjectIndex() const = 0;
		virtual size_t getObjectId() const = 0;
		virtual Rgb getObjectIndexAutoColor() const = 0;
		virtual const Light *getObjectLight() const = 0;
		virtual bool hasMotionBlur() const = 0;
		virtual PolyDouble::ClipResultWithBound clipToBound(Logger &logger, const std::array<Vec3d, 2> &bound, const ClipPlane &clip_plane, const PolyDouble &poly) const;
		virtual PolyDouble::ClipResultWithBound clipToBound(Logger &logger, const std::array<Vec3d, 2> &bound, const ClipPlane &clip_plane, const PolyDouble &poly, const Matrix4f &obj_to_world) const;
};

} //namespace yafaray

#endif // LIBYAFARAY_PRIMITIVE_H
