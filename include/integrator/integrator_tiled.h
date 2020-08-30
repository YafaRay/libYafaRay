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

#include "constants.h"
#include "integrator.h"
#include "utility/util_thread.h"
#include <vector>

BEGIN_YAFARAY

struct RenderArea;
class SurfacePoint;
class ImageFilm;
typedef unsigned int Bsdf_t;
class PhotonMap;
class Pdf1D;
enum class DarkDetectionType : int;

class ThreadControl
{
	public:
		ThreadControl() : finished_threads_(0) {}
		std::mutex m_;
		std::condition_variable c_; //!< condition variable to signal main thread
		std::vector<RenderArea> areas_; //!< area to be output to e.g. blender, if any
		volatile int finished_threads_; //!< number of finished threads, lock countCV when increasing/reading!
};

class TiledIntegrator: public SurfaceIntegrator
{
	public:
		/*! Rendering prepasses to precalc suff in case needed */
		virtual void preRender(); //!< Called before the render starts and after the minDepth and maxDepth are calculated
		virtual void prePass(int samples, int offset, bool adaptive); //!< Called before the proper rendering of all the tiles starts
		virtual void preTile(RenderArea &a, int n_samples, int offset, bool adaptive, int thread_id); //!< Called brfore each tile is rendered

		/*! do whatever is required to render the image; default implementation renders image in passes
		dividing each pass into tiles for multithreading. */
		virtual bool render(int num_view, ImageFilm *image_film);
		/*! render a pass; only required by the default implementation of render() */
		virtual bool renderPass(int num_view, int samples, int offset, bool adaptive, int aa_pass_number);
		/*! render a tile; only required by default implementation of render() */
		virtual bool renderTile(int num_view, RenderArea &a, int n_samples, int offset, bool adaptive, int thread_id, int aa_pass_number = 0);
		virtual void renderWorker(int m_num_view, TiledIntegrator *integrator, Scene *scene, ImageFilm *image_film, ThreadControl *control, int thread_id, int samples, int offset = 0, bool adaptive = false, int aa_pass = 0);

		//		virtual void recursiveRaytrace(renderState_t &state, diffRay_t &ray, int rDepth, BSDF_t bsdfs, surfacePoint_t &sp, vector3d_t &wo, Rgb &col, float &alpha) const;
		virtual void precalcDepths();
		virtual void generateCommonRenderPasses(ColorPasses &color_passes, RenderState &state, const SurfacePoint &sp, const DiffRay &ray) const; //!< Generates render passes common to all integrators

	protected:
		int aa_samples_, aa_passes_, aa_inc_samples_;
		float i_aa_passes_; //!< Inverse of AA_passes used for depth map
		float aa_threshold_;
		float aa_resampled_floor_; //!< minimum amount of resampled pixels (% of the total pixels) below which we will automatically decrease the AA_threshold value for the next pass
		float aa_sample_multiplier_factor_;
		float aa_light_sample_multiplier_factor_;
		float aa_indirect_sample_multiplier_factor_;
		bool aa_detect_color_noise_;
		DarkDetectionType aa_dark_detection_type_;
		float aa_dark_threshold_factor_;
		int aa_variance_edge_size_;
		int aa_variance_pixels_;
		float aa_clamp_samples_;
		float aa_clamp_indirect_;
		float aa_sample_multiplier_;
		float aa_light_sample_multiplier_;
		float aa_indirect_sample_multiplier_;
		ImageFilm *image_film_;
		float max_depth_; //!< Inverse of max depth from camera within the scene boundaries
		float min_depth_; //!< Distance between camera and the closest object on the scene
		bool diff_rays_enabled_;	//!< Differential rays enabled/disabled - for future motion blur / interference features
		static std::vector<int> correlative_sample_number_;  //!< Used to sample lights more uniformly when using estimateOneDirectLight
};

END_YAFARAY

#endif // YAFARAY_INTEGRATOR_TILED_H
