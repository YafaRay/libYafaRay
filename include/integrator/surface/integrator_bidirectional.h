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

BEGIN_YAFARAY

class Vec3;
class Light;
class PathData;
class PathVertex;
class Pdf1D;
struct PathEvalVertex;

class BidirectionalIntegrator final : public TiledIntegrator
{
	public:
		static std::unique_ptr<Integrator> factory(Logger &logger, ParamMap &params, const Scene &scene);

	private:
		BidirectionalIntegrator(Logger &logger, bool transp_shad = false, int shadow_depth = 4);
		virtual std::string getShortName() const override { return "BdPT"; }
		virtual std::string getName() const override { return "BidirectionalPathTracer"; }
		virtual bool preprocess(const RenderControl &render_control, Timer &timer, const RenderView *render_view, ImageFilm *image_film) override;
		virtual void cleanup() override;
		virtual Rgba integrate(int thread_id, int ray_level, bool chromatic_enabled, float wavelength, Ray &ray, int additional_depth, const RayDivision &ray_division, ColorLayers *color_layers, const Camera *camera, RandomGenerator &random_generator, const PixelSamplingData &pixel_sampling_data, bool lights_geometry_material_emit) const override;
		int createPath(bool chromatic_enabled, float wavelength, const Ray &start, std::vector<PathVertex> &path, int max_len, const Camera *camera, RandomGenerator &random_generator) const;
		Rgb evalPath(int s, int t, PathData &pd, const Camera *camera) const;
		Rgb evalLPath(int t, PathData &pd, const Ray &l_ray, const Rgb &lcol, const Camera *camera) const;
		Rgb evalPathE(int s, PathData &pd, const Camera *camera) const;
		bool connectPaths(int s, int t, PathData &pd) const;
		bool connectLPath(bool chromatic_enabled, float wavelength, int t, PathData &pd, Ray &l_ray, Rgb &lcol, RandomGenerator &random_generator, bool lights_geometry_material_emit) const;
		bool connectPathE(const Camera *camera, int s, PathData &pd) const;
		float pathWeight(int s, int t, PathData &pd) const;
		float pathWeight0T(int t, PathData &pd) const;
		static void clearPath(std::vector<PathEvalVertex> &p, int s, int t);
		static void checkPath(std::vector<PathEvalVertex> &p, int s, int t);
		static void copyLightSubpath(PathData &pd, int s, int t);
		static void copyEyeSubpath(PathData &pd, int s, int t);

		//mutable std::vector<pathVertex_t> lightPath, eyePath;
		//mutable int nPaths;
		//mutable pathData_t pathData;
		mutable std::vector<PathData> thread_data_;
		std::vector<const Light *> lights_; //! An array containing all the scene lights
		std::unique_ptr<Pdf1D> light_power_d_;
		float f_num_lights_;
		std::map <const Light *, float> inv_light_power_d_;

		bool transp_background_; //! Render background as transparent
		bool transp_refracted_background_; //! Render refractions of background as transparent

		static constexpr int max_path_length_ = 32;
		static constexpr int min_path_length_ = 3;
};

END_YAFARAY

#endif // YAFARAY_INTEGRATOR_BIDIRECTIONAL_H
