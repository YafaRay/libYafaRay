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

#ifndef LIBYAFARAY_RENDER_VIEW_H
#define LIBYAFARAY_RENDER_VIEW_H

#include "common/collection.h"
#include "common/enum.h"
#include "common/enum_map.h"
#include "param/class_meta.h"
#include "common/logger.h"
#include <string>
#include <vector>
#include <memory>

namespace yafaray {

template <typename T> class SceneItems;

class Camera;
class Light;
class ParamMap;
class Renderer;
class Scene;

class RenderView final
{
	private: struct Type;
	public:
		inline static std::string getClassName() { return "RenderView"; }
		static Type type() ;
		void setId(size_t id) { id_ = id; }
		[[nodiscard]] size_t getId() const { return id_; }
		static std::pair<std::unique_ptr<RenderView>, ParamResult> factory(Logger &logger, const Renderer &renderer, const std::string &name, const ParamMap &param_map);
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return Params::meta_.print(excluded_params); }
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const;
		RenderView(Logger &logger, ParamResult &param_result, const ParamMap &param_map, const SceneItems<Camera> &cameras, size_t camera_id);
		bool init(Logger &logger, const Scene &scene);
		[[nodiscard]] std::string getName() const { return name_; }
		const Camera *getCamera() const;
		std::map<std::string, const Light *> getLights() const { return lights_; }
		bool isSpectral() const { return params_.wavelength_ != 0.f; }
		float getWaveLength() const { return params_.wavelength_; }
		std::vector<const Light *> getLightsVisible() const;
		std::vector<const Light *> getLightsEmittingCausticPhotons() const;
		std::vector<const Light *> getLightsEmittingDiffusePhotons() const;

	private:
		struct Type : public Enum<Type>
		{
			using Enum::Enum;
			enum : ValueType_t { RenderView };
			inline static const EnumMap<ValueType_t> map_{{
					{"RenderView", RenderView, ""},
				}};
		};
		const struct Params
		{
			PARAM_INIT;
			PARAM_DECL(std::string, camera_name_, "", "camera_name", "Name of the camera used for this render view");
			PARAM_DECL(std::string, light_names_, "", "light_names", "Name of the lights, separated by a semicolon, used for this render view. If not specified, all lights will be included");
			PARAM_DECL(float, wavelength_, 0.f, "wavelength", "Wavelength in nm used for this render view (NOT IMPLEMENTED YET). If set to 0.f regular color rendering will take place");
		} params_;
		size_t id_{0};
		std::string name_;
		size_t camera_id_{math::invalid<size_t>};
		const SceneItems<Camera> &cameras_;
		std::map<std::string, const Light *> lights_;
};

} //namespace yafaray

#endif //LIBYAFARAY_RENDER_VIEW_H
