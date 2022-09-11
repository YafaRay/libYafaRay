#pragma once
/****************************************************************************
 *
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
 *
 */

#ifndef YAFARAY_RENDER_VIEW_H
#define YAFARAY_RENDER_VIEW_H

#include "common/collection.h"
#include "common/enum.h"
#include "common/enum_map.h"
#include "param/class_meta.h"
#include <string>
#include <vector>
#include <memory>
#include "common/logger.h"

namespace yafaray {

class Camera;
class Light;
class ParamMap;
class Scene;

class RenderView final
{
	public:
		inline static std::string getClassName() { return "RenderView"; }
		static std::pair<RenderView *, ParamError> factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map);
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return Params::meta_.print(excluded_params); }
		bool init(Logger &logger, const Scene &scene);
		[[nodiscard]] std::string getName() const { return name_; }
		const Camera *getCamera() const { return camera_; }
		std::map<std::string, Light *> getLights() const { return lights_; }
		bool isSpectral() const { return params_.wavelength_ != 0.f; }
		float getWaveLength() const { return params_.wavelength_; }
		std::vector<const Light *> getLightsVisible() const;
		std::vector<const Light *> getLightsEmittingCausticPhotons() const;
		std::vector<const Light *> getLightsEmittingDiffusePhotons() const;

	private:
		const struct Params
		{
			PARAM_INIT;
			PARAM_DECL(std::string, camera_name_, "", "camera_name", "Name of the camera used for this render view");
			PARAM_DECL(std::string, light_names_, "", "light_names", "Name of the lights, separated by a semicolon, used for this render view. If not specified, all lights will be included");
			PARAM_DECL(float, wavelength_, 0.f, "wavelength", "Wavelength in nm used for this render view (NOT IMPLEMENTED YET). If set to 0.f regular color rendering will take place");
		} params_;
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const;
		RenderView(Logger &logger, ParamError &param_error, const ParamMap &param_map);
		std::string name_;
		const Camera *camera_ = nullptr;
		std::map<std::string, Light *> lights_;
};

} //namespace yafaray

#endif //YAFARAY_RENDER_VIEW_H
