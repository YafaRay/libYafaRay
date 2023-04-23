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

#ifndef LIBYAFARAY_INTEGRATOR_TILED_H
#define LIBYAFARAY_INTEGRATOR_TILED_H

#include "integrator/surface/integrator_surface.h"
#include "common/aa_noise_params.h"
#include "color/color.h"
#include <vector>
#include <condition_variable>
#include <accelerator/accelerator.h>

namespace yafaray {

struct RenderArea;

class ThreadControl final
{
	public:
		std::mutex m_;
		std::condition_variable c_; //!< condition variable to signal main thread
		std::vector<RenderArea> areas_; //!< area to be output to e.g. blender, if any
		int finished_threads_ = 0; //!< number of finished threads, lock countCV when increasing/reading!
};

class TiledIntegrator : public SurfaceIntegrator
{
	public:
		inline static std::string getClassName() { return "TiledIntegrator"; }
		TiledIntegrator(Logger &logger, ParamResult &param_result, const std::string &name, const ParamMap &param_map) : SurfaceIntegrator{logger, param_result, name, param_map} { }
		/*! Rendering prepasses to precalc suff in case needed */
		virtual void prePass(RenderControl &render_control, ImageFilm &image_film, int samples, int offset, bool adaptive) { } //!< Called before the proper rendering of all the tiles starts
		/*! do whatever is required to render the image; default implementation renders image in passes
		dividing each pass into tiles for multithreading. */
		bool render(RenderControl &render_control, ImageFilm &image_film, unsigned int object_index_highest, unsigned int material_index_highest) override;
		/*! render a pass; only required by the default implementation of render() */
		bool renderPass(RenderControl &render_control, ImageFilm &image_film, std::vector<int> &correlative_sample_number, int samples, int offset, bool adaptive, int aa_pass_number, unsigned int object_index_highest, unsigned int material_index_highest, float aa_light_sample_multiplier, float aa_indirect_sample_multiplier);
		virtual bool renderTile(ImageFilm &image_film, std::vector<int> &correlative_sample_number, const RenderArea &a, int n_samples, int offset, bool adaptive, int thread_id, int aa_pass_number, unsigned int object_index_highest, unsigned int material_index_highest, float aa_light_sample_multiplier, float aa_indirect_sample_multiplier, const RenderControl &render_control);
		void renderWorker(ImageFilm &image_film, ThreadControl *control, std::vector<int> &correlative_sample_number, int thread_id, int samples, int offset, bool adaptive, int aa_pass, unsigned int object_index_highest, unsigned int material_index_highest, float aa_light_sample_multiplier, float aa_indirect_sample_multiplier, const RenderControl &render_control);
		void precalcDepths(ImageFilm &image_film) const;
		static void generateCommonLayers(ColorLayers *color_layers, const SurfacePoint &sp, const MaskParams &mask_params, unsigned int object_index_highest, unsigned int material_index_highest); //!< Generates render passes common to all integrators
		static void generateOcclusionLayers(ColorLayers *color_layers, const Accelerator &accelerator, bool chromatic_enabled, float wavelength, const RayDivision &ray_division, const Camera *camera, const PixelSamplingData &pixel_sampling_data, const SurfacePoint &sp, const Vec3f &wo, int ao_samples, bool shadow_bias_auto, float shadow_bias, float ao_dist, const Rgb &ao_col, int transp_shadows_depth);
		/*! Samples ambient occlusion for a given surface point */
		static Rgb sampleAmbientOcclusion(const Accelerator &accelerator, bool chromatic_enabled, float wavelength, const SurfacePoint &sp, const Vec3f &wo, const RayDivision &ray_division, const Camera *camera, const PixelSamplingData &pixel_sampling_data, bool transparent_shadows, bool clay, int ao_samples, bool shadow_bias_auto, float shadow_bias, float ao_dist, const Rgb &ao_col, int transp_shadows_depth);
		static void applyVolumetricEffects(Rgb &col, float &alpha, ColorLayers *color_layers, const Ray &ray, RandomGenerator &random_generator, const VolumeIntegrator &volume_integrator, bool transparent_background);
		static std::pair<Rgb, float> background(const Ray &ray, ColorLayers *color_layers, bool transparent_background, bool transparent_refracted_background, const Background *background, int ray_level);
};

} //namespace yafaray

#endif // LIBYAFARAY_INTEGRATOR_TILED_H
