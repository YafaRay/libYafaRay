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
#include <common/logger.h>

BEGIN_YAFARAY

class Vec3;
class Background;
class Camera;
class Light;
class PathData;
class PathVertex;
class ImageFilm;
class Pdf1D;

class BidirectionalIntegrator final : public TiledIntegrator
{
	public:
		static std::unique_ptr<Integrator> factory(Logger &logger, ParamMap &params, const Scene &scene);

	private:
		BidirectionalIntegrator(Logger &logger, bool transp_shad = false, int shadow_depth = 4);
		virtual std::string getShortName() const override { return "BdPT"; }
		virtual std::string getName() const override { return "BidirectionalPathTracer"; }
		virtual bool preprocess(const RenderControl &render_control, const RenderView *render_view, ImageFilm *image_film) override;
		virtual void cleanup() override;
		virtual Rgba integrate(RenderData &render_data, const DiffRay &ray, int additional_depth, ColorLayers *color_layers, const RenderView *render_view) const override;
		Rgb sampleAmbientOcclusionLayer(RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo) const;
		Rgb sampleAmbientOcclusionClayLayer(RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo) const;
		int createPath(RenderData &render_data, const Ray &start, std::vector<PathVertex> &path, int max_len) const;
		Rgb evalPath(RenderData &render_data, int s, int t, PathData &pd) const;
		Rgb evalLPath(RenderData &render_data, int t, PathData &pd, Ray &l_ray, const Rgb &lcol) const;
		Rgb evalPathE(RenderData &render_data, int s, PathData &pd) const;
		bool connectPaths(RenderData &render_data, int s, int t, PathData &pd) const;
		bool connectLPath(RenderData &render_data, int t, PathData &pd, Ray &l_ray, Rgb &lcol) const;
		bool connectPathE(const RenderView *render_view, RenderData &render_data, int s, PathData &pd) const;
		float pathWeight(RenderData &render_data, int s, int t, PathData &pd) const;
		float pathWeight0T(RenderData &render_data, int t, PathData &pd) const;

		bool tr_shad_;        //!< calculate transparent shadows for transparent objects
		int s_depth_;
		//mutable std::vector<pathVertex_t> lightPath, eyePath;
		//mutable int nPaths;
		//mutable pathData_t pathData;
		mutable std::vector<PathData> thread_data_;
		std::vector<const Light *> lights_; //! An array containing all the scene lights
		std::unique_ptr<Pdf1D> light_power_d_;
		float f_num_lights_;
		std::map <const Light *, float> inv_light_power_d_;

		bool use_ambient_occlusion_; //! Use ambient occlusion
		int ao_samples_; //! Ambient occlusion samples
		float ao_dist_; //! Ambient occlusion distance
		Rgb ao_col_; //! Ambient occlusion color
		bool transp_background_; //! Render background as transparent
		bool transp_refracted_background_; //! Render refractions of background as transparent
};

END_YAFARAY

#endif // YAFARAY_INTEGRATOR_BIDIRECTIONAL_H
