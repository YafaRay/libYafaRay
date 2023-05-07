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

#ifndef LIBYAFARAY_CAMERA_ORTHOGRAPHIC_H
#define LIBYAFARAY_CAMERA_ORTHOGRAPHIC_H

#include "camera/camera.h"

namespace yafaray {

class ParamMap;
class Scene;

class OrthographicCamera final: public Camera
{
		using ThisClassType_t = OrthographicCamera; using ParentClassType_t = Camera;

	public:
		inline static std::string getClassName() { return "OrthographicCamera"; }
		static std::pair<std::unique_ptr<Camera>, ParamResult> factory(Logger &logger, const std::string &name, const ParamMap &param_map);
		[[nodiscard]] std::map<std::string, const ParamMeta *> getParamMetaMap() const override { return params_.getParamMetaMap(); }
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return class_meta::print<Params>(excluded_params); }
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;
		OrthographicCamera(Logger &logger, ParamResult &param_result, const ParamMap &param_map);

	private:
		[[nodiscard]] Type type() const override { return Type::Orthographic; }
		const struct Params
		{
			Params(ParamResult &param_result, const ParamMap &param_map);
			static std::map<std::string, const ParamMeta *> getParamMetaMap();
			PARAM_DECL(float, scale_, 1.f, "scale", "");
		} params_;
		void setAxis(const Vec3f &vx, const Vec3f &vy, const Vec3f &vz);
		CameraRay shootRay(float px, float py, const Uv<float> &uv) const override;
		Point3f screenproject(const Point3f &p) const override;

		Point3f pos_;
};

} //namespace yafaray

#endif // LIBYAFARAY_CAMERA_ORTHOGRAPHIC_H
