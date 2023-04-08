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
#include "integrator/volume/integrator_volume.h"
#include "render/imagesplitter.h"
#include "render/imagefilm.h"

namespace yafaray {

std::map<std::string, const ParamMeta *> DebugIntegrator::Params::getParamMetaMap()
{
	auto param_meta_map{ParentClassType_t::Params::getParamMetaMap()};
	PARAM_META(debug_type_);
	PARAM_META(show_pn_);
	return param_meta_map;
}

DebugIntegrator::Params::Params(ParamResult &param_result, const ParamMap &param_map)
{
	PARAM_ENUM_LOAD(debug_type_);
	PARAM_LOAD(show_pn_);
}

ParamMap DebugIntegrator::getAsParamMap(bool only_non_default) const
{
	auto param_map{ParentClassType_t::getAsParamMap(only_non_default)};
	param_map.setParam("type", type().print());
	PARAM_ENUM_SAVE(debug_type_);
	PARAM_SAVE(show_pn_);
	return param_map;
}

std::pair<SurfaceIntegrator *, ParamResult> DebugIntegrator::factory(Logger &logger, const std::string &name, const ParamMap &param_map)
{
	auto param_result{class_meta::check<Params>(param_map, {"type"}, {})};
	auto integrator {new DebugIntegrator(logger, param_result, name, param_map)};
	if(param_result.notOk()) logger.logWarning(param_result.print<ThisClassType_t>(getClassName(), {"type"}));
	return {integrator, param_result};
}

DebugIntegrator::DebugIntegrator(Logger &logger, ParamResult &param_result, const std::string &name, const ParamMap &param_map) : ParentClassType_t(logger, param_result, name, param_map), params_{param_result, param_map}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + getAsParamMap(true).print());
	//FIXME render_info_ += getClassName() + ": '" + params_.debug_type_.print() + "' | ";
}

std::pair<Rgb, float> DebugIntegrator::integrate(ImageFilm *image_film, Ray &ray, RandomGenerator &random_generator, std::vector<int> &correlative_sample_number, ColorLayers *color_layers, int thread_id, int ray_level, bool chromatic_enabled, float wavelength, int additional_depth, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data, unsigned int object_index_highest, unsigned int material_index_highest, float aa_light_sample_multiplier, float aa_indirect_sample_multiplier)
{
	const auto [sp, tmax] = accelerator_->intersect(ray, image_film->getCamera());
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
