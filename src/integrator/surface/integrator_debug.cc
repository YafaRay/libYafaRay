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

#include "integrator/surface/integrator_debug.h"
#include "geometry/surface.h"
#include "param/param.h"
#include "accelerator/accelerator.h"
#include "render/imagesplitter.h"

namespace yafaray {

DebugIntegrator::Params::Params(ParamResult &param_result, const ParamMap &param_map)
{
	PARAM_ENUM_LOAD(debug_type_);
	PARAM_LOAD(show_pn_);
}

ParamMap DebugIntegrator::Params::getAsParamMap(bool only_non_default) const
{
	PARAM_SAVE_START;
	PARAM_ENUM_SAVE(debug_type_);
	PARAM_SAVE(show_pn_);
	PARAM_SAVE_END;
}

ParamMap DebugIntegrator::getAsParamMap(bool only_non_default) const
{
	ParamMap result{ParentClassType_t::getAsParamMap(only_non_default)};
	result.append(params_.getAsParamMap(only_non_default));
	return result;
}

std::pair<std::unique_ptr<SurfaceIntegrator>, ParamResult> DebugIntegrator::factory(Logger &logger, RenderControl &render_control, const ParamMap &param_map, const Scene &scene)
{
	auto param_result{Params::meta_.check(param_map, {"type"}, {})};
	auto integrator {std::make_unique<ThisClassType_t>(render_control, logger, param_result, param_map)};
	if(param_result.notOk()) logger.logWarning(param_result.print<ThisClassType_t>(getClassName(), {"type"}));
	return {std::move(integrator), param_result};
}

DebugIntegrator::DebugIntegrator(RenderControl &render_control, Logger &logger, ParamResult &param_result, const ParamMap &param_map) : TiledIntegrator(render_control, logger, param_result, param_map), params_{param_result, param_map}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + params_.getAsParamMap(true).print());
	render_info_ += getClassName() + ": '" + params_.debug_type_.print() + "' | ";
}

bool DebugIntegrator::preprocess(FastRandom &fast_random, ImageFilm *image_film, const RenderView *render_view, const Scene &scene)
{
	bool success = SurfaceIntegrator::preprocess(fast_random, image_film, render_view, scene);
	return success;
}

std::pair<Rgb, float> DebugIntegrator::integrate(Ray &ray, FastRandom &fast_random, RandomGenerator &random_generator, std::vector<int> &correlative_sample_number, ColorLayers *color_layers, int thread_id, int ray_level, bool chromatic_enabled, float wavelength, int additional_depth, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data, unsigned int object_index_highest, unsigned int material_index_highest) const
{
	const auto [sp, tmax] = accelerator_->intersect(ray, camera_);
	if(sp)
	{
		Rgb col {0.f};
		switch(params_.debug_type_.value())
		{
			case DebugType::N: col = Rgb((sp->n_[Axis::X] + 1.f) * .5f, (sp->n_[Axis::Y] + 1.f) * .5f, (sp->n_[Axis::Z] + 1.f) * .5f); break;
			case DebugType::DPdU: col = Rgb((sp->dp_.u_[Axis::X] + 1.f) * .5f, (sp->dp_.u_[Axis::Y] + 1.f) * .5f, (sp->dp_.u_[Axis::Z] + 1.f) * .5f);
				break;
			case DebugType::DPdV: col = Rgb((sp->dp_.v_[Axis::X] + 1.f) * .5f, (sp->dp_.v_[Axis::Y] + 1.f) * .5f, (sp->dp_.v_[Axis::Z] + 1.f) * .5f);
				break;
			case DebugType::Nu: col = Rgb((sp->uvn_.u_[Axis::X] + 1.f) * .5f, (sp->uvn_.u_[Axis::Y] + 1.f) * .5f, (sp->uvn_.u_[Axis::Z] + 1.f) * .5f);
				break;
			case DebugType::Nv: col = Rgb((sp->uvn_.v_[Axis::X] + 1.f) * .5f, (sp->uvn_.v_[Axis::Y] + 1.f) * .5f, (sp->uvn_.v_[Axis::Z] + 1.f) * .5f);
				break;
			case DebugType::DSdU: col = Rgb((sp->ds_.u_[Axis::X] + 1.f) * .5f, (sp->ds_.u_[Axis::Y] + 1.f) * .5f, (sp->ds_.u_[Axis::Z] + 1.f) * .5f);
				break;
			case DebugType::DSdV: col = Rgb((sp->ds_.v_[Axis::X] + 1.f) * .5f, (sp->ds_.v_[Axis::Y] + 1.f) * .5f, (sp->ds_.v_[Axis::Z] + 1.f) * .5f);
				break;
		}
		return {std::move(col), 1.f};
	}
	else return {Rgb{0.f}, 1.f};
}

} //namespace yafaray
