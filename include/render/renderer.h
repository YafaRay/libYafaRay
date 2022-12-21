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
#include "scene/scene_items.h"

namespace yafaray {

class Scene;
class ImageFilm;
class ImageOutput;
class Format;
class SurfaceIntegrator;
class VolumeIntegrator;
class SurfacePoint;
class ImageOutput;
class RenderView;
class Camera;
enum class DarkDetectionType : unsigned char;

class Renderer final
{
	public:
		inline static std::string getClassName() { return "Renderer"; }
		Renderer(Logger &logger, const std::string &name, const Scene &scene, const ParamMap &param_map, ::yafaray_DisplayConsole display_console);
		~Renderer();
		void setAntialiasing(AaNoiseParams &&aa_noise_params) { aa_noise_params_ = aa_noise_params; };
		void setNumThreads(int threads);
		void setNumThreadsPhotons(int threads_photons);
		std::string name() const { return name_; }
		bool render(std::unique_ptr<ProgressBar> progress_bar, const Scene &scene);
		const ImageFilm *getImageFilm() const { return image_film_.get(); }
		int getNumThreads() const { return nthreads_; }
		int getNumThreadsPhotons() const { return nthreads_photons_; }
		AaNoiseParams getAaParameters() const { return aa_noise_params_; }
		RenderControl &getRenderControl() { return render_control_; }
		std::pair<size_t, ResultFlags> getOutput(const std::string &name) const;
		const SceneItems<RenderView> &getRenderViews() const { return render_views_; }
		std::pair<size_t, ParamResult> createRenderView(const std::string &name, const ParamMap &param_map);
		std::pair<size_t, ParamResult> createOutput(const std::string &name, const ParamMap &param_map);
		bool disableOutput(const std::string &name);
		void clearOutputs();
		const SceneItems<ImageOutput> &getOutputs() const { return outputs_; }
		bool setupSceneRenderParams(const ParamMap &param_map);
		void defineLayer(const ParamMap &param_map);
		void defineLayer(std::string &&layer_type_name, std::string &&image_type_name, std::string &&exported_image_type_name, std::string &&exported_image_name);
		void defineLayer(LayerDef::Type layer_type, Image::Type image_type = Image::Type{Image::Type::None}, Image::Type exported_image_type = Image::Type{Image::Type::None}, const std::string &exported_image_name = "");
		void clearLayers();
		const Layers * getLayers() const { return &layers_; }
		float getShadowBias() const { return shadow_bias_; }
		bool isShadowBiasAuto() const { return shadow_bias_auto_; }
		float getRayMinDist() const { return ray_min_dist_; }
		bool isRayMinDistAuto() const { return ray_min_dist_auto_; }

		MaskParams getMaskParams() const { return mask_params_; }
		EdgeToonParams getEdgeToonParams() const { return edge_toon_params_; }

		void setRenderNotifyViewCallback(yafaray_RenderNotifyViewCallback callback, void *callback_data);
		void setRenderNotifyLayerCallback(yafaray_RenderNotifyLayerCallback callback, void *callback_data);
		void setRenderPutPixelCallback(yafaray_RenderPutPixelCallback callback, void *callback_data);
		void setRenderHighlightPixelCallback(yafaray_RenderHighlightPixelCallback callback, void *callback_data);
		void setRenderFlushAreaCallback(yafaray_RenderFlushAreaCallback callback, void *callback_data);
		void setRenderFlushCallback(yafaray_RenderFlushCallback callback, void *callback_data);
		void setRenderHighlightAreaCallback(yafaray_RenderHighlightAreaCallback callback, void *callback_data);
		const RenderCallbacks &getRenderCallbacks() const { return render_callbacks_; }
		ParamResult defineSurfaceIntegrator(const ParamMap &param_map);
		ParamResult defineVolumeIntegrator(const Scene &scene, const ParamMap &param_map);
		const VolumeIntegrator *getVolIntegrator() const { return vol_integrator_.get(); }
		std::pair<size_t, ParamResult> createCamera(const std::string &name, const ParamMap &param_map);
		std::tuple<Camera *, size_t, ResultFlags> getCamera(const std::string &name) const;
		const SceneItems<Camera> &getCameras() const { return cameras_; }
		SceneItems<Camera> &getCameras() { return cameras_; }

	private:
		void setMaskParams(const ParamMap &params);
		void setEdgeToonParams(const ParamMap &params);
		void defineBasicLayers();
		void defineDependentLayers(); //!< This function generates the basic/auxiliary layers. Must be called *after* defining all render layers with the defineLayer function.
		template <typename T> std::pair<size_t, ParamResult> createRendererItem(Logger &logger, const std::string &name, const ParamMap &param_map, SceneItems<T> &map);

		std::string name_{"Renderer"};
		int nthreads_ = 1;
		int nthreads_photons_ = 1;
		float shadow_bias_ = 1.0e-4f;  //shadow bias to apply to shadows to avoid self-shadow artifacts
		bool shadow_bias_auto_ = true; //enable automatic shadow bias calculation
		float ray_min_dist_ = 1.0e-5f;  //ray minimum distance
		bool ray_min_dist_auto_ = true;  //enable automatic ray minimum distance calculation
		RenderControl render_control_;
		std::unique_ptr<ImageFilm> image_film_;
		std::unique_ptr<SurfaceIntegrator> surf_integrator_;
		std::unique_ptr<VolumeIntegrator> vol_integrator_;
		SceneItems<ImageOutput> outputs_;
		SceneItems<RenderView> render_views_;
		SceneItems<Camera> cameras_;
		Layers layers_;
		AaNoiseParams aa_noise_params_;
		MaskParams mask_params_;
		EdgeToonParams edge_toon_params_;
		RenderCallbacks render_callbacks_;
		Logger &logger_;
};

} //namespace yafaray

#endif //LIBYAFARAY_RENDERER_H
