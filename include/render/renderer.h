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

#ifndef LIBYAFARAY_RENDERER_H
#define LIBYAFARAY_RENDERER_H

#include "public_api/yafaray_c_api.h"
#include "render/render_control.h"
#include "common/aa_noise_params.h"
#include "common/mask_edge_toon_params.h"
#include "common/layers.h"
#include "common/items.h"

namespace yafaray {

class Scene;
class ImageFilm;
class Format;
class SurfaceIntegrator;
class SurfacePoint;
class Camera;
class Light;
enum class DarkDetectionType : unsigned char;

class Renderer final
{
	public:
		inline static std::string getClassName() { return "Renderer"; }
		Renderer(Logger &logger, const std::string &name, const Scene &scene, const ParamMap &param_map, ::yafaray_DisplayConsole display_console);
		~Renderer();
		std::string getName() const { return name_; }
		bool render(ImageFilm &image_film, std::unique_ptr<ProgressBar> progress_bar, const Scene &scene);

	private:
		std::string name_{"Renderer"};
		float shadow_bias_ = 1.0e-4f;  //shadow bias to apply to shadows to avoid self-shadow artifacts
		bool shadow_bias_auto_ = true; //enable automatic shadow bias calculation
		float ray_min_dist_ = 1.0e-5f;  //ray minimum distance
		bool ray_min_dist_auto_ = true;  //enable automatic ray minimum distance calculation
		std::unique_ptr<SurfaceIntegrator> surf_integrator_;
		Logger &logger_;
};

} //namespace yafaray

#endif //LIBYAFARAY_RENDERER_H
