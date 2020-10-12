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

#include "constants.h"
#include "common/collection.h"
#include <string>
#include <vector>

BEGIN_YAFARAY

class Camera;
class Light;
class ParamMap;
class Scene;

class RenderView final
{
	public:
		static RenderView *factory(ParamMap &params, const Scene &scene);
		RenderView(const std::string &name, const Camera *camera, const std::map<std::string, Light *> &lights, float wavelength) : name_(name), camera_(camera), lights_(lights), wavelength_(wavelength) { }
		std::string getName() const { return name_; }
		const Camera *getCamera() const { return camera_; }
		const std::map<std::string, Light *> getLights() const { return lights_; }
		bool isSpectral() const { return wavelength_ != 0.f; }
		float getWaveLength() const { return wavelength_; }
		const std::vector<Light *> getLightsVisible() const;
		const std::vector<Light *> getLightsEmittingCausticPhotons() const;
		const std::vector<Light *> getLightsEmittingDiffusePhotons() const;

	private:
		std::string name_;
		const Camera *camera_ = nullptr;
		const std::map<std::string, Light *> lights_;
		float wavelength_ = 0.f;
};

END_YAFARAY

#endif //YAFARAY_RENDER_VIEW_H
