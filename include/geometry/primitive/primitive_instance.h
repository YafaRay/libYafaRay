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

#ifndef LIBYAFARAY_PRIMITIVE_INSTANCE_H
#define LIBYAFARAY_PRIMITIVE_INSTANCE_H

#include "color/color.h"
#include "geometry/instance.h"
#include "primitive.h"
#include <array>

namespace yafaray {

class Material;
template<typename T> class Bound;
class SurfacePoint;
template <typename T, size_t N> class Point;
typedef Point<float, 3> Point3f;
class ParamMap;
class Scene;

class PrimitiveInstance final : public Primitive
{
	public:
		PrimitiveInstance(const Primitive &base_primitive, const Instance &base_instance) : base_instance_(base_instance), base_primitive_(base_primitive) { }
		Bound<float> getBound() const override;
		Bound<float> getBound(const Matrix4f &obj_to_world) const override;
		bool clippingSupport() const override { return base_primitive_.clippingSupport() && !hasMotionBlur(); }
		PolyDouble::ClipResultWithBound clipToBound(Logger &logger, const std::array<Vec3d, 2> &bound, const ClipPlane &clip_plane, const PolyDouble &poly) const override;
		PolyDouble::ClipResultWithBound clipToBound(Logger &logger, const std::array<Vec3d, 2> &bound, const ClipPlane &clip_plane, const PolyDouble &poly, const Matrix4f &obj_to_world) const override;
		std::pair<float, Uv<float>> intersect(const Point3f &from, const Vec3f &dir, float time) const override;
		std::pair<float, Uv<float>> intersect(const Point3f &from, const Vec3f &dir, float time, const Matrix4f &obj_to_world) const override;
		std::unique_ptr<const SurfacePoint> getSurface(const RayDifferentials *ray_differentials, const Point3f &hit_point, float time, const Uv<float> &intersect_uv, const Camera *camera) const override;
		std::unique_ptr<const SurfacePoint> getSurface(const RayDifferentials *ray_differentials, const Point3f &hit_point, float time, const Uv<float> &intersect_uv, const Camera *camera, const Matrix4f &obj_to_world) const override;
		const Material *getMaterial() const override { return base_primitive_.getMaterial(); }
		float surfaceArea(float time) const override;
		float surfaceArea(float time, const Matrix4f &obj_to_world) const override;
		Vec3f getGeometricNormal(const Uv<float> &uv, float time, bool) const override;
		Vec3f getGeometricNormal(const Uv<float> &uv, float time, const Matrix4f &obj_to_world) const override;
		std::pair<Point3f, Vec3f> sample(const Uv<float> &uv, float time) const override;
		std::pair<Point3f, Vec3f> sample(const Uv<float> &uv, float time, const Matrix4f &obj_to_world) const override;
		uintptr_t getObjectHandle() const override { return reinterpret_cast<uintptr_t>(&base_instance_); }
		Visibility getVisibility() const override { return base_primitive_.getVisibility(); }
		int getObjectIndex() const override { return base_primitive_.getObjectIndex(); }
		size_t getObjectId() const override { return base_primitive_.getObjectId(); }
		Rgb getObjectIndexAutoColor() const override { return base_primitive_.getObjectIndexAutoColor(); }
		const Light *getObjectLight() const override { return base_primitive_.getObjectLight(); }
		bool hasMotionBlur() const override { return base_instance_.hasMotionBlur(); }
		float getDistToNearestEdge(const Uv<float> &uv, const Uv<Vec3f> &dp_abs) const override { return base_primitive_.getDistToNearestEdge(uv, dp_abs); }

	private:
		const Instance &base_instance_;
		const Primitive &base_primitive_;
};

inline PolyDouble::ClipResultWithBound PrimitiveInstance::clipToBound(Logger &logger, const std::array<Vec3d, 2> &bound, const ClipPlane &clip_plane, const PolyDouble &poly) const
{
	return base_primitive_.clipToBound(logger, bound, clip_plane, poly, base_instance_.getObjToWorldMatrix(0));
}

inline PolyDouble::ClipResultWithBound PrimitiveInstance::clipToBound(Logger &logger, const std::array<Vec3d, 2> &bound, const ClipPlane &clip_plane, const PolyDouble &poly, const Matrix4f &obj_to_world) const
{
	return base_primitive_.clipToBound(logger, bound, clip_plane, poly, obj_to_world * base_instance_.getObjToWorldMatrix(0));
}

inline std::pair<float, Uv<float>> PrimitiveInstance::intersect(const Point3f &from, const Vec3f &dir, float time) const
{
	return base_primitive_.intersect(from, dir, time, base_instance_.getObjToWorldMatrixAtTime(time));
}

inline std::pair<float, Uv<float>> PrimitiveInstance::intersect(const Point3f &from, const Vec3f &dir, float time, const Matrix4f &obj_to_world) const
{
	return base_primitive_.intersect(from, dir, time, obj_to_world * base_instance_.getObjToWorldMatrixAtTime(time));
}

inline float PrimitiveInstance::surfaceArea(float time) const
{
	return base_primitive_.surfaceArea(time, base_instance_.getObjToWorldMatrixAtTime(time));
}

inline float PrimitiveInstance::surfaceArea(float time, const Matrix4f &obj_to_world) const
{
	return base_primitive_.surfaceArea(time, obj_to_world * base_instance_.getObjToWorldMatrixAtTime(time));
}

inline Vec3f PrimitiveInstance::getGeometricNormal(const Uv<float> &uv, float time, bool) const
{
	return base_primitive_.getGeometricNormal(uv, time, base_instance_.getObjToWorldMatrixAtTime(time));
}

inline Vec3f PrimitiveInstance::getGeometricNormal(const Uv<float> &uv, float time, const Matrix4f &obj_to_world) const
{
	return base_primitive_.getGeometricNormal(uv, time, obj_to_world * base_instance_.getObjToWorldMatrixAtTime(time));
}

inline std::pair<Point3f, Vec3f> PrimitiveInstance::sample(const Uv<float> &uv, float time) const
{
	return base_primitive_.sample(uv, time, base_instance_.getObjToWorldMatrixAtTime(time));
}

inline std::pair<Point3f, Vec3f> PrimitiveInstance::sample(const Uv<float> &uv, float time, const Matrix4f &obj_to_world) const
{
	return base_primitive_.sample(uv, time, obj_to_world * base_instance_.getObjToWorldMatrixAtTime(time));
}

inline Bound<float> PrimitiveInstance::getBound() const
{
	const std::vector<const Matrix4f *> matrices = base_instance_.getObjToWorldMatrices();
	Bound result{base_primitive_.getBound(*matrices[0])};
	for(size_t i = 1; i < matrices.size(); ++i)
	{
		result.include(base_primitive_.getBound(*matrices[i]));
	}
	return result;
}

inline Bound<float> PrimitiveInstance::getBound(const Matrix4f &obj_to_world) const
{
	const std::vector<const Matrix4f *> matrices = base_instance_.getObjToWorldMatrices();
	Bound result{base_primitive_.getBound(obj_to_world * *(matrices[0]))};
	for(size_t i = 1; i < matrices.size(); ++i)
	{
		result.include(base_primitive_.getBound(obj_to_world * *(matrices[i])));
	}
	return result;
}

} //namespace yafaray

#endif //LIBYAFARAY_PRIMITIVE_INSTANCE_H
