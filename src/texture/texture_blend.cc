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

#include "texture/texture_blend.h"
#include "param/param.h"

namespace yafaray {

BlendTexture::Params::Params(ParamError &param_error, const ParamMap &param_map)
{
	PARAM_ENUM_LOAD(blend_type_);
	PARAM_LOAD(use_flip_axis_);
}

ParamMap BlendTexture::Params::getAsParamMap(bool only_non_default) const
{
	PARAM_SAVE_START;
	PARAM_ENUM_SAVE(blend_type_);
	PARAM_SAVE(use_flip_axis_);
	PARAM_SAVE_END;
}

ParamMap BlendTexture::getAsParamMap(bool only_non_default) const
{
	ParamMap result{Texture::getAsParamMap(only_non_default)};
	result.append(params_.getAsParamMap(only_non_default));
	return result;
}

std::pair<Texture *, ParamError> BlendTexture::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	auto param_error{Params::meta_.check(param_map, {"type"}, {"ramp_item_"})};
	auto result {new BlendTexture(logger, param_error, param_map)};
	if(param_error.flags_ != ParamError::Flags::Ok) logger.logWarning(param_error.print<BlendTexture>(name, {"type"}));
	return {result, param_error};
}

BlendTexture::BlendTexture(Logger &logger, ParamError &param_error, const ParamMap &param_map) : Texture{logger, param_error, param_map},
		params_{param_error, param_map}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + params_.getAsParamMap(true).print());
}

float BlendTexture::getFloat(const Point3f &p, const MipMapParams *mipmap_params) const
{
	float coord_1 = p[Axis::X];
	float coord_2 = p[Axis::Y];

	if(params_.use_flip_axis_)
	{
		coord_1 = p[Axis::Y];
		coord_2 = p[Axis::X];
	}

	float blend;
	switch(params_.blend_type_.value())
	{
		case BlendType::Quadratic:
			// Transform -1..1 to 0..1
			blend = 0.5f * (coord_1 + 1.f);
			if(blend < 0.f) blend = 0.f;
			else blend *= blend;
			break;
		case BlendType::Easing:
			blend = 0.5f * (coord_1 + 1.f);
			if(blend <= 0.f) blend = 0.f;
			else if(blend >= 1.f) blend = 1.f;
			else blend = (3.f * blend * blend - 2.f * blend * blend * blend);
			break;
		case BlendType::Diagonal:
			blend = 0.25f * (2.f + coord_1 + coord_2);
			break;
		case BlendType::Spherical:
		case BlendType::QuadraticSphere:
			blend = 1.f - math::sqrt(coord_1 * coord_1 + coord_2 * coord_2 + p[Axis::Z] * p[Axis::Z]);
			if(blend < 0.f) blend = 0.f;
			if(params_.blend_type_ == BlendType::QuadraticSphere) blend *= blend;
			break;
		case BlendType::Radial:
			blend = (atan2f(coord_2, coord_1) / (2.f * math::num_pi<>) + 0.5f);
			break;
		case BlendType::Linear:
		default:
			// Transform -1..1 to 0..1
			blend = 0.5f * (coord_1 + 1.f);
			break;
	}
	// Clipping to 0..1
	blend = std::max(0.f, std::min(blend, 1.f));
	return applyIntensityContrastAdjustments(blend);
}

Rgba BlendTexture::getColor(const Point3f &p, const MipMapParams *mipmap_params) const
{
	if(!color_ramp_) return applyColorAdjustments(Rgba{Texture::getFloat(p)});
	else return applyColorAdjustments(color_ramp_->getColorInterpolated(Texture::getFloat(p)));
}

} //namespace yafaray
