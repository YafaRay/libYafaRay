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

namespace yafaray {

class Scene;
class ParamMap;
template<typename T> class Bound;
class SurfacePoint;

class SpherePrimitive final : public Primitive
{
	public:
		inline static std::string getClassName() { return "CurveObject"; }
		static std::pair<SpherePrimitive *, ParamError> factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map, const Object &object);
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return Params::meta_.print(excluded_params); }

	private:
		struct Params
		{
			PARAM_INIT;
			PARAM_DECL(Vec3f , center_, Vec3f{0.f}, "center", "");
			PARAM_DECL(float , radius_, 1.f, "radius", "");
			PARAM_DECL(std::string , material_name_, "", "material", "");
		} params_;
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const;
		SpherePrimitive(Logger &logger, ParamError &param_error, Params params, const std::unique_ptr<const Material> *material, const Object &base_object);
		Bound<float> getBound() const override;
		Bound<float> getBound(const Matrix4f &obj_to_world) const override;
		std::pair<float, Uv<float>> intersect(const Point3f &from, const Vec3f &dir, float time) const override;
		std::pair<float, Uv<float>> intersect(const Point3f &from, const Vec3f &dir, float time, const Matrix4f &obj_to_world) const override;
		std::unique_ptr<const SurfacePoint> getSurface(const RayDifferentials *ray_differentials, const Point3f &hit_point, float time, const Uv<float> &intersect_uv, const Camera *camera) const override;
		std::unique_ptr<const SurfacePoint> getSurface(const RayDifferentials *ray_differentials, const Point3f &hit_point, float time, const Uv<float> &intersect_uv, const Camera *camera, const Matrix4f &obj_to_world) const override;
		const Material *getMaterial() const override { return material_->get(); }
		float surfaceArea(float time) const override;
		float surfaceArea(float time, const Matrix4f &obj_to_world) const override;
		Vec3f getGeometricNormal(const Uv<float> &uv, float time, bool) const override;
		Vec3f getGeometricNormal(const Uv<float> &uv, float time, const Matrix4f &obj_to_world) const override;
		std::pair<Point3f, Vec3f> sample(const Uv<float> &uv, float time) const override;
		std::pair<Point3f, Vec3f> sample(const Uv<float> &uv, float time, const Matrix4f &obj_to_world) const override;
		const Object *getObject() const override { return &base_object_; }
		Visibility getVisibility() const override { return base_object_.getVisibility(); }
		bool clippingSupport() const override { return false; }
		float getDistToNearestEdge(const Uv<float> &uv, const Uv<Vec3f> &dp_abs) const override { return 0.f; }
		unsigned int getObjectIndex() const override { return base_object_.getIndex(); }
		unsigned int getObjectIndexAuto() const override { return base_object_.getIndexAuto(); }
		Rgb getObjectIndexAutoColor() const override { return base_object_.getIndexAutoColor(); }
		const Light *getObjectLight() const override { return base_object_.getLight(); }
		bool hasObjectMotionBlur() const override { return base_object_.hasMotionBlur(); }

		const Object &base_object_;
		const std::unique_ptr<const Material> *material_ = nullptr;
};

} //namespace yafaray

#endif //YAFARAY_PRIMITIVE_SPHERE_H
