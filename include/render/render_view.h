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
#include <string>
#include <utility>
#include <vector>
#include <memory>
#include <common/logger.h>

namespace yafaray {

class Camera;
class Light;
class ParamMap;
class Scene;

class RenderView final
{
	public:
		static RenderView *factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params);
		bool init(Logger &logger, const Scene &scene);
		std::string getName() const { return name_; }
		const Camera *getCamera() const { return camera_; }
		std::map<std::string, Light *> getLights() const { return lights_; }
		bool isSpectral() const { return wavelength_ != 0.f; }
		float getWaveLength() const { return wavelength_; }
		std::vector<const Light *> getLightsVisible() const;
		std::vector<const Light *> getLightsEmittingCausticPhotons() const;
		std::vector<const Light *> getLightsEmittingDiffusePhotons() const;

	private:
		RenderView(std::string name, std::string camera_name, std::string light_names, float wavelength) : name_(std::move(name)), camera_name_(std::move(camera_name)), light_names_(std::move(light_names)), wavelength_(wavelength) { }
		std::string name_;
		std::string camera_name_;
		std::string light_names_;
		float wavelength_ = 0.f;
		const Camera *camera_ = nullptr;
		std::map<std::string, Light *> lights_;
};

} //namespace yafaray

#endif //YAFARAY_RENDER_VIEW_H
