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
struct BsdfFlags;
class Pdf1D;
class Material;
class MaterialData;
struct DirectionColor;
class Accelerator;
class Halton;

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
		MonteCarloIntegrator(Logger &logger);

	protected:
		virtual ~MonteCarloIntegrator() override;
		/*! Estimates direct light from all sources in a mc fashion and completing MIS (Multiple Importance Sampling) for a given surface point */
		Rgb estimateAllDirectLight(RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo, const RayDivision &ray_division, ColorLayers *color_layers, const Camera *camera) const;
		/*! Like previous but for only one random light source for a given surface point */
		Rgb estimateOneDirectLight(int thread_id, RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo, int n, const RayDivision &ray_division, const Camera *camera) const;
		/*! Does the actual light estimation on a specific light for the given surface point */
		Rgb doLightEstimation(RenderData &render_data, const Light *light, const SurfacePoint &sp, const Vec3 &wo, unsigned int loffs, const RayDivision &ray_division, ColorLayers *color_layers, const Camera *camera) const;
		/*! Does recursive mc raytracing with MIS (Multiple Importance Sampling) for a given surface point */
		void recursiveRaytrace(int thread_id, int ray_level, RenderData &render_data, const Ray &ray, const BsdfFlags &bsdfs, SurfacePoint &sp, const Vec3 &wo, Rgb &col, float &alpha, int additional_depth, const RayDivision &ray_division, ColorLayers *color_layers, const Camera *camera) const;
		Rgb dispersive(int thread_id, int ray_level, RenderData &render_data, const SurfacePoint &sp, const Material *material, const BsdfFlags &bsdfs, const Vec3 &wo, float ray_min_dist, int additional_depth, const RayDivision &ray_division, ColorLayers *color_layers, const Camera *camera) const;
		Rgb glossy(int thread_id, int ray_level, RenderData &render_data, float &alpha, const Ray &ray, const SurfacePoint &sp, const Material *material, const BsdfFlags &mat_bsdfs, const BsdfFlags &bsdfs, const Vec3 &wo, float ray_min_dist, int additional_depth, const RayDivision &ray_division, ColorLayers *color_layers, const Camera *camera) const;
		Rgb specularReflect(int thread_id, int ray_level, RenderData &render_data, float &alpha, const Ray &ray, const SurfacePoint &sp, const Material *material, const BsdfFlags &bsdfs, const DirectionColor *reflect_data, float ray_min_dist, int additional_depth, const RayDivision &ray_division, ColorLayers *color_layers) const;
		Rgb specularRefract(int thread_id, int ray_level, RenderData &render_data, float &alpha, const Ray &ray, const SurfacePoint &sp, const Material *material, const BsdfFlags &bsdfs, const DirectionColor *refract_data, float ray_min_dist, int additional_depth, const RayDivision &ray_division, ColorLayers *color_layers) const;
		Rgb diracLight(const Accelerator *accelerator, const Light *light, const Vec3 &wo, const SurfacePoint &sp, Random *prng, bool cast_shadows, const Camera *camera, ColorLayers *color_layers) const;
		Rgb areaLightSampleLight(const RenderData &render_data, const Accelerator *accelerator, const Light *light, const Vec3 &wo, const SurfacePoint &sp, bool cast_shadows, unsigned int num_samples, float inv_num_samples, Halton &hal_2, Halton &hal_3, ColorLayers *color_layers, const Camera *camera) const;
		Rgb areaLightSampleMaterial(const RenderData &render_data, const Accelerator *accelerator, const Light *light, const Vec3 &wo, const SurfacePoint &sp, bool cast_shadows, unsigned int num_samples, float inv_num_samples, Halton &hal_2, Halton &hal_3, ColorLayers *color_layers, const Camera *camera) const;
		/*! Creates and prepares the caustic photon map */
		bool createCausticMap(const RenderView *render_view, const RenderControl &render_control, const Timer &timer);
		/*! Estimates caustic photons for a given surface point */
		Rgb estimateCausticPhotons(const SurfacePoint &sp, const Vec3 &wo) const;
		/*! Samples ambient occlusion for a given surface point */
		Rgb sampleAmbientOcclusion(RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo, const RayDivision &ray_division, const Camera *camera) const;
		Rgb sampleAmbientOcclusionLayer(RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo, const RayDivision &ray_division, const Camera *camera) const;
		Rgb sampleAmbientOcclusionClayLayer(RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo, const RayDivision &ray_division) const;

		int r_depth_; //! Ray depth
		bool tr_shad_; //! Use transparent shadows
		int s_depth_; //! Shadow depth for transparent shadows

		bool use_photon_caustics_; //! Use photon caustics
		unsigned int n_caus_photons_; //! Number of caustic photons (to be shoot but it should be the target
		int n_caus_search_; //! Amount of caustic photons to be gathered in estimation
		float caus_radius_; //! Caustic search radius for estimation
		int caus_depth_; //! Caustic photons max path depth
		std::unique_ptr<Pdf1D> light_power_d_;

		bool use_ambient_occlusion_; //! Use ambient occlusion
		int ao_samples_; //! Ambient occlusion samples
		float ao_dist_; //! Ambient occlusion distance
		Rgb ao_col_; //! Ambient occlusion color

		PhotonMapProcessing photon_map_processing_ = PhotonsGenerateOnly;

		int n_paths_; //! Number of samples for mc raytracing
		int max_bounces_; //! Max. path depth for mc raytracing
		std::vector<const Light *> lights_; //! An array containing all the scene lights
		bool transp_background_; //! Render background as transparent
		bool transp_refracted_background_; //! Render refractions of background as transparent
		void causticWorker(PhotonMap *caustic_map, int thread_id, const Scene *scene, const RenderView *render_view, const RenderControl &render_control, const Timer &timer, unsigned int n_caus_photons, Pdf1D *light_power_d, int num_lights, const std::vector<const Light *> &caus_lights, int caus_depth, ProgressBar *pb, int pb_step, unsigned int &total_photons_shot);
		std::unique_ptr<PhotonMap> caustic_map_;

		static constexpr int loffs_delta_ = 4567; //just some number to have different sequences per light...and it's a prime even...
};

END_YAFARAY

#endif
