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

#ifndef LIBYAFARAY_INTEGRATOR_SKY_H
#define LIBYAFARAY_INTEGRATOR_SKY_H

#include <math/random.h>
#include "integrator/volume/integrator_volume.h"
#include "render/renderer.h"

namespace yafaray {

class Background;
template <typename T> class Items;

class SkyIntegrator : public VolumeIntegrator
{
		using ThisClassType_t = SkyIntegrator; using ParentClassType_t = VolumeIntegrator;

	public:
		inline static std::string getClassName() { return "SkyIntegrator"; }
		static std::pair<std::unique_ptr<VolumeIntegrator>, ParamResult> factory(Logger &logger, const ParamMap &params);
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return class_meta::print<Params>(excluded_params); }
		SkyIntegrator(Logger &logger, ParamResult &param_result, const ParamMap &param_map);
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;

	private:
		[[nodiscard]] Type type() const override { return Type::Sky; }
		const struct Params
		{
			Params(ParamResult &param_result, const ParamMap &param_map);
			static std::map<std::string, const ParamMeta *> getParamMetaMap();
			PARAM_DECL(float , step_size_, 1.f, "stepSize", "");
			PARAM_DECL(float , scale_, 0.1f, "sigma_t", "Actually it is the scale_ variable in the code. It's unclear what this parameter actually means in the code at the moment"); //"Beta in the paper, more or less the thickness coefficient");//FIXME DAVID: it seems to be unused in the code (using the scale_ variable instead for some reason?) or even worse, used with uninitialized values. Not sure why is this the case, but I'm removing its usage from the code completely for now
			PARAM_DECL(float , alpha_, 0.5f, "alpha", "Steepness of the exponential density");
			PARAM_DECL(float , turbidity_, 3.f, "turbidity", "");
		} params_;
		bool preprocess(const Scene &scene, const SurfaceIntegrator &surf_integrator) override;
		// optical thickness, absorption, attenuation, extinction
		Rgb transmittance(RandomGenerator &random_generator, const Ray &ray) const override;
		// emission and in-scattering
		Rgb integrate(RandomGenerator &random_generator, const Ray &ray, int additional_depth) const override;
		//Rgb skyTau(const Ray &ray) const;//FIXME: sigma_t is unused at the moment for some reason, and this function is also unused.
		Rgb skyTau(const Ray &ray, float beta, float alpha) const;
		static float mieScatter(float theta);

		//float sigma_t_; // beta in the paper, more or less the thickness coefficient //FIXME DAVID: it seems to be unused in the code (using the scale_ variable instead for some reason?) or even worse, used with uninitialized values. Not sure why is this the case, but I'm removing its usage from the code completely for now
		const Background *background_ = nullptr;
		float b_m_, b_r_;
		float alpha_r_; // rayleigh, molecules
		float alpha_m_; // mie, haze
};

} //namespace yafaray

#endif // LIBYAFARAY_INTEGRATOR_SKY_H