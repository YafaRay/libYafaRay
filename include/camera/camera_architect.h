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

#ifndef LIBYAFARAY_CAMERA_ARCHITECT_H
#define LIBYAFARAY_CAMERA_ARCHITECT_H

#include "camera/camera_perspective.h"

namespace yafaray {

class ParamMap;
class Scene;

class ArchitectCamera final : public PerspectiveCamera
{
		using ThisClassType_t = ArchitectCamera; using ParentClassType_t = PerspectiveCamera;

	public:
		inline static std::string getClassName() { return "ArchitectCamera"; }
		static std::pair<std::unique_ptr<Camera>, ParamResult> factory(Logger &logger, const std::string &name, const ParamMap &param_map);
		[[nodiscard]] std::map<std::string, const ParamMeta *> getParamMetaMap() const override { return params_.getParamMetaMap(); }
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return class_meta::print<Params>(excluded_params); }
		ArchitectCamera(Logger &logger, ParamResult &param_result, const ParamMap &param_map);

	private:
		[[nodiscard]] Type type() const override { return Type::Architect; }
		void setAxis(const Vec3f &vx, const Vec3f &vy, const Vec3f &vz) override;
		Point3f screenproject(const Point3f &p) const override;
};

} //namespace yafaray

#endif // LIBYAFARAY_CAMERA_ARCHITECT_H
