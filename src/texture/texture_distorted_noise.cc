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

namespace yafaray {

DistortedNoiseTexture::Params::Params(ParamError &param_error, const ParamMap &param_map)
{
	PARAM_ENUM_LOAD(noise_type_1_);
	PARAM_ENUM_LOAD(noise_type_2_);
	PARAM_LOAD(color_1_);
	PARAM_LOAD(color_2_);
	PARAM_LOAD(distort_);
	PARAM_LOAD(size_);
}

ParamMap DistortedNoiseTexture::Params::getAsParamMap(bool only_non_default) const
{
	PARAM_SAVE_START;
	PARAM_ENUM_SAVE(noise_type_1_);
	PARAM_ENUM_SAVE(noise_type_2_);
	PARAM_SAVE(color_1_);
	PARAM_SAVE(color_2_);
	PARAM_SAVE(distort_);
	PARAM_SAVE(size_);
	PARAM_SAVE_END;
}

ParamMap DistortedNoiseTexture::getAsParamMap(bool only_non_default) const
{
	ParamMap result{ParentClassType_t::getAsParamMap(only_non_default)};
	result.append(params_.getAsParamMap(only_non_default));
	return result;
}

std::pair<std::unique_ptr<Texture>, ParamError> DistortedNoiseTexture::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	auto param_error{Params::meta_.check(param_map, {"type"}, {"ramp_item_"})};
	auto texture {std::make_unique<ThisClassType_t>(logger, param_error, param_map)};
	if(param_error.notOk()) logger.logWarning(param_error.print<ThisClassType_t>(name, {"type"}));
	return {std::move(texture), param_error};
}

DistortedNoiseTexture::DistortedNoiseTexture(Logger &logger, ParamError &param_error, const ParamMap &param_map) : ParentClassType_t{logger, param_error, param_map}, params_{param_error, param_map}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + params_.getAsParamMap(true).print());
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
