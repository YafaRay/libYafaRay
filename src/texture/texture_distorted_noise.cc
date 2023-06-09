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

#include "texture/texture_distorted_noise.h"
#include "param/param.h"
#include "scene/scene.h"

namespace yafaray {

std::map<std::string, const ParamMeta *> DistortedNoiseTexture::Params::getParamMetaMap()
{
	auto param_meta_map{ParentClassType_t::Params::getParamMetaMap()};
	PARAM_META(noise_type_1_);
	PARAM_META(noise_type_2_);
	PARAM_META(color_1_);
	PARAM_META(color_2_);
	PARAM_META(distort_);
	PARAM_META(size_);
	return param_meta_map;
}

DistortedNoiseTexture::Params::Params(ParamResult &param_result, const ParamMap &param_map)
{
	PARAM_ENUM_LOAD(noise_type_1_);
	PARAM_ENUM_LOAD(noise_type_2_);
	PARAM_LOAD(color_1_);
	PARAM_LOAD(color_2_);
	PARAM_LOAD(distort_);
	PARAM_LOAD(size_);
}

ParamMap DistortedNoiseTexture::getAsParamMap(bool only_non_default) const
{
	auto param_map{ParentClassType_t::getAsParamMap(only_non_default)};
	param_map.setParam("type", type().print());
	PARAM_ENUM_SAVE(noise_type_1_);
	PARAM_ENUM_SAVE(noise_type_2_);
	PARAM_SAVE(color_1_);
	PARAM_SAVE(color_2_);
	PARAM_SAVE(distort_);
	PARAM_SAVE(size_);
	return param_map;
}

std::pair<std::unique_ptr<Texture>, ParamResult> DistortedNoiseTexture::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	auto param_result{class_meta::check<Params>(param_map, {"type"}, {"ramp_item_"})};
	auto texture {std::make_unique<DistortedNoiseTexture>(logger, param_result, param_map, scene.getTextures())};
	if(param_result.notOk()) logger.logWarning(param_result.print<ThisClassType_t>(name, {"type"}));
	return {std::move(texture), param_result};
}

DistortedNoiseTexture::DistortedNoiseTexture(Logger &logger, ParamResult &param_result, const ParamMap &param_map, const Items <Texture> &textures) : ParentClassType_t{logger, param_result, param_map, textures}, params_{param_result, param_map}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + getAsParamMap(true).print());
}

float DistortedNoiseTexture::getFloat(const Point3f &p, const MipMapParams *mipmap_params) const
{
	// get a random vector and scale the randomization
	const Point3f ofs{{13.5, 13.5, 13.5}};
	const Point3f tp(p * params_.size_);
	const Point3f rv{{NoiseGenerator::getSignedNoise(n_gen_1_.get(), tp + ofs), NoiseGenerator::getSignedNoise(n_gen_1_.get(), tp), NoiseGenerator::getSignedNoise(n_gen_1_.get(), static_cast<Point3f>(tp - ofs))}};
	return applyIntensityContrastAdjustments(NoiseGenerator::getSignedNoise(n_gen_2_.get(), tp + rv * params_.distort_));	// distorted-domain noise
}

Rgba DistortedNoiseTexture::getColor(const Point3f &p, const MipMapParams *mipmap_params) const
{
	if(!color_ramp_) return applyColorAdjustments(Rgba{params_.color_1_} + Texture::getFloat(p) * Rgba{params_.color_2_ - params_.color_1_});
	else return applyColorAdjustments(color_ramp_->getColorInterpolated(Texture::getFloat(p)));
}

} //namespace yafaray
