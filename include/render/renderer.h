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
#include "render/render_callbacks.h"
#include "render/render_control.h"
#include "render/render_view.h"
#include "common/aa_noise_params.h"
#include "common/mask_edge_toon_params.h"
#include "common/layers.h"
#include "common/items.h"

namespace yafaray {

class Scene;
class ImageFilm;
class Format;
class SurfaceIntegrator;
class VolumeIntegrator;
class SurfacePoint;
class RenderView;
class Camera;
enum class DarkDetectionType : unsigned char;

class Renderer final
{
	public:
		inline static std::string getClassName() { return "Renderer"; }
		Renderer(Logger &logger, const std::string &name, const Scene &scene, const ParamMap &param_map, ::yafaray_DisplayConsole display_console);
		~Renderer();
		void setNumThreads(int threads);
		void setNumThreadsPhotons(int threads_photons);
		std::string name() const { return name_; }
		bool render(ImageFilm &image_film, std::unique_ptr<ProgressBar> progress_bar, const Scene &scene);
		int getNumThreads() const { return nthreads_; }
		int getNumThreadsPhotons() const { return nthreads_photons_; }
		RenderControl &getRenderControl() { return render_control_; }
		const Items<RenderView> &getRenderViews() const { return render_views_; }
		std::pair<size_t, ParamResult> createRenderView(const std::string &name, const ParamMap &param_map);
		bool setupSceneRenderParams(const ParamMap &param_map);
		float getShadowBias() const { return shadow_bias_; }
		bool isShadowBiasAuto() const { return shadow_bias_auto_; }
		float getRayMinDist() const { return ray_min_dist_; }
		bool isRayMinDistAuto() const { return ray_min_dist_auto_; }
		ParamResult defineSurfaceIntegrator(const ParamMap &param_map);
		ParamResult defineVolumeIntegrator(const Scene &scene, const ParamMap &param_map);
		const VolumeIntegrator *getVolIntegrator() const { return vol_integrator_.get(); }
		std::pair<size_t, ParamResult> createCamera(const std::string &name, const ParamMap &param_map);
		std::tuple<Camera *, size_t, ResultFlags> getCamera(const std::string &name) const;
		const Items<Camera> &getCameras() const { return cameras_; }

	private:
		std::string name_{"Renderer"};
		int nthreads_ = 1;
		int nthreads_photons_ = 1;
		float shadow_bias_ = 1.0e-4f;  //shadow bias to apply to shadows to avoid self-shadow artifacts
		bool shadow_bias_auto_ = true; //enable automatic shadow bias calculation
		float ray_min_dist_ = 1.0e-5f;  //ray minimum distance
		bool ray_min_dist_auto_ = true;  //enable automatic ray minimum distance calculation
		RenderControl render_control_;
		std::unique_ptr<SurfaceIntegrator> surf_integrator_;
		std::unique_ptr<VolumeIntegrator> vol_integrator_;
		Items<RenderView> render_views_;
		Items<Camera> cameras_;
		Logger &logger_;
};

} //namespace yafaray

#endif //LIBYAFARAY_RENDERER_H
