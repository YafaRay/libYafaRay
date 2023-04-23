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

#ifndef LIBYAFARAY_INTEGRATOR_PATH_TRACER_H
#define LIBYAFARAY_INTEGRATOR_PATH_TRACER_H

#include "integrator_photon_caustic.h"

namespace yafaray {

class PathIntegrator final : public CausticPhotonIntegrator
{
		using ThisClassType_t = PathIntegrator; using ParentClassType_t = CausticPhotonIntegrator;

	public:
		inline static std::string getClassName() { return "PathIntegrator"; }
		static std::pair<std::unique_ptr<SurfaceIntegrator>, ParamResult> factory(Logger &logger, const std::string &name, const ParamMap &params);
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return class_meta::print<Params>(excluded_params); }
		PathIntegrator(Logger &logger, ParamResult &param_result, const std::string &name, const ParamMap &param_map);
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;

	private:
		struct CausticType : public Enum<CausticType>
		{
			enum : ValueType_t { None = 0, Path = 1 << 0, Photon = 1 << 1, Both = Path | Photon };
			inline static const EnumMap<ValueType_t> map_{{
					{"none", None, ""},
					{"path", Path, ""},
					{"photon", Photon, ""},
					{"both", Both, ""},
				}};
		};
		[[nodiscard]] Type type() const override { return Type::Path; }
		const struct Params
		{
			Params(ParamResult &param_result, const ParamMap &param_map);
			static std::map<std::string, const ParamMeta *> getParamMetaMap();
			PARAM_DECL(int, path_samples_, 32, "path_samples", "Number of samples for Montecarlo raytracing");
			PARAM_DECL(int, bounces_, 3, "bounces", "Max. path depth for Montecarlo raytracing");
			PARAM_DECL(int, russian_roulette_min_bounces_, 0, "russian_roulette_min_bounces", "Minimum number of bounces where russian roulette is not applied. Afterwards russian roulette will be used until the maximum selected bounces. If min_bounces >= max_bounces, then no russian roulette takes place");
			PARAM_DECL(bool, no_recursive_, false, "no_recursive", "");
			PARAM_ENUM_DECL(CausticType , caustic_type_, CausticType::Path, "caustic_type", "");
		} params_;
		bool preprocess(RenderControl &render_control, RenderMonitor &render_monitor, const Scene &scene) override;
		std::pair<Rgb, float> integrate(ImageFilm &image_film, Ray &ray, RandomGenerator &random_generator, std::vector<int> &correlative_sample_number, ColorLayers *color_layers, int thread_id, int ray_level, bool chromatic_enabled, float wavelength, int additional_depth, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data, unsigned int object_index_highest, unsigned int material_index_highest, float aa_light_sample_multiplier, float aa_indirect_sample_multiplier) override;
};

} //namespace yafaray

#endif // PATHTRACER
