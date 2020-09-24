#pragma once
/****************************************************************************
 *      mcintegrator.h: A basic abstract integrator for MC sampling
 *      This is part of the libYafaRay package
 *      Copyright (C) 2010  Rodrigo Placencia (DarkTide)
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

#ifndef YAFARAY_INTEGRATOR_MONTECARLO_H
#define YAFARAY_INTEGRATOR_MONTECARLO_H

#include "integrator_tiled.h"
#include "color/color.h"


BEGIN_YAFARAY

class Background;
class Photon;
class Vec3;
class Light;
enum class BsdfFlags : unsigned int;

enum PhotonMapProcessing
{
	PhotonsGenerateOnly,
	PhotonsGenerateAndSave,
	PhotonsLoad,
	PhotonsReuse
};

class MonteCarloIntegrator: public TiledIntegrator
{
	public:
		MonteCarloIntegrator() = default;

	protected:
		/*! Estimates direct light from all sources in a mc fashion and completing MIS (Multiple Importance Sampling) for a given surface point */
		Rgb estimateAllDirectLight(RenderState &state, const SurfacePoint &sp, const Vec3 &wo, IntPasses *int_passes = nullptr) const;
		/*! Like previous but for only one random light source for a given surface point */
		Rgb estimateOneDirectLight(RenderState &state, const SurfacePoint &sp, Vec3 wo, int n) const;
		/*! Does the actual light estimation on a specific light for the given surface point */
		Rgb doLightEstimation(RenderState &state, Light *light, const SurfacePoint &sp, const Vec3 &wo, const unsigned int &loffs, IntPasses *int_passes = nullptr) const;
		/*! Does recursive mc raytracing with MIS (Multiple Importance Sampling) for a given surface point */
		void recursiveRaytrace(RenderState &state, DiffRay &ray, BsdfFlags bsdfs, SurfacePoint &sp, Vec3 &wo, Rgb &col, float &alpha, int additional_depth, IntPasses *int_passes = nullptr) const;
		/*! Creates and prepares the caustic photon map */
		bool createCausticMap();
		/*! Estimates caustic photons for a given surface point */
		Rgb estimateCausticPhotons(RenderState &state, const SurfacePoint &sp, const Vec3 &wo) const;
		/*! Samples ambient occlusion for a given surface point */
		Rgb sampleAmbientOcclusion(RenderState &state, const SurfacePoint &sp, const Vec3 &wo) const;
		Rgb sampleAmbientOcclusionPass(RenderState &state, const SurfacePoint &sp, const Vec3 &wo) const;
		Rgb sampleAmbientOcclusionPassClay(RenderState &state, const SurfacePoint &sp, const Vec3 &wo) const;

		int r_depth_; //! Ray depth
		bool tr_shad_; //! Use transparent shadows
		int s_depth_; //! Shadow depth for transparent shadows

		bool use_photon_caustics_; //! Use photon caustics
		unsigned int n_caus_photons_; //! Number of caustic photons (to be shoot but it should be the target
		int n_caus_search_; //! Amount of caustic photons to be gathered in estimation
		float caus_radius_; //! Caustic search radius for estimation
		int caus_depth_; //! Caustic photons max path depth
		Pdf1D *light_power_d_;

		bool use_ambient_occlusion_; //! Use ambient occlusion
		int ao_samples_; //! Ambient occlusion samples
		float ao_dist_; //! Ambient occlusion distance
		Rgb ao_col_; //! Ambient occlusion color

		PhotonMapProcessing photon_map_processing_ = PhotonsGenerateOnly;

		int n_paths_; //! Number of samples for mc raytracing
		int max_bounces_; //! Max. path depth for mc raytracing
		std::vector<Light *> lights_; //! An array containing all the scene lights
		bool transp_background_; //! Render background as transparent
		bool transp_refracted_background_; //! Render refractions of background as transparent
		void causticWorker(PhotonMap *caustic_map, int thread_id, const Scene *scene, unsigned int n_caus_photons, Pdf1D *light_power_d, int num_lights, const std::vector<Light *> &caus_lights, int caus_depth, ProgressBar *pb, int pb_step, unsigned int &total_photons_shot);
};

END_YAFARAY

#endif
