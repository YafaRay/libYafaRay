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

#ifndef YAFARAY_INTEGRATOR_BIDIRECTIONAL_H
#define YAFARAY_INTEGRATOR_BIDIRECTIONAL_H


#include "integrator_tiled.h"
#include "color/color.h"
#include <map>
#include <atomic>

BEGIN_YAFARAY

class Vec3;
class Light;
class Pdf1D;

class BidirectionalIntegrator final : public TiledIntegrator
{
	public:
		static Integrator *factory(Logger &logger, ParamMap &params, const Scene &scene, RenderControl &render_control);
		static constexpr int max_path_length_ = 32;
		static constexpr int max_path_eval_length_ = 2 * max_path_length_ + 1;
		static constexpr int min_path_length_ = 3;

	private:
		struct PathData;
		struct PathVertex;
		struct PathEvalVertex;
		BidirectionalIntegrator(RenderControl &render_control, Logger &logger, bool transp_shad = false, int shadow_depth = 4);
		std::string getShortName() const override { return "BdPT"; }
		std::string getName() const override { return "BidirectionalPathTracer"; }
		bool preprocess(ImageFilm *image_film, const RenderView *render_view, const Scene &scene) override;
		void cleanup() override;
		std::pair<Rgb, float> integrate(Ray &ray, RandomGenerator &random_generator, ColorLayers *color_layers, int thread_id, int ray_level, bool chromatic_enabled, float wavelength, int additional_depth, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data) const override;
		int createPath(RandomGenerator &random_generator, const Accelerator &accelerator, bool chromatic_enabled, float wavelength, const Ray &start, std::vector<PathVertex> &path, int max_len, const Camera *camera) const;
		Rgb evalPath(const Accelerator &accelerator, int s, int t, const PathData &pd, const Camera *camera) const;
		Rgb evalLPath(const Accelerator &accelerator, int t, const PathData &pd, const Ray &l_ray, const Rgb &lcol, const Camera *camera) const;
		Rgb evalPathE(const Accelerator &accelerator, int s, const PathData &pd, const Camera *camera) const;
		bool connectPaths(PathData &pd, int s, int t) const;
		bool connectLPath(PathData &pd, Ray &l_ray, Rgb &lcol, RandomGenerator &random_generator, bool chromatic_enabled, float wavelength, int t) const;
		bool connectPathE(PathData &pd, int s) const;
		float pathWeight(int s, int t, const PathData &pd) const;
		float pathWeight0T(PathData &pd, int t) const;
		static void clearPath(std::vector<PathEvalVertex> &p, int s, int t);
		static void checkPath(std::vector<PathEvalVertex> &p, int s, int t);
		static void copyLightSubpath(PathData &pd, int s, int t);
		static void copyEyeSubpath(PathData &pd, int s, int t);

		//mutable std::vector<pathVertex_t> lightPath, eyePath;
		//mutable int nPaths;
		//mutable pathData_t pathData;
		mutable std::atomic<int> n_paths_ {0};
		std::vector<const Light *> lights_; //! An array containing all the scene lights
		std::unique_ptr<Pdf1D> light_power_d_;
		float f_num_lights_;
		std::map <const Light *, float> inv_light_power_d_;
};

END_YAFARAY

#endif // YAFARAY_INTEGRATOR_BIDIRECTIONAL_H
