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

#include "texture/texture_marble.h"
#include "param/param.h"

namespace yafaray {

MarbleTexture::Params::Params(ParamError &param_error, const ParamMap &param_map)
{
	PARAM_ENUM_LOAD(shape_);
	PARAM_ENUM_LOAD(noise_type_);
	PARAM_LOAD(color_1_);
	PARAM_LOAD(color_2_);
	PARAM_LOAD(octaves_);
	PARAM_LOAD(size_);
	PARAM_LOAD(hard_);
	PARAM_LOAD(sharpness_);
	PARAM_LOAD(turbulence_);
}

ParamMap MarbleTexture::Params::getAsParamMap(bool only_non_default) const
{
	PARAM_SAVE_START;
	PARAM_ENUM_SAVE(shape_);
	PARAM_ENUM_SAVE(noise_type_);
	PARAM_SAVE(color_1_);
	PARAM_SAVE(color_2_);
	PARAM_SAVE(octaves_);
	PARAM_SAVE(size_);
	PARAM_SAVE(hard_);
	PARAM_SAVE(sharpness_);
	PARAM_SAVE(turbulence_);
	PARAM_SAVE_END;
}

ParamMap MarbleTexture::getAsParamMap(bool only_non_default) const
{
	ParamMap result{Texture::getAsParamMap(only_non_default)};
	result.append(params_.getAsParamMap(only_non_default));
	return result;
}

std::pair<Texture *, ParamError> MarbleTexture::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	auto param_error{Params::meta_.check(param_map, {"type"}, {"ramp_item_"})};
	auto result {new MarbleTexture(logger, param_error, param_map)};
	if(param_error.flags_ != ParamError::Flags::Ok) logger.logWarning(param_error.print<MarbleTexture>(name, {"type"}));
	return {result, param_error};
}

MarbleTexture::MarbleTexture(Logger &logger, ParamError &param_error, const ParamMap &param_map) : Texture{logger, param_error, param_map}, params_{param_error, param_map}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + params_.getAsParamMap(true).print());
}

float MarbleTexture::getFloat(const Point3f &p, const MipMapParams *mipmap_params) const
{
	float w = (p[Axis::X] + p[Axis::Y] + p[Axis::Z]) * 5.f + ((params_.turbulence_ == 0.f) ? 0.f : params_.turbulence_ * NoiseGenerator::turbulence(n_gen_.get(), p, params_.octaves_, params_.size_, params_.hard_));
	switch(params_.shape_.value())
	{
		case Shape::Saw:
			w *= 0.5f * math::div_1_by_pi<>;
			w -= std::floor(w);
			break;
		case Shape::Tri:
			w *= 0.5f * math::div_1_by_pi<>;
			w = std::abs(2.f * (w - std::floor(w)) -1.f);
			break;
		default:
		case Shape::Sin:
			w = 0.5f + 0.5f * math::sin(w);
	}
	return applyIntensityContrastAdjustments(math::pow(w, sharpness_));
}

Rgba MarbleTexture::getColor(const Point3f &p, const MipMapParams *mipmap_params) const
{
	if(!color_ramp_) return applyColorAdjustments(Rgba{params_.color_1_} + Texture::getFloat(p) * Rgba{params_.color_2_ - params_.color_1_});
	else return applyColorAdjustments(color_ramp_->getColorInterpolated(Texture::getFloat(p)));
}

} //namespace yafaray