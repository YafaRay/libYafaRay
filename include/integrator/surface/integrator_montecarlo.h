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
		MonteCarloIntegrator(RenderControl &render_control, Logger &logger);

	protected:
		~MonteCarloIntegrator() override;
		/*! Estimates direct light from all sources in a mc fashion and completing MIS (Multiple Importance Sampling) for a given surface point */
		Rgb estimateAllDirectLight(RandomGenerator &random_generator, ColorLayers *color_layers, bool chromatic_enabled, float wavelength, const SurfacePoint &sp, const Vec3 &wo, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data) const;
		/*! Like previous but for only one random light source for a given surface point */
		Rgb estimateOneDirectLight(RandomGenerator &random_generator, std::vector<int> &correlative_sample_number, int thread_id, bool chromatic_enabled, float wavelength, const SurfacePoint &sp, const Vec3 &wo, int n, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data) const;
		/*! Does the actual light estimation on a specific light for the given surface point */
		Rgb doLightEstimation(RandomGenerator &random_generator, ColorLayers *color_layers, bool chromatic_enabled, float wavelength, const Light *light, const SurfacePoint &sp, const Vec3 &wo, unsigned int loffs, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data) const;
		/*! Does recursive mc raytracing with MIS (Multiple Importance Sampling) for a given surface point */
		std::pair<Rgb, float> recursiveRaytrace(FastRandom &fast_random, RandomGenerator &random_generator, std::vector<int> &correlative_sample_number, ColorLayers *color_layers, int thread_id, int ray_level, bool chromatic_enabled, float wavelength, const Ray &ray, const BsdfFlags &bsdfs, const SurfacePoint &sp, const Vec3 &wo, int additional_depth, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data) const;
		std::pair<Rgb, float> dispersive(FastRandom &fast_random, RandomGenerator &random_generator, std::vector<int> &correlative_sample_number, ColorLayers *color_layers, int thread_id, int ray_level, bool chromatic_enabled, const SurfacePoint &sp, const Material *material, const BsdfFlags &bsdfs, const Vec3 &wo, int additional_depth, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data) const;
		std::pair<Rgb, float> glossy(FastRandom &fast_random, RandomGenerator &random_generator, std::vector<int> &correlative_sample_number, ColorLayers *color_layers, int thread_id, int ray_level, bool chromatic_enabled, float wavelength, const Ray &ray, const SurfacePoint &sp, const BsdfFlags &bsdfs, const Vec3 &wo, int additional_depth, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data) const;
		std::pair<Rgb, float> specularReflect(FastRandom &fast_random, RandomGenerator &random_generator, std::vector<int> &correlative_sample_number, ColorLayers *color_layers, int thread_id, int ray_level, bool chromatic_enabled, float wavelength, const Ray &ray, const SurfacePoint &sp, const Material *material, const BsdfFlags &bsdfs, const DirectionColor *reflect_data, int additional_depth, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data) const;
		std::pair<Rgb, float> specularRefract(FastRandom &fast_random, RandomGenerator &random_generator, std::vector<int> &correlative_sample_number, ColorLayers *color_layers, int thread_id, int ray_level, bool chromatic_enabled, float wavelength, const Ray &ray, const SurfacePoint &sp, const Material *material, const BsdfFlags &bsdfs, const DirectionColor *refract_data, int additional_depth, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data) const;
		Rgb diracLight(RandomGenerator &random_generator, ColorLayers *color_layers, const Light *light, const Vec3 &wo, const SurfacePoint &sp, bool cast_shadows) const;
		Rgb areaLightSampleLight(Halton &hal_2, Halton &hal_3, RandomGenerator &random_generator, ColorLayers *color_layers, const Light *light, const Vec3 &wo, const SurfacePoint &sp, bool cast_shadows, unsigned int num_samples, float inv_num_samples) const;
		Rgb areaLightSampleMaterial(Halton &hal_2, Halton &hal_3, RandomGenerator &random_generator, ColorLayers *color_layers, bool chromatic_enabled, float wavelength, const Light *light, const Vec3 &wo, const SurfacePoint &sp, bool cast_shadows, unsigned int num_samples, float inv_num_samples) const;
		/*! Creates and prepares the caustic photon map */
		bool createCausticMap(FastRandom &fast_random);
		void causticWorker(FastRandom &fast_random, unsigned int &total_photons_shot, int thread_id, const Pdf1D *light_power_d_caustic, const std::vector<const Light *> &lights_caustic, int pb_step);
		std::pair<Rgb, float> glossyReflectNoTransmit(FastRandom &fast_random, RandomGenerator &random_generator, std::vector<int> &correlative_sample_number, int thread_id, int ray_level, bool chromatic_enabled, float wavelength, const Ray &ray, const SurfacePoint &sp, const BsdfFlags &bsdfs, const Vec3 &wo, int additional_depth, const PixelSamplingData &pixel_sampling_data, const RayDivision &ray_division_new, const float s_1, const float s_2) const;
		std::pair<Rgb, float> glossyTransmit(FastRandom &fast_random, RandomGenerator &random_generator, std::vector<int> &correlative_sample_number, int thread_id, int ray_level, bool chromatic_enabled, float wavelength, const Ray &ray, const SurfacePoint &sp, const BsdfFlags &bsdfs, int additional_depth, const PixelSamplingData &pixel_sampling_data, const RayDivision &ray_division_new, const Rgb &transmit_col, float w, const Vec3 &dir) const;
		std::pair<Rgb, float> glossyReflect(FastRandom &fast_random, RandomGenerator &random_generator, std::vector<int> &correlative_sample_number, int thread_id, int ray_level, bool chromatic_enabled, float wavelength, const Ray &ray, const SurfacePoint &sp, const BsdfFlags &bsdfs, int additional_depth, const PixelSamplingData &pixel_sampling_data, const RayDivision &ray_division_new, const Rgb &reflect_color, float w, const Vec3 &dir) const;
		/*! Estimates caustic photons for a given surface point */
		static Rgb estimateCausticPhotons(const SurfacePoint &sp, const Vec3 &wo, const PhotonMap *caustic_map, float caustic_radius, int n_caus_search);
		static Rgb causticPhotons(ColorLayers *color_layers, const Ray &ray, const SurfacePoint &sp, const Vec3 &wo, float clamp_indirect, const PhotonMap *caustic_map, float caustic_radius, int n_caus_search);

		int r_depth_; //! Ray depth

		bool use_photon_caustics_; //! Use photon caustics
		unsigned int n_caus_photons_; //! Number of caustic photons (to be shoot but it should be the target
		int n_caus_search_; //! Amount of caustic photons to be gathered in estimation
		float caus_radius_; //! Caustic search radius for estimation
		int caus_depth_; //! Caustic photons max path depth

		PhotonMapProcessing photon_map_processing_ = PhotonsGenerateOnly;

		int n_paths_; //! Number of samples for mc raytracing
		int max_bounces_; //! Max. path depth for mc raytracing
		std::vector<const Light *> lights_; //! An array containing all the visible scene lights
		std::unique_ptr<PhotonMap> caustic_map_;

		static constexpr int initial_ray_samples_dispersive_ = 8;
		static constexpr int initial_ray_samples_glossy_ = 8;
		static constexpr int loffs_delta_ = 4567; //just some number to have different sequences per light...and it's a prime even...
};

END_YAFARAY

#endif
