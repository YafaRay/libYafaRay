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

#include <array>
#include "primitive.h"
#include "geometry/object/object_instance.h"

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

class PrimitiveInstance : public Primitive
{
	public:
		//static PrimitiveInstance *factory(ParamMap &params, const Scene &scene);
		PrimitiveInstance(const Primitive *base_primitive, const ObjectInstance &base_instance) : base_instance_(base_instance), base_primitive_(base_primitive) { }
		Bound getBound() const override { return getBound(nullptr); }
		Bound getBound(const Matrix4 *) const override { return base_primitive_->getBound(base_instance_.getObjToWorldMatrix()); }
		bool intersectsBound(const ExBound &b, const Matrix4 *) const override;
		bool clippingSupport() const override { return base_primitive_->clippingSupport(); }
		PolyDouble::ClipResultWithBound clipToBound(Logger &logger, const std::array<Vec3Double, 2> &bound, const ClipPlane &clip_plane, const PolyDouble &poly, const Matrix4 *obj_to_world) const override;
		IntersectData intersect(const Ray &ray) const override { return intersect(ray, nullptr); }
		IntersectData intersect(const Ray &ray, const Matrix4 *) const override;
		std::unique_ptr<const SurfacePoint> getSurface(const RayDifferentials *ray_differentials, const Point3 &hit_point, const IntersectData &intersect_data, const Matrix4 *, const Camera *camera) const override;
		const Material *getMaterial() const override { return base_primitive_->getMaterial(); }
		float surfaceArea(const Matrix4 *) const override;
		Vec3 getGeometricFaceNormal(const Matrix4 *, float u, float v) const override;
		std::pair<Point3, Vec3> sample(float s_1, float s_2, const Matrix4 *) const override;
		const Object *getObject() const override { return &base_instance_; }
		Visibility getVisibility() const override { return base_primitive_->getVisibility(); }

	private:
		const ObjectInstance &base_instance_;
		const Primitive *base_primitive_ = nullptr;
};

inline bool PrimitiveInstance::intersectsBound(const ExBound &b, const Matrix4 *) const
{
	return base_primitive_->intersectsBound(b, base_instance_.getObjToWorldMatrix());
}

inline PolyDouble::ClipResultWithBound PrimitiveInstance::clipToBound(Logger &logger, const std::array<Vec3Double, 2> &bound, const ClipPlane &clip_plane, const PolyDouble &poly, const Matrix4 *obj_to_world) const
{
	return base_primitive_->clipToBound(logger, bound, clip_plane, poly, base_instance_.getObjToWorldMatrix());
}

inline IntersectData PrimitiveInstance::intersect(const Ray &ray, const Matrix4 *) const
{
	return base_primitive_->intersect(ray, base_instance_.getObjToWorldMatrix());
}

inline float PrimitiveInstance::surfaceArea(const Matrix4 *) const
{
	return base_primitive_->surfaceArea(base_instance_.getObjToWorldMatrix());
}

inline Vec3 PrimitiveInstance::getGeometricFaceNormal(const Matrix4 *, float u, float v) const
{
	return base_primitive_->getGeometricFaceNormal(base_instance_.getObjToWorldMatrix(), u, v);
}

inline std::pair<Point3, Vec3> PrimitiveInstance::sample(float s_1, float s_2, const Matrix4 *) const
{
	return base_primitive_->sample(s_1, s_2, base_instance_.getObjToWorldMatrix());
}

END_YAFARAY

#endif //YAFARAY_PRIMITIVE_INSTANCE_H
