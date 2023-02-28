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

#ifndef LIBYAFARAY_CAMERA_PERSPECTIVE_H
#define LIBYAFARAY_CAMERA_PERSPECTIVE_H

#include "camera/camera.h"
#include <vector>

namespace yafaray {

class ParamMap;
class Scene;

class PerspectiveCamera : public Camera
{
		using ThisClassType_t = PerspectiveCamera; using ParentClassType_t = Camera;

	public:
		inline static std::string getClassName() { return "PerspectiveCamera"; }
		static std::pair<std::unique_ptr<Camera>, ParamResult> factory(Logger &logger, const std::string &name, const ParamMap &param_map);
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return class_meta::print<Params>(excluded_params); }
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;
		PerspectiveCamera(Logger &logger, ParamResult &param_result, const ParamMap &param_map);

	protected:
		struct BokehType : public Enum<BokehType>
		{
			enum : ValueType_t { Disk1, Disk2, Triangle, Square, Pentagon, Hexagon, Ring };
			inline static const EnumMap<ValueType_t> map_{{
					{"disk1", Disk1, ""},
					{"disk2", Disk2, ""},
					{"triangle", Triangle, ""},
					{"square", Square, ""},
					{"pentagon", Pentagon, ""},
					{"hexagon", Hexagon, ""},
					{"ring", Ring, ""},
				}};
		};
		struct BokehBias : public Enum<BokehBias>
		{
			enum : ValueType_t { None, Center, Edge };
			inline static const EnumMap<ValueType_t> map_{{
					{"none", None, ""},
					{"center", Center, ""},
					{"edge", Edge, ""},
				}};
		};
		[[nodiscard]] Type type() const override { return Type::Perspective; }
		const struct Params
		{
			Params(ParamResult &param_result, const ParamMap &param_map);
			static std::map<std::string, const ParamMeta *> getParamMetaMap();
			PARAM_DECL(float, focal_distance_, 1.f, "focal", "");
			PARAM_DECL(float, aperture_, 0.f, "aperture", "");
			PARAM_DECL(float, depth_of_field_distance_, 0.f, "dof_distance", "");
			PARAM_DECL(float, bokeh_rotation_, 0.f, "bokeh_rotation", "");
			PARAM_ENUM_DECL(BokehType, bokeh_type_, BokehType::Disk1, "bokeh_type", "");
			PARAM_ENUM_DECL(BokehBias, bokeh_bias_, BokehBias::Center, "bokeh_bias", "");
		} params_;
		CameraRay shootRay(float px, float py, const Uv<float> &uv) const override;
		bool sampleLens() const override;
		Point3f screenproject(const Point3f &p) const override;
		bool project(const Ray &wo, float lu, float lv, float &u, float &v, float &pdf) const override;
		float biasDist(float r) const;
		Uv<float> sampleTsd(float r_1, float r_2) const;
		Uv<float> getLensUv(float r_1, float r_2) const;
		virtual void setAxis(const Vec3f &vx, const Vec3f &vy, const Vec3f &vz);

		Vec3f dof_up_, dof_rt_;

	private:
		float a_pix_;
		std::vector<float> ls_;
};

} //namespace yafaray

#endif // LIBYAFARAY_CAMERA_PERSPECTIVE_H
