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

namespace yafaray {

class Pdf1D;
class Halton;

class MonteCarloIntegrator: public TiledIntegrator
{
	public:
		inline static std::string getClassName() { return "MonteCarloIntegrator"; }
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return Params::meta_.print(excluded_params); }

	protected:
		const struct Params
		{
			PARAM_INIT_PARENT(TiledIntegrator);
			PARAM_DECL(int, r_depth_, 5, "raydepth", "Ray depth");
			PARAM_DECL(bool, transparent_shadows_, false, "transpShad", "Use transparent shadows");
			PARAM_DECL(int, shadow_depth_, 4, "shadowDepth", "Shadow depth for transparent shadows");
			PARAM_DECL(bool, ao_, false, "do_AO", "Use ambient occlusion");
			PARAM_DECL(int, ao_samples_, 32, "AO_samples", "Ambient occlusion samples");
			PARAM_DECL(float, ao_distance_, 1.f, "AO_distance", "Ambient occlusion distance");
			PARAM_DECL(Rgb, ao_color_, Rgb{1.f}, "AO_color", "Ambient occlusion color");
			PARAM_DECL(bool, transparent_background_, false, "bg_transp", "Render background as transparent");
			PARAM_DECL(bool, transparent_background_refraction_, false, "bg_transp_refract", "Render refractions of background as transparent");
		} params_;
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;
		MonteCarloIntegrator(RenderControl &render_control, Logger &logger, ParamError &param_error, const ParamMap &param_map);
		~MonteCarloIntegrator() override;
		/*! Estimates direct light from all sources in a mc fashion and completing MIS (Multiple Importance Sampling) for a given surface point */
		Rgb estimateAllDirectLight(RandomGenerator &random_generator, ColorLayers *color_layers, bool chromatic_enabled, float wavelength, const SurfacePoint &sp, const Vec3f &wo, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data) const;
		/*! Like previous but for only one random light source for a given surface point */
		Rgb estimateOneDirectLight(RandomGenerator &random_generator, std::vector<int> &correlative_sample_number, int thread_id, bool chromatic_enabled, float wavelength, const SurfacePoint &sp, const Vec3f &wo, int n, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data) const;
		/*! Does the actual light estimation on a specific light for the given surface point */
		Rgb doLightEstimation(RandomGenerator &random_generator, ColorLayers *color_layers, bool chromatic_enabled, float wavelength, const Light *light, const SurfacePoint &sp, const Vec3f &wo, unsigned int loffs, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data) const;
		/*! Does recursive mc raytracing with MIS (Multiple Importance Sampling) for a given surface point */
		std::pair<Rgb, float> recursiveRaytrace(FastRandom &fast_random, RandomGenerator &random_generator, std::vector<int> &correlative_sample_number, ColorLayers *color_layers, int thread_id, int ray_level, bool chromatic_enabled, float wavelength, const Ray &ray, BsdfFlags bsdfs, const SurfacePoint &sp, const Vec3f &wo, int additional_depth, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data) const;
		std::pair<Rgb, float> dispersive(FastRandom &fast_random, RandomGenerator &random_generator, std::vector<int> &correlative_sample_number, ColorLayers *color_layers, int thread_id, int ray_level, bool chromatic_enabled, const SurfacePoint &sp, const Material *material, BsdfFlags bsdfs, const Vec3f &wo, int additional_depth, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data) const;
		std::pair<Rgb, float> glossy(FastRandom &fast_random, RandomGenerator &random_generator, std::vector<int> &correlative_sample_number, ColorLayers *color_layers, int thread_id, int ray_level, bool chromatic_enabled, float wavelength, const Ray &ray, const SurfacePoint &sp, BsdfFlags bsdfs, const Vec3f &wo, int additional_depth, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data) const;
		std::pair<Rgb, float> specularReflect(FastRandom &fast_random, RandomGenerator &random_generator, std::vector<int> &correlative_sample_number, ColorLayers *color_layers, int thread_id, int ray_level, bool chromatic_enabled, float wavelength, const Ray &ray, const SurfacePoint &sp, const Material *material, BsdfFlags bsdfs, const DirectionColor *reflect_data, int additional_depth, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data) const;
		std::pair<Rgb, float> specularRefract(FastRandom &fast_random, RandomGenerator &random_generator, std::vector<int> &correlative_sample_number, ColorLayers *color_layers, int thread_id, int ray_level, bool chromatic_enabled, float wavelength, const Ray &ray, const SurfacePoint &sp, const Material *material, BsdfFlags bsdfs, const DirectionColor *refract_data, int additional_depth, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data) const;
		Rgb diracLight(RandomGenerator &random_generator, ColorLayers *color_layers, const Light *light, const Vec3f &wo, const SurfacePoint &sp, bool cast_shadows, float time) const;
		Rgb areaLightSampleLight(Halton &hal_2, Halton &hal_3, RandomGenerator &random_generator, ColorLayers *color_layers, const Light *light, const Vec3f &wo, const SurfacePoint &sp, bool cast_shadows, unsigned int num_samples, float inv_num_samples, float time) const;
		Rgb areaLightSampleMaterial(Halton &hal_2, Halton &hal_3, RandomGenerator &random_generator, ColorLayers *color_layers, bool chromatic_enabled, float wavelength, const Light *light, const Vec3f &wo, const SurfacePoint &sp, bool cast_shadows, unsigned int num_samples, float inv_num_samples) const;
		std::pair<Rgb, float> glossyReflectNoTransmit(FastRandom &fast_random, RandomGenerator &random_generator, std::vector<int> &correlative_sample_number, int thread_id, int ray_level, bool chromatic_enabled, float wavelength, const Ray &ray, const SurfacePoint &sp, BsdfFlags bsdfs, const Vec3f &wo, int additional_depth, const PixelSamplingData &pixel_sampling_data, const RayDivision &ray_division_new, float s_1, float s_2) const;
		std::pair<Rgb, float> glossyTransmit(FastRandom &fast_random, RandomGenerator &random_generator, std::vector<int> &correlative_sample_number, int thread_id, int ray_level, bool chromatic_enabled, float wavelength, const Ray &ray, const SurfacePoint &sp, BsdfFlags bsdfs, int additional_depth, const PixelSamplingData &pixel_sampling_data, const RayDivision &ray_division_new, const Rgb &transmit_col, float w, const Vec3f &dir) const;
		std::pair<Rgb, float> glossyReflect(FastRandom &fast_random, RandomGenerator &random_generator, std::vector<int> &correlative_sample_number, int thread_id, int ray_level, bool chromatic_enabled, float wavelength, const Ray &ray, const SurfacePoint &sp, BsdfFlags bsdfs, int additional_depth, const PixelSamplingData &pixel_sampling_data, const RayDivision &ray_division_new, const Rgb &reflect_color, float w, const Vec3f &dir) const;

		std::vector<const Light *> lights_; //! An array containing all the visible scene lights

		static constexpr inline int initial_ray_samples_dispersive_ = 8;
		static constexpr inline int initial_ray_samples_glossy_ = 8;
		static constexpr inline int loffs_delta_ = 4567; //just some number to have different sequences per light...and it's a prime even...
};

} //namespace yafaray

#endif
