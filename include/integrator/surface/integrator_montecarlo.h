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

#ifndef LIBYAFARAY_INTEGRATOR_MONTECARLO_H
#define LIBYAFARAY_INTEGRATOR_MONTECARLO_H

#include "integrator_tiled.h"
#include "color/color.h"

namespace yafaray {

class Pdf1D;
class Halton;

class MonteCarloIntegrator: public TiledIntegrator
{
		using ThisClassType_t = MonteCarloIntegrator; using ParentClassType_t = TiledIntegrator;

	public:
		inline static std::string getClassName() { return "MonteCarloIntegrator"; }
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return class_meta::print<Params>(excluded_params); }

	protected:
		const struct Params
		{
			Params(ParamResult &param_result, const ParamMap &param_map);
			static std::map<std::string, const ParamMeta *> getParamMetaMap();
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
		MonteCarloIntegrator(Logger &logger, ParamResult &param_result, const std::string &name, const ParamMap &param_map);
		~MonteCarloIntegrator() override;
		/*! Estimates direct light from all sources in a mc fashion and completing MIS (Multiple Importance Sampling) for a given surface point */
		Rgb estimateAllDirectLight(RandomGenerator &random_generator, ColorLayers *color_layers, const Camera *camera, bool chromatic_enabled, float wavelength, float aa_light_sample_multiplier, const SurfacePoint &sp, const Vec3f &wo, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data) const;
		/*! Like previous but for only one random light source for a given surface point */
		Rgb estimateOneDirectLight(RandomGenerator &random_generator, std::vector<int> &correlative_sample_number, unsigned int base_sampling_offset, int thread_id, const Camera *camera, bool chromatic_enabled, float wavelength, const SurfacePoint &sp, const Vec3f &wo, int n, float aa_light_sample_multiplier, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data) const;
		/*! Does recursive mc raytracing with MIS (Multiple Importance Sampling) for a given surface point */
		std::pair<Rgb, float> recursiveRaytrace(ImageFilm &image_film, RandomGenerator &random_generator, std::vector<int> &correlative_sample_number, ColorLayers *color_layers, int thread_id, int ray_level, bool chromatic_enabled, float aa_light_sample_multiplier, float aa_indirect_sample_multiplier, float wavelength, const Ray &ray, BsdfFlags bsdfs, const SurfacePoint &sp, const Vec3f &wo, int additional_depth, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data);

		static constexpr inline int initial_ray_samples_dispersive_ = 8;
		static constexpr inline int initial_ray_samples_glossy_ = 8;

	private:
		/*! Does the actual light estimation on a specific light for the given surface point */
		Rgb doLightEstimation(RandomGenerator &random_generator, ColorLayers *color_layers, const Camera *camera, bool chromatic_enabled, float wavelength, const Light *light, const SurfacePoint &sp, const Vec3f &wo, unsigned int loffs, float aa_light_sample_multiplier, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data) const;
		std::pair<Rgb, float> dispersive(ImageFilm &image_film, RandomGenerator &random_generator, std::vector<int> &correlative_sample_number, ColorLayers *color_layers, int thread_id, int ray_level, bool chromatic_enabled, float aa_light_sample_multiplier, float aa_indirect_sample_multiplier, const SurfacePoint &sp, const Material *material, BsdfFlags bsdfs, const Vec3f &wo, int additional_depth, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data);
		std::pair<Rgb, float> glossy(ImageFilm &image_film, RandomGenerator &random_generator, std::vector<int> &correlative_sample_number, ColorLayers *color_layers, int thread_id, int ray_level, bool chromatic_enabled, float aa_light_sample_multiplier, float aa_indirect_sample_multiplier, float wavelength, const Ray &ray, const SurfacePoint &sp, BsdfFlags bsdfs, const Vec3f &wo, int additional_depth, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data);
		std::pair<Rgb, float> specularReflect(ImageFilm &image_film, RandomGenerator &random_generator, std::vector<int> &correlative_sample_number, ColorLayers *color_layers, int thread_id, int ray_level, bool chromatic_enabled, float aa_light_sample_multiplier, float aa_indirect_sample_multiplier, float wavelength, const Ray &ray, const SurfacePoint &sp, const Material *material, BsdfFlags bsdfs, const DirectionColor *reflect_data, int additional_depth, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data);
		std::pair<Rgb, float> specularRefract(ImageFilm &image_film, RandomGenerator &random_generator, std::vector<int> &correlative_sample_number, ColorLayers *color_layers, int thread_id, int ray_level, bool chromatic_enabled, float aa_light_sample_multiplier, float aa_indirect_sample_multiplier, float wavelength, const Ray &ray, const SurfacePoint &sp, const Material *material, BsdfFlags bsdfs, const DirectionColor *refract_data, int additional_depth, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data);
		Rgb diracLight(RandomGenerator &random_generator, ColorLayers *color_layers, const Camera *camera, const Light *light, const Vec3f &wo, const SurfacePoint &sp, bool cast_shadows, float time) const;
		Rgb areaLightSampleLight(Halton &hal_2, Halton &hal_3, RandomGenerator &random_generator, ColorLayers *color_layers, const Camera *camera, const Light *light, const Vec3f &wo, const SurfacePoint &sp, bool cast_shadows, unsigned int num_samples, float inv_num_samples, float time) const;
		Rgb areaLightSampleMaterial(Halton &hal_2, Halton &hal_3, RandomGenerator &random_generator, ColorLayers *color_layers, const Camera *camera, bool chromatic_enabled, float wavelength, const Light *light, const Vec3f &wo, const SurfacePoint &sp, bool cast_shadows, unsigned int num_samples, float inv_num_samples) const;
		std::pair<Rgb, float> glossyReflectNoTransmit(ImageFilm &image_film, RandomGenerator &random_generator, std::vector<int> &correlative_sample_number, int thread_id, int ray_level, bool chromatic_enabled, float aa_light_sample_multiplier, float aa_indirect_sample_multiplier, float wavelength, const Ray &ray, const SurfacePoint &sp, BsdfFlags bsdfs, const Vec3f &wo, int additional_depth, const PixelSamplingData &pixel_sampling_data, const RayDivision &ray_division_new, const float s_1, const float s_2);
		std::pair<Rgb, float> glossyTransmit(ImageFilm &image_film, RandomGenerator &random_generator, std::vector<int> &correlative_sample_number, int thread_id, int ray_level, bool chromatic_enabled, float aa_light_sample_multiplier, float aa_indirect_sample_multiplier, float wavelength, const Ray &ray, const SurfacePoint &sp, BsdfFlags bsdfs, int additional_depth, const PixelSamplingData &pixel_sampling_data, const RayDivision &ray_division_new, const Rgb &transmit_col, float w, const Vec3f &dir);
		std::pair<Rgb, float> glossyReflect(ImageFilm &image_film, RandomGenerator &random_generator, std::vector<int> &correlative_sample_number, int thread_id, int ray_level, bool chromatic_enabled, float aa_light_sample_multiplier, float aa_indirect_sample_multiplier, float wavelength, const Ray &ray, const SurfacePoint &sp, BsdfFlags bsdfs, int additional_depth, const PixelSamplingData &pixel_sampling_data, const RayDivision &ray_division_new, const Rgb &reflect_color, float w, const Vec3f &dir);

		static constexpr inline int loffs_delta_ = 4567; //just some number to have different sequences per light...and it's a prime even...
};

} //namespace yafaray

#endif
