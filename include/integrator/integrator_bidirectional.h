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


#include "integrator/integrator_tiled.h"
#include "common/color.h"
#include <map>

BEGIN_YAFARAY

class Vec3;
class Background;
class Camera;
class Light;
class PathData;
class PathVertex;

class BidirectionalIntegrator final : public TiledIntegrator
{
	public:
		static Integrator *factory(ParamMap &params, RenderEnvironment &render);

	private:
		BidirectionalIntegrator(bool transp_shad = false, int shadow_depth = 4);
		virtual ~BidirectionalIntegrator() override;
		virtual bool preprocess() override;
		virtual void cleanup() override;
		virtual Rgba integrate(RenderState &state, DiffRay &ray, ColorPasses &color_passes, int additional_depth = 0) const override;
		Rgb sampleAmbientOcclusionPass(RenderState &state, const SurfacePoint &sp, const Vec3 &wo) const;
		Rgb sampleAmbientOcclusionPassClay(RenderState &state, const SurfacePoint &sp, const Vec3 &wo) const;
		int createPath(RenderState &state, Ray &start, std::vector<PathVertex> &path, int max_len) const;
		Rgb evalPath(RenderState &state, int s, int t, PathData &pd) const;
		Rgb evalLPath(RenderState &state, int t, PathData &pd, Ray &l_ray, const Rgb &lcol) const;
		Rgb evalPathE(RenderState &state, int s, PathData &pd) const;
		bool connectPaths(RenderState &state, int s, int t, PathData &pd) const;
		bool connectLPath(RenderState &state, int t, PathData &pd, Ray &l_ray, Rgb &lcol) const;
		bool connectPathE(RenderState &state, int s, PathData &pd) const;
		//Rgb estimateOneDirect(renderState_t &state, const surfacePoint_t &sp, vector3d_t wo, pathCon_t &pc) const;
		float pathWeight(RenderState &state, int s, int t, PathData &pd) const;
		float pathWeight0T(RenderState &state, int t, PathData &pd) const;

		Background *background_ = nullptr;
		const Camera *cam_ = nullptr;
		bool tr_shad_;        //!< calculate transparent shadows for transparent objects
		bool use_bg_;        //!< configuration; include background for GI
		bool ibl_;           //!< configuration; use background light, if available
		bool include_bg_;    //!< determined on precrocess;
		int s_depth_, bounces_;
		std::vector<Light *> lights_;
		//mutable std::vector<pathVertex_t> lightPath, eyePath;
		//mutable int nPaths;
		//mutable pathData_t pathData;
		mutable std::vector<PathData> thread_data_;
		Pdf1D *light_power_d_ = nullptr;
		float f_num_lights_;
		std::map <const Light *, float> inv_light_power_d_;
		ImageFilm *light_image_ = nullptr;

		bool use_ambient_occlusion_; //! Use ambient occlusion
		int ao_samples_; //! Ambient occlusion samples
		float ao_dist_; //! Ambient occlusion distance
		Rgb ao_col_; //! Ambient occlusion color
		bool transp_background_; //! Render background as transparent
		bool transp_refracted_background_; //! Render refractions of background as transparent
};

END_YAFARAY

#endif // YAFARAY_INTEGRATOR_BIDIRECTIONAL_H
