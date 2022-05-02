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

#ifndef YAFARAY_PRIMITIVE_SPHERE_H
#define YAFARAY_PRIMITIVE_SPHERE_H

#include "primitive.h"
#include "geometry/vector.h"
#include "geometry/object/object.h"
#include "camera/camera.h"

BEGIN_YAFARAY

class Scene;
class ParamMap;
class Bound;
class ExBound;
class Ray;
class SurfacePoint;

class SpherePrimitive final : public Primitive
{
	public:
		static Primitive *factory(const ParamMap &params, const Scene &scene, const Object &object);
		SpherePrimitive(const Point3 &centr, float rad, const std::unique_ptr<const Material> *material, const Object &base_object): center_(centr), radius_(rad), base_object_(base_object), material_(material) {}

	private:
		Bound getBound() const override { return getBound(nullptr); }
		Bound getBound(const Matrix4 *) const override;
		bool intersectsBound(const ExBound &b, const Matrix4 *obj_to_world) const override { return true; };
		IntersectData intersect(const Ray &ray) const override { return intersect(ray, nullptr); }
		IntersectData intersect(const Ray &ray, const Matrix4 *obj_to_world) const override;
		std::unique_ptr<const SurfacePoint> getSurface(const RayDifferentials *ray_differentials, const Point3 &hit, const IntersectData &intersect_data, const Matrix4 *obj_to_world, const Camera *camera) const override;
		const Material *getMaterial() const override { return material_->get(); }
		float surfaceArea(const Matrix4 *obj_to_world) const override;
		Vec3 getGeometricFaceNormal(const Matrix4 *obj_to_world, float u, float v) const override;
		std::pair<Point3, Vec3> sample(float s_1, float s_2, const Matrix4 *obj_to_world) const override;
		const Object *getObject() const override { return &base_object_; }
		Visibility getVisibility() const override { return base_object_.getVisibility(); }

		Point3 center_;
		float radius_;
		const Object &base_object_;
		const std::unique_ptr<const Material> *material_ = nullptr;
};

END_YAFARAY

#endif //YAFARAY_PRIMITIVE_SPHERE_H
