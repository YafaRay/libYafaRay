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

#include <camera/camera.h>
#include "primitive.h"
#include "geometry/vector.h"
#include "geometry/object/object.h"

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
		static Primitive *factory(ParamMap &params, const Scene &scene, const Object &object);
		SpherePrimitive(const Point3 &centr, float rad, const Material *m, const Object &base_object): center_(centr), radius_(rad), base_object_(base_object), material_(m) {}

	private:
		virtual Bound getBound(const Matrix4 *obj_to_world) const override;
		virtual bool intersectsBound(const ExBound &b, const Matrix4 *obj_to_world) const override { return true; };
		virtual IntersectData intersect(const Ray &ray, const Matrix4 *obj_to_world) const override;
		virtual SurfacePoint getSurface(const RayDifferentials *ray_differentials, const Point3 &hit, const IntersectData &intersect_data, const Matrix4 *obj_to_world, const Camera *camera) const override;
		virtual const Material *getMaterial() const override { return material_; }
		virtual float surfaceArea(const Matrix4 *obj_to_world) const override;
		virtual Vec3 getGeometricNormal(const Matrix4 *obj_to_world, float u, float v) const override;
		virtual void sample(float s_1, float s_2, Point3 &p, Vec3 &n, const Matrix4 *obj_to_world) const override;
		virtual const Object *getObject() const override { return &base_object_; }
		virtual Visibility getVisibility() const override { return base_object_.getVisibility(); }

		Point3 center_;
		float radius_;
		const Object &base_object_;
		const Material *material_ = nullptr;
};

END_YAFARAY

#endif //YAFARAY_PRIMITIVE_SPHERE_H
