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

#ifndef YAFARAY_PRIMITIVE_INSTANCE_H
#define YAFARAY_PRIMITIVE_INSTANCE_H

#include "color/color.h"
#include "geometry/object/object_instance.h"
#include "primitive.h"
#include <array>

namespace yafaray {

class Material;
class Bound;
class SurfacePoint;
class Point3;
class ParamMap;
class Scene;

class PrimitiveInstance : public Primitive
{
	public:
		PrimitiveInstance(const Primitive *base_primitive, const ObjectInstance &base_instance) : base_instance_(base_instance), base_primitive_(base_primitive) { }
		Bound getBound() const override;
		Bound getBound(const Matrix4 &obj_to_world) const override;
		bool clippingSupport() const override { return base_primitive_->clippingSupport() && !base_instance_.hasMotionBlur(); }
		PolyDouble::ClipResultWithBound clipToBound(Logger &logger, const std::array<Vec3Double, 2> &bound, const ClipPlane &clip_plane, const PolyDouble &poly) const override;
		PolyDouble::ClipResultWithBound clipToBound(Logger &logger, const std::array<Vec3Double, 2> &bound, const ClipPlane &clip_plane, const PolyDouble &poly, const Matrix4 &obj_to_world) const override;
		std::pair<float, Uv<float>> intersect(const Point3 &from, const Vec3 &dir, float time) const override;
		std::pair<float, Uv<float>> intersect(const Point3 &from, const Vec3 &dir, float time, const Matrix4 &obj_to_world) const override;
		std::unique_ptr<const SurfacePoint> getSurface(const RayDifferentials *ray_differentials, const Point3 &hit_point, float time, const Uv<float> &intersect_uv, const Camera *camera) const override;
		std::unique_ptr<const SurfacePoint> getSurface(const RayDifferentials *ray_differentials, const Point3 &hit_point, float time, const Uv<float> &intersect_uv, const Camera *camera, const Matrix4 &obj_to_world) const override;
		const Material *getMaterial() const override { return base_primitive_->getMaterial(); }
		float surfaceArea(float time) const override;
		float surfaceArea(float time, const Matrix4 &obj_to_world) const override;
		Vec3 getGeometricNormal(const Uv<float> &uv, float time, bool) const override;
		Vec3 getGeometricNormal(const Uv<float> &uv, float time, const Matrix4 &obj_to_world) const override;
		std::pair<Point3, Vec3> sample(const Uv<float> &uv, float time) const override;
		std::pair<Point3, Vec3> sample(const Uv<float> &uv, float time, const Matrix4 &obj_to_world) const override;
		const Object *getObject() const override { return &base_instance_; }
		Visibility getVisibility() const override { return base_primitive_->getVisibility(); }
		unsigned int getObjectIndex() const override { return base_primitive_->getObjectIndex(); }
		unsigned int getObjectIndexAuto() const override { return base_primitive_->getObjectIndexAuto(); }
		Rgb getObjectIndexAutoColor() const override { return base_primitive_->getObjectIndexAutoColor(); }
		const Light *getObjectLight() const override { return base_primitive_->getObjectLight(); }
		bool hasObjectMotionBlur() const override { return base_primitive_->hasObjectMotionBlur(); }
		float getDistToNearestEdge(const Uv<float> &uv, const Uv<Vec3> &dp_abs) const override { return base_primitive_->getDistToNearestEdge(uv, dp_abs); }

	private:
		const ObjectInstance &base_instance_;
		const Primitive *base_primitive_ = nullptr;
};

inline PolyDouble::ClipResultWithBound PrimitiveInstance::clipToBound(Logger &logger, const std::array<Vec3Double, 2> &bound, const ClipPlane &clip_plane, const PolyDouble &poly) const
{
	return base_primitive_->clipToBound(logger, bound, clip_plane, poly, base_instance_.getObjToWorldMatrix(0));
}

inline PolyDouble::ClipResultWithBound PrimitiveInstance::clipToBound(Logger &logger, const std::array<Vec3Double, 2> &bound, const ClipPlane &clip_plane, const PolyDouble &poly, const Matrix4 &obj_to_world) const
{
	return base_primitive_->clipToBound(logger, bound, clip_plane, poly, obj_to_world * base_instance_.getObjToWorldMatrix(0));
}

inline std::pair<float, Uv<float>> PrimitiveInstance::intersect(const Point3 &from, const Vec3 &dir, float time) const
{
	return base_primitive_->intersect(from, dir, time, base_instance_.getObjToWorldMatrixAtTime(time));
}

inline std::pair<float, Uv<float>> PrimitiveInstance::intersect(const Point3 &from, const Vec3 &dir, float time, const Matrix4 &obj_to_world) const
{
	return base_primitive_->intersect(from, dir, time, obj_to_world * base_instance_.getObjToWorldMatrixAtTime(time));
}

inline float PrimitiveInstance::surfaceArea(float time) const
{
	return base_primitive_->surfaceArea(time, base_instance_.getObjToWorldMatrixAtTime(time));
}

inline float PrimitiveInstance::surfaceArea(float time, const Matrix4 &obj_to_world) const
{
	return base_primitive_->surfaceArea(time, obj_to_world * base_instance_.getObjToWorldMatrixAtTime(time));
}

inline Vec3 PrimitiveInstance::getGeometricNormal(const Uv<float> &uv, float time, bool) const
{
	return base_primitive_->getGeometricNormal(uv, time, base_instance_.getObjToWorldMatrixAtTime(time));
}

inline Vec3 PrimitiveInstance::getGeometricNormal(const Uv<float> &uv, float time, const Matrix4 &obj_to_world) const
{
	return base_primitive_->getGeometricNormal(uv, time, obj_to_world * base_instance_.getObjToWorldMatrixAtTime(time));
}

inline std::pair<Point3, Vec3> PrimitiveInstance::sample(const Uv<float> &uv, float time) const
{
	return base_primitive_->sample(uv, time, base_instance_.getObjToWorldMatrixAtTime(time));
}

inline std::pair<Point3, Vec3> PrimitiveInstance::sample(const Uv<float> &uv, float time, const Matrix4 &obj_to_world) const
{
	return base_primitive_->sample(uv, time, obj_to_world * base_instance_.getObjToWorldMatrixAtTime(time));
}

inline Bound PrimitiveInstance::getBound() const
{
	const std::vector<const Matrix4 *> matrices = base_instance_.getObjToWorldMatrices();
	Bound result{base_primitive_->getBound(*matrices[0])};
	for(size_t i = 1; i < matrices.size(); ++i)
	{
		result.include(base_primitive_->getBound(*matrices[i]));
	}
	return result;
}

inline Bound PrimitiveInstance::getBound(const Matrix4 &obj_to_world) const
{
	const std::vector<const Matrix4 *> matrices = base_instance_.getObjToWorldMatrices();
	Bound result{base_primitive_->getBound(obj_to_world * *(matrices[0]))};
	for(size_t i = 1; i < matrices.size(); ++i)
	{
		result.include(base_primitive_->getBound(obj_to_world * *(matrices[i])));
	}
	return result;
}

} //namespace yafaray

#endif //YAFARAY_PRIMITIVE_INSTANCE_H
