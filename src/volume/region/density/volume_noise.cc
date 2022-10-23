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

#include "volume/region/density/volume_noise.h"
#include "geometry/surface.h"
#include "texture/texture.h"
#include "param/param.h"
#include "scene/scene.h"

namespace yafaray {

NoiseVolumeRegion::Params::Params(ParamResult &param_result, const ParamMap &param_map)
{
	PARAM_LOAD(sharpness_);
	PARAM_LOAD(density_);
	PARAM_LOAD(cover_);
	PARAM_LOAD(texture_);
}

ParamMap NoiseVolumeRegion::Params::getAsParamMap(bool only_non_default) const
{
	PARAM_SAVE_START;
	PARAM_SAVE(sharpness_);
	PARAM_SAVE(density_);
	PARAM_SAVE(cover_);
	PARAM_SAVE(texture_);
	PARAM_SAVE_END;
}

ParamMap NoiseVolumeRegion::getAsParamMap(bool only_non_default) const
{
	ParamMap result{ParentClassType_t::getAsParamMap(only_non_default)};
	result.append(params_.getAsParamMap(only_non_default));
	return result;
}

std::pair<std::unique_ptr<VolumeRegion>, ParamResult> NoiseVolumeRegion::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	auto param_result{Params::meta_.check(param_map, {"type"}, {})};
	std::string tex_name;
	param_map.getParam(Params::texture_meta_.name(), tex_name);
	if(tex_name.empty())
	{
		if(logger.isVerbose()) logger.logVerbose(getClassName() + ": Noise texture not set, the volume region won't be created.");
		return {nullptr, ParamResult{ResultFlags::ErrorWhileCreating}};
	}
	const Texture *texture{scene.getTexture(tex_name)};
	if(!texture)
	{
		if(logger.isVerbose()) logger.logVerbose(getClassName() + ": Noise texture '", tex_name, "' couldn't be found, the volume region won't be created.");
		return {nullptr, ParamResult{ResultFlags::ErrorWhileCreating}};
	}
	auto volume_region {std::make_unique<ThisClassType_t>(logger, param_result, param_map, texture)};
	if(param_result.notOk()) logger.logWarning(param_result.print<ThisClassType_t>(name, {"type"}));
	return {std::move(volume_region), param_result};
}

NoiseVolumeRegion::NoiseVolumeRegion(Logger &logger, ParamResult &param_result, const ParamMap &param_map, const Texture *texture) :
		ParentClassType_t{logger, param_result, param_map}, params_{param_result, param_map}, tex_dist_noise_{texture}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + params_.getAsParamMap(true).print());
}

float NoiseVolumeRegion::density(const Point3f &p) const
{
	float d = tex_dist_noise_->getColor(p * 0.1f).energy();

	d = 1.0f / (1.0f + math::exp(sharpness_ * (1.0f - params_.cover_ - d)));
	d *= params_.density_;

	return d;
}

} //namespace yafaray
