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
#include <vector>
#include <condition_variable>

BEGIN_YAFARAY

class SurfacePoint;
class PhotonMap;
struct RenderArea;
struct MaskParams;
enum class DarkDetectionType : int;

class ThreadControl final
{
	public:
		ThreadControl() : finished_threads_(0) {}
		std::mutex m_;
		std::condition_variable c_; //!< condition variable to signal main thread
		std::vector<RenderArea> areas_; //!< area to be output to e.g. blender, if any
		volatile int finished_threads_; //!< number of finished threads, lock countCV when increasing/reading!
};

class TiledIntegrator : public SurfaceIntegrator
{
	public:
		TiledIntegrator(Logger &logger) : SurfaceIntegrator(logger) { }
		/*! Rendering prepasses to precalc suff in case needed */
		virtual void prePass(int samples, int offset, bool adaptive, const RenderControl &render_control, Timer &timer, const RenderView *render_view) { } //!< Called before the proper rendering of all the tiles starts
		/*! do whatever is required to render the image; default implementation renders image in passes
		dividing each pass into tiles for multithreading. */
		virtual bool render(RenderControl &render_control, Timer &timer, const RenderView *render_view) override;
		/*! render a pass; only required by the default implementation of render() */
		virtual bool renderPass(const RenderView *render_view, int samples, int offset, bool adaptive, int aa_pass_number, RenderControl &render_control, Timer &timer);
		/*! render a tile; only required by default implementation of render() */
		virtual bool renderTile(RenderArea &a, const RenderView *render_view, const RenderControl &render_control, const Timer &timer, int n_samples, int offset, bool adaptive, int thread_id, int aa_pass_number = 0);
		virtual void renderWorker(TiledIntegrator *integrator, const Scene *scene, const RenderView *render_view, const RenderControl &render_control, const Timer &timer, ThreadControl *control, int thread_id, int samples, int offset = 0, bool adaptive = false, int aa_pass = 0);

		//		virtual void recursiveRaytrace(renderState_t &state, diffRay_t &ray, int rDepth, BSDF_t bsdfs, surfacePoint_t &sp, vector3d_t &wo, Rgb &col, float &alpha) const;
		virtual void precalcDepths(const RenderView *render_view);
		static void generateCommonLayers(int raylevel, const SurfacePoint &sp, const Ray &ray, const MaskParams &mask_params, ColorLayers *color_layers = nullptr); //!< Generates render passes common to all integrators

	protected:
		float i_aa_passes_; //!< Inverse of AA_passes used for depth map
		AaNoiseParams aa_noise_params_;
		float aa_sample_multiplier_ = 1.f;
		float aa_light_sample_multiplier_ = 1.f;
		float aa_indirect_sample_multiplier_ = 1.f;
		float max_depth_; //!< Inverse of max depth from camera within the scene boundaries
		float min_depth_; //!< Distance between camera and the closest object on the scene
		bool diff_rays_enabled_;	//!< Differential rays enabled/disabled - for future motion blur / interference features
		static std::vector<int> correlative_sample_number_;  //!< Used to sample lights more uniformly when using estimateOneDirectLight
};

END_YAFARAY

#endif // YAFARAY_INTEGRATOR_TILED_H
