#pragma once
/****************************************************************************
 *      directlight.cc: an integrator for direct lighting only
 *      This is part of the libYafaRay package
 *      Copyright (C) 2006  Mathias Wein
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

#ifndef LIBYAFARAY_INTEGRATOR_DEBUG_H
#define LIBYAFARAY_INTEGRATOR_DEBUG_H

#include "integrator_tiled.h"

namespace yafaray {

class Light;

class DebugIntegrator final : public TiledIntegrator
{
		using ThisClassType_t = DebugIntegrator; using ParentClassType_t = TiledIntegrator;

	public:
		inline static std::string getClassName() { return "DebugIntegrator"; }
		static std::pair<std::unique_ptr<SurfaceIntegrator>, ParamResult> factory(Logger &logger, const std::string &name, const ParamMap &params);
		[[nodiscard]] std::map<std::string, const ParamMeta *> getParamMetaMap() const override { return params_.getParamMetaMap(); }
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return class_meta::print<Params>(excluded_params); }
		DebugIntegrator(Logger &logger, ParamResult &param_result, const std::string &name, const ParamMap &param_map);
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;

	private:
		struct DebugType : public Enum<DebugType>
		{
			using Enum::Enum;
			enum : ValueType_t {N = 1, DPdU = 2, DPdV = 3, Nu = 4, Nv = 5, DSdU = 6, DSdV = 7};
			inline static const EnumMap<ValueType_t> map_{{
					{"N", N, ""},
					{"dPdU", DPdU, ""},
					{"dPdV", DPdV, ""},
					{"NU", Nu, ""},
					{"NV", Nv, ""},
					{"dSdU", DSdU, ""},
					{"dSdV", DSdV, ""},
				}};
		};
		[[nodiscard]] Type type() const override { return Type::Debug; }
		const struct Params
		{
			Params(ParamResult &param_result, const ParamMap &param_map);
			static std::map<std::string, const ParamMeta *> getParamMetaMap();
			PARAM_ENUM_DECL(DebugType, debug_type_, DebugType::N, "debugType", "");
			PARAM_DECL(bool , show_pn_, false, "showPN", "");
		} params_;
		std::pair<Rgb, float> integrate(Ray &ray, RandomGenerator &random_generator, std::vector<int> &correlative_sample_number, ColorLayers *color_layers, int ray_level, bool chromatic_enabled, float wavelength, int additional_depth, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data) override;
};

} //namespace yafaray

#endif // LIBYAFARAY_INTEGRATOR_DEBUG_H