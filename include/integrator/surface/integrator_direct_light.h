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

#ifndef LIBYAFARAY_INTEGRATOR_DIRECT_LIGHT_H
#define LIBYAFARAY_INTEGRATOR_DIRECT_LIGHT_H

#include "integrator_photon_caustic.h"

namespace yafaray {

class DirectLightIntegrator final : public CausticPhotonIntegrator
{
		using ThisClassType_t = DirectLightIntegrator; using ParentClassType_t = CausticPhotonIntegrator;

	public:
		inline static std::string getClassName() { return "DirectLightIntegrator"; }
		static std::pair<std::unique_ptr<SurfaceIntegrator>, ParamResult> factory(Logger &logger, const std::string &name, const ParamMap &params);
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return class_meta::print<Params>(excluded_params); }
		DirectLightIntegrator(Logger &logger, ParamResult &param_result, const std::string &name, const ParamMap &param_map);
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;

	private:
		[[nodiscard]] Type type() const override { return Type::DirectLight; }
		bool preprocess(RenderMonitor &render_monitor, const RenderControl &render_control, const Scene &scene) override;
		std::pair<Rgb, float> integrate(Ray &ray, RandomGenerator &random_generator, std::vector<int> &correlative_sample_number, ColorLayers *color_layers, int ray_level, bool chromatic_enabled, float wavelength, int additional_depth, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data) override;
};

} //namespace yafaray

#endif // DIRECTLIGHT
