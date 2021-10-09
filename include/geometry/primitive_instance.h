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
#include "geometry/primitive.h"
#include "geometry/object_instance.h"

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
		virtual Bound getBound(const Matrix4 *) const override;
		virtual bool intersectsBound(const ExBound &b, const Matrix4 *) const override;
		virtual bool clippingSupport() const override { return base_primitive_->clippingSupport(); }
		virtual PolyDouble::ClipResultWithBound clipToBound(Logger &logger, const std::array<Vec3Double, 2> &bound, const ClipPlane &clip_plane, const PolyDouble &poly, const Matrix4 *obj_to_world) const override;
		virtual IntersectData intersect(const Ray &ray, const Matrix4 *) const override;
		virtual SurfacePoint getSurface(const Point3 &hit_point, const IntersectData &intersect_data, const Matrix4 *) const override;
		virtual const Material *getMaterial() const override { return base_primitive_->getMaterial(); }
		virtual float surfaceArea(const Matrix4 *) const override;
		virtual Vec3 getGeometricNormal(const Matrix4 *, float u, float v) const override;
		virtual void sample(float s_1, float s_2, Point3 &p, Vec3 &n, const Matrix4 *) const override;
		virtual const Object *getObject() const override { return &base_instance_; }
		virtual Visibility getVisibility() const override { return base_primitive_->getVisibility(); }

	private:
		const ObjectInstance &base_instance_;
		const Primitive *base_primitive_ = nullptr;
};

END_YAFARAY

#endif //YAFARAY_PRIMITIVE_INSTANCE_H
