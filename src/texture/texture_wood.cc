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

#include "texture/texture_wood.h"
#include "param/param.h"
#include "scene/scene.h"

namespace yafaray {

std::map<std::string, const ParamMeta *> WoodTexture::Params::getParamMetaMap()
{
	auto param_meta_map{ParentClassType_t::Params::getParamMetaMap()};
	PARAM_META(wood_type_);
	PARAM_META(shape_);
	PARAM_META(noise_type_);
	PARAM_META(color_1_);
	PARAM_META(color_2_);
	PARAM_META(octaves_);
	PARAM_META(turbulence_);
	PARAM_META(size_);
	PARAM_META(hard_);
	return param_meta_map;
}

WoodTexture::Params::Params(ParamResult &param_result, const ParamMap &param_map)
{
	PARAM_ENUM_LOAD(wood_type_);
	PARAM_ENUM_LOAD(shape_);
	PARAM_ENUM_LOAD(noise_type_);
	PARAM_LOAD(color_1_);
	PARAM_LOAD(color_2_);
	PARAM_LOAD(octaves_);
	PARAM_LOAD(turbulence_);
	PARAM_LOAD(size_);
	PARAM_LOAD(hard_);
}

ParamMap WoodTexture::getAsParamMap(bool only_non_default) const
{
	auto param_map{ParentClassType_t::getAsParamMap(only_non_default)};
	param_map.setParam("type", type().print());
	PARAM_ENUM_SAVE(wood_type_);
	PARAM_ENUM_SAVE(shape_);
	PARAM_ENUM_SAVE(noise_type_);
	PARAM_SAVE(color_1_);
	PARAM_SAVE(color_2_);
	PARAM_SAVE(octaves_);
	PARAM_SAVE(turbulence_);
	PARAM_SAVE(size_);
	PARAM_SAVE(hard_);
	return param_map;
}

std::pair<std::unique_ptr<Texture>, ParamResult> WoodTexture::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	auto param_result{class_meta::check<Params>(param_map, {"type"}, {"ramp_item_"})};
	auto texture {std::make_unique<WoodTexture>(logger, param_result, param_map, scene.getTextures())};
	if(param_result.notOk()) logger.logWarning(param_result.print<ThisClassType_t>(name, {"type"}));
	return {std::move(texture), param_result};
}

WoodTexture::WoodTexture(Logger &logger, ParamResult &param_result, const ParamMap &param_map, const Items <Texture> &textures) : ParentClassType_t{logger, param_result, param_map, textures}, params_{param_result, param_map}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + getAsParamMap(true).print());
}

float WoodTexture::getFloat(const Point3f &p, const MipMapParams *mipmap_params) const
{
	float w;
	if(params_.wood_type_ == WoodType::Rings)
		w = math::sqrt(p[Axis::X] * p[Axis::X] + p[Axis::Y] * p[Axis::Y] + p[Axis::Z] * p[Axis::Z]) * 20.f;
	else
		w = (p[Axis::X] + p[Axis::Y] + p[Axis::Z]) * 10.f;
	w += (params_.turbulence_ == 0.f) ? 0.f : params_.turbulence_ * NoiseGenerator::turbulence(n_gen_.get(), p, params_.octaves_, params_.size_, params_.hard_);
	switch(params_.shape_.value())
	{
		case Shape::Saw:
			w *= 0.5f * math::div_1_by_pi<>;
			w -= std::floor(w);
			break;
		case Shape::Tri:
			w *= 0.5f * math::div_1_by_pi<>;
			w = std::abs(2.f * (w - std::floor(w)) - 1.f);
			break;
		case Shape::Sin:
		default:
			w = 0.5f + 0.5f * math::sin(w);
	}
	return applyIntensityContrastAdjustments(w);
}

Rgba WoodTexture::getColor(const Point3f &p, const MipMapParams *mipmap_params) const
{
	if(!color_ramp_) return applyColorAdjustments(Rgba{params_.color_1_} + Texture::getFloat(p) * Rgba{params_.color_2_ - params_.color_1_});
	else return applyColorAdjustments(color_ramp_->getColorInterpolated(Texture::getFloat(p)));
}

} //namespace yafaray
