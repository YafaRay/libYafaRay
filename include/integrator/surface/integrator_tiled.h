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

#ifndef YAFARAY_INTEGRATOR_TILED_H
#define YAFARAY_INTEGRATOR_TILED_H

#include "integrator/integrator.h"
#include "common/aa_noise_params.h"
#include "color/color.h"
#include <vector>
#include <condition_variable>
#include <accelerator/accelerator.h>

BEGIN_YAFARAY

class SurfacePoint;
class PhotonMap;
class Background;
struct RenderArea;
struct MaskParams;
class Vec3;
enum class DarkDetectionType : unsigned char;

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
		TiledIntegrator(RenderControl &render_control, Logger &logger) : SurfaceIntegrator(render_control, logger) { }
		/*! Rendering prepasses to precalc suff in case needed */
		virtual void prePass(FastRandom &fast_random, int samples, int offset, bool adaptive) { } //!< Called before the proper rendering of all the tiles starts
		/*! do whatever is required to render the image; default implementation renders image in passes
		dividing each pass into tiles for multithreading. */
		bool render(FastRandom &fast_random, unsigned int object_index_highest, unsigned int material_index_highest) override;
		/*! render a pass; only required by the default implementation of render() */
		virtual bool renderPass(FastRandom &fast_random, std::vector<int> &correlative_sample_number, int samples, int offset, bool adaptive, int aa_pass_number, unsigned int object_index_highest, unsigned int material_index_highest);
		/*! render a tile; only required by default implementation of render() */
		virtual bool renderTile(FastRandom &fast_random, std::vector<int> &correlative_sample_number, const RenderArea &a, int n_samples, int offset, bool adaptive, int thread_id, int aa_pass_number, unsigned int object_index_highest, unsigned int material_index_highest);
		bool renderTile(FastRandom &fast_random, std::vector<int> &correlative_sample_number, const RenderArea &a, int n_samples, int offset, bool adaptive, int thread_id, unsigned int object_index_highest, unsigned int material_index_highest) { return renderTile(fast_random, correlative_sample_number, a, n_samples, offset, adaptive, thread_id, 0, object_index_highest, material_index_highest); }
		virtual void renderWorker(ThreadControl *control, FastRandom &fast_random, std::vector<int> &correlative_sample_number, int thread_id, int samples, int offset, bool adaptive, int aa_pass, unsigned int object_index_highest, unsigned int material_index_highest);
		virtual void precalcDepths();
		static void generateCommonLayers(ColorLayers *color_layers, const SurfacePoint &sp, const MaskParams &mask_params, unsigned int object_index_highest, unsigned int material_index_highest); //!< Generates render passes common to all integrators
		static void generateOcclusionLayers(ColorLayers *color_layers, const Accelerator &accelerator, bool chromatic_enabled, float wavelength, const RayDivision &ray_division, const Camera *camera, const PixelSamplingData &pixel_sampling_data, const SurfacePoint &sp, const Vec3 &wo, int ao_samples, bool shadow_bias_auto, float shadow_bias, float ao_dist, const Rgb &ao_col, int transp_shadows_depth);
		/*! Samples ambient occlusion for a given surface point */
		static Rgb sampleAmbientOcclusion(const Accelerator &accelerator, bool chromatic_enabled, float wavelength, const SurfacePoint &sp, const Vec3 &wo, const RayDivision &ray_division, const Camera *camera, const PixelSamplingData &pixel_sampling_data, bool transparent_shadows, bool clay, int ao_samples, bool shadow_bias_auto, float shadow_bias, float ao_dist, const Rgb &ao_col, int transp_shadows_depth);
		static void applyVolumetricEffects(Rgb &col, float &alpha, ColorLayers *color_layers, const Ray &ray, RandomGenerator &random_generator, const VolumeIntegrator *volume_integrator, bool transparent_background);
		static std::pair<Rgb, float> background(const Ray &ray, ColorLayers *color_layers, bool transparent_background, bool transparent_refracted_background, const Background *background, int ray_level);

	protected:
		float i_aa_passes_; //!< Inverse of AA_passes used for depth map
		float aa_sample_multiplier_ = 1.f;
		float aa_light_sample_multiplier_ = 1.f;
		float aa_indirect_sample_multiplier_ = 1.f;
		float max_depth_; //!< Inverse of max depth from camera within the scene boundaries
		float min_depth_; //!< Distance between camera and the closest object on the scene
		bool use_ambient_occlusion_; //! Use ambient occlusion
		int ao_samples_; //! Ambient occlusion samples
		float ao_dist_; //! Ambient occlusion distance
		Rgb ao_col_; //! Ambient occlusion color
		int s_depth_; //! Shadow depth for transparent shadows
		bool tr_shad_; //! Use transparent shadows
		bool transp_background_; //! Render background as transparent
		bool transp_refracted_background_; //! Render refractions of background as transparent
};

END_YAFARAY

#endif // YAFARAY_INTEGRATOR_TILED_H
