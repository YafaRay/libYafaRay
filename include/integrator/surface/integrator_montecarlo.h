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
		Rgb estimateAllDirectLight(const Accelerator &accelerator, bool chromatic_enabled, float wavelength, const SurfacePoint &sp, const Vec3 &wo, const RayDivision &ray_division, ColorLayers *color_layers, const Camera *camera, RandomGenerator &random_generator, const PixelSamplingData &pixel_sampling_data) const;
		/*! Like previous but for only one random light source for a given surface point */
		Rgb estimateOneDirectLight(const Accelerator &accelerator, int thread_id, bool chromatic_enabled, float wavelength, const SurfacePoint &sp, const Vec3 &wo, int n, const RayDivision &ray_division, const Camera *camera, RandomGenerator &random_generator, const PixelSamplingData &pixel_sampling_data) const;
		/*! Does the actual light estimation on a specific light for the given surface point */
		Rgb doLightEstimation(const Accelerator &accelerator, bool chromatic_enabled, float wavelength, const Light *light, const SurfacePoint &sp, const Vec3 &wo, unsigned int loffs, const RayDivision &ray_division, ColorLayers *color_layers, const Camera *camera, RandomGenerator &random_generator, const PixelSamplingData &pixel_sampling_data) const;
		/*! Does recursive mc raytracing with MIS (Multiple Importance Sampling) for a given surface point */
		std::pair<Rgb, float> recursiveRaytrace(const Accelerator &accelerator, int thread_id, int ray_level, bool chromatic_enabled, float wavelength, const Ray &ray, const BsdfFlags &bsdfs, SurfacePoint &sp, const Vec3 &wo, int additional_depth, const RayDivision &ray_division, ColorLayers *color_layers, const Camera *camera, RandomGenerator &random_generator, const PixelSamplingData &pixel_sampling_data) const;
		std::pair<Rgb, float> dispersive(const Accelerator &accelerator, int thread_id, int ray_level, bool chromatic_enabled, const SurfacePoint &sp, const Material *material, const BsdfFlags &bsdfs, const Vec3 &wo, int additional_depth, const RayDivision &ray_division, ColorLayers *color_layers, const Camera *camera, RandomGenerator &random_generator, const PixelSamplingData &pixel_sampling_data) const;
		std::pair<Rgb, float> glossy(const Accelerator &accelerator, int thread_id, int ray_level, bool chromatic_enabled, float wavelength, const Ray &ray, const SurfacePoint &sp, const Material *material, const BsdfFlags &mat_bsdfs, const BsdfFlags &bsdfs, const Vec3 &wo, int additional_depth, const RayDivision &ray_division, ColorLayers *color_layers, const Camera *camera, RandomGenerator &random_generator, const PixelSamplingData &pixel_sampling_data) const;
		std::pair<Rgb, float> specularReflect(const Accelerator &accelerator, int thread_id, int ray_level, bool chromatic_enabled, float wavelength, const Ray &ray, const SurfacePoint &sp, const Material *material, const BsdfFlags &bsdfs, const DirectionColor *reflect_data, int additional_depth, const RayDivision &ray_division, ColorLayers *color_layers, RandomGenerator &random_generator, const PixelSamplingData &pixel_sampling_data, const Camera *camera) const;
		std::pair<Rgb, float> specularRefract(const Accelerator &accelerator, int thread_id, int ray_level, bool chromatic_enabled, float wavelength, const Ray &ray, const SurfacePoint &sp, const Material *material, const BsdfFlags &bsdfs, const DirectionColor *refract_data, int additional_depth, const RayDivision &ray_division, ColorLayers *color_layers, RandomGenerator &random_generator, const PixelSamplingData &pixel_sampling_data, const Camera *camera) const;
		Rgb diracLight(const Accelerator &accelerator, const Light *light, const Vec3 &wo, const SurfacePoint &sp, RandomGenerator &random_generator, bool cast_shadows, const Camera *camera, ColorLayers *color_layers) const;
		Rgb areaLightSampleLight(const Accelerator &accelerator, const Light *light, const Vec3 &wo, const SurfacePoint &sp, bool cast_shadows, unsigned int num_samples, float inv_num_samples, Halton &hal_2, Halton &hal_3, ColorLayers *color_layers, const Camera *camera, RandomGenerator &random_generator) const;
		Rgb areaLightSampleMaterial(const Accelerator &accelerator, bool chromatic_enabled, float wavelength, const Light *light, const Vec3 &wo, const SurfacePoint &sp, bool cast_shadows, unsigned int num_samples, float inv_num_samples, Halton &hal_2, Halton &hal_3, ColorLayers *color_layers, const Camera *camera, RandomGenerator &random_generator) const;
		/*! Creates and prepares the caustic photon map */
		bool createCausticMap(const RenderView *render_view, const RenderControl &render_control, const Timer &timer);
		/*! Estimates caustic photons for a given surface point */
		static Rgb estimateCausticPhotons(const SurfacePoint &sp, const Vec3 &wo, const PhotonMap *caustic_map, float caustic_radius, int n_caus_search);
		static Rgb causticPhotons(const Ray &ray, ColorLayers *color_layers, const SurfacePoint &sp, const Vec3 &wo, float clamp_indirect, const PhotonMap *caustic_map, float caustic_radius, int n_caus_search);

		int r_depth_; //! Ray depth

		bool use_photon_caustics_; //! Use photon caustics
		unsigned int n_caus_photons_; //! Number of caustic photons (to be shoot but it should be the target
		int n_caus_search_; //! Amount of caustic photons to be gathered in estimation
		float caus_radius_; //! Caustic search radius for estimation
		int caus_depth_; //! Caustic photons max path depth
		std::unique_ptr<Pdf1D> light_power_d_;

		PhotonMapProcessing photon_map_processing_ = PhotonsGenerateOnly;

		int n_paths_; //! Number of samples for mc raytracing
		int max_bounces_; //! Max. path depth for mc raytracing
		std::vector<const Light *> lights_; //! An array containing all the scene lights
		void causticWorker(const Accelerator &accelerator, PhotonMap *caustic_map, int thread_id, const Scene *scene, const RenderView *render_view, const RenderControl &render_control, const Timer &timer, unsigned int n_caus_photons, Pdf1D *light_power_d, int num_lights, const std::vector<const Light *> &caus_lights, int caus_depth, ProgressBar *pb, int pb_step, unsigned int &total_photons_shot);
		std::unique_ptr<PhotonMap> caustic_map_;

		static constexpr int initial_ray_samples_dispersive_ = 8;
		static constexpr int initial_ray_samples_glossy_ = 8;
		static constexpr int loffs_delta_ = 4567; //just some number to have different sequences per light...and it's a prime even...
		std::pair<Rgb, float> glossyReflectNoTransmit(const Accelerator &accelerator, int thread_id, int ray_level, bool chromatic_enabled, float wavelength, const Ray &ray, const SurfacePoint &sp, const Material *material, const BsdfFlags &bsdfs, const Vec3 &wo, int additional_depth, const Camera *camera, const PixelSamplingData &pixel_sampling_data, const RayDivision &ray_division_new, const float s_1, const float s_2, RandomGenerator &random_generator) const;
		std::pair<Rgb, float> glossyTransmit(const Accelerator &accelerator, int thread_id, int ray_level, bool chromatic_enabled, float wavelength, const Ray &ray, const SurfacePoint &sp, const Material *material, const BsdfFlags &bsdfs, int additional_depth, const ColorLayers *color_layers, const Camera *camera, const PixelSamplingData &pixel_sampling_data, const RayDivision &ray_division_new, const Rgb &transmit_col, float w, const Vec3 &dir, RandomGenerator &random_generator) const;
		std::pair<Rgb, float> glossyReflectDispersive(const Accelerator &accelerator, int thread_id, int ray_level, bool chromatic_enabled, float wavelength, const Ray &ray, const SurfacePoint &sp, const Material *material, const BsdfFlags &bsdfs, int additional_depth, const ColorLayers *color_layers, const Camera *camera, const PixelSamplingData &pixel_sampling_data, const RayDivision &ray_division_new, const Rgb &reflect_color, float w, const Vec3 &dir, RandomGenerator &random_generator) const;
};

END_YAFARAY

#endif
