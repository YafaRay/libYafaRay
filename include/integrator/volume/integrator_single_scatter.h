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

#ifndef LIBYAFARAY_INTEGRATOR_SINGLE_SCATTER_H
#define LIBYAFARAY_INTEGRATOR_SINGLE_SCATTER_H


#include "integrator/volume/integrator_volume.h"
#include <vector>
#include "render/render_view.h"

namespace yafaray {

class VolumeRegion;
class Light;
class Rgb;
class RandomGenerator;
template <typename T> class SceneItems;

class SingleScatterIntegrator final : public VolumeIntegrator
{
		using ThisClassType_t = SingleScatterIntegrator; using ParentClassType_t = VolumeIntegrator;

	public:
		inline static std::string getClassName() { return "SingleScatterIntegrator"; }
		static std::pair<std::unique_ptr<VolumeIntegrator>, ParamResult> factory(Logger &logger, const ParamMap &param_map, const SceneItems<VolumeRegion> &volume_regions);
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return Params::meta_.print(excluded_params); }
		SingleScatterIntegrator(Logger &logger, ParamResult &param_result, const ParamMap &param_map, const SceneItems<VolumeRegion> &volume_regions);
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;

	private:
		[[nodiscard]] Type type() const override { return Type::SingleScatter; }
		const struct Params
		{
			PARAM_INIT_PARENT(ParentClassType_t);
			PARAM_DECL(float , step_size_, 1.f, "stepSize", "");
			PARAM_DECL(bool , adaptive_, false, "adaptive", "");
			PARAM_DECL(bool , optimize_, false, "optimize", "");
		} params_;
		bool preprocess(FastRandom &fast_random, ImageFilm *image_film, const RenderView *render_view, const Scene &scene, const Renderer &renderer) override;
		// optical thickness, absorption, attenuation, extinction
		Rgb transmittance(RandomGenerator &random_generator, const Ray &ray) const override;
		// emission and in-scattering
		Rgb integrate(RandomGenerator &random_generator, const Ray &ray, int additional_depth) const override;
		Rgb getInScatter(RandomGenerator &random_generator, const Ray &step_ray, float current_step) const;

		const float adaptive_step_size_{params_.step_size_ * 100.0f};
		std::vector<const Light *> lights_;
		const Accelerator *accelerator_ = nullptr;
		const SceneItems<VolumeRegion> &volume_regions_;
};

} //namespace yafaray

#endif // LIBYAFARAY_INTEGRATOR_SINGLE_SCATTER_H