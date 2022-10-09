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

#ifndef YAFARAY_CAMERA_ANGULAR_H
#define YAFARAY_CAMERA_ANGULAR_H

#include "camera/camera.h"

namespace yafaray {

class ParamMap;
class Scene;

class AngularCamera final : public Camera
{
	public:
		inline static std::string getClassName() { return "AngularCamera"; }
		static std::pair<Camera *, ParamError> factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map);
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return Params::meta_.print(excluded_params); }

	private:
		struct Projection : public Enum<Projection>  //Fish Eye Projections as defined in https://en.wikipedia.org/wiki/Fisheye_lens
		{
			enum : ValueType_t{
				Equidistant, //!<Default and used traditionally in YafaRay
				Orthographic, //!<Orthographic projection where the centre of the image is enlarged/more defined at the cost of much more distorted edges. Angle should be 90º or less
				Stereographic, //!<angle should be less than 180º
				EquisolidAngle,
				Rectilinear, //!<angle should be less than 90º
			};
			inline static const EnumMap<ValueType_t> map_{{
					{"equidistant", Equidistant, "Default and used traditionally in YafaRay"},
					{"orthographic", Orthographic, "Orthographic projection where the centre of the image is enlarged/more defined at the cost of much more distorted edges. Angle should be 90º or less"},
					{"stereographic", Stereographic, "angle should be less than 180º"},
					{"equisolid_angle", EquisolidAngle, ""},
					{"rectilinear", Rectilinear, "angle should be less than 90º"},
				}};
		};
		[[nodiscard]] Type type() const override { return Type::Angular; }
		const struct Params
		{
			PARAM_INIT_PARENT(Camera);
			PARAM_DECL(float, angle_degrees_, 90.f, "angle", "");
			PARAM_DECL(float, max_angle_degrees_, 0.f, "max_angle", "If not specified it uses the value from '" + angle_degrees_meta_.name() + "'");
			PARAM_DECL(bool, circular_, true, "circular", "");
			PARAM_DECL(bool, mirrored_, false, "mirrored", "");
			PARAM_ENUM_DECL(Projection, projection_, Projection::Equidistant, "projection", "");
		} params_;
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;

		AngularCamera(Logger &logger, ParamError &param_error, const ParamMap &param_map);
		void setAxis(const Vec3f &vx, const Vec3f &vy, const Vec3f &vz) override;
		CameraRay shootRay(float px, float py, const Uv<float> &uv) const override;
		Point3f screenproject(const Point3f &p) const override;

		float focal_length_;
		float angle_;
		float max_radius_;
};


} //namespace yafaray

#endif // YAFARAY_CAMERA_ANGULAR_H
