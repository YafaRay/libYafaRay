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

#include "texture/texture_voronoi.h"
#include "param/param.h"
#include "scene/scene.h"

namespace yafaray {

std::map<std::string, const ParamMeta *> VoronoiTexture::Params::getParamMetaMap()
{
	auto param_meta_map{ParentClassType_t::Params::getParamMetaMap()};
	PARAM_META(distance_metric_);
	PARAM_META(color_mode_);
	PARAM_META(color_1_);
	PARAM_META(color_2_);
	PARAM_META(size_);
	PARAM_META(weight_1_);
	PARAM_META(weight_2_);
	PARAM_META(weight_3_);
	PARAM_META(weight_4_);
	PARAM_META(mk_exponent_);
	PARAM_META(intensity_);
	return param_meta_map;
}

VoronoiTexture::Params::Params(ParamResult &param_result, const ParamMap &param_map)
{
	PARAM_ENUM_LOAD(distance_metric_);
	PARAM_ENUM_LOAD(color_mode_);
	PARAM_LOAD(color_1_);
	PARAM_LOAD(color_2_);
	PARAM_LOAD(size_);
	PARAM_LOAD(weight_1_);
	PARAM_LOAD(weight_2_);
	PARAM_LOAD(weight_3_);
	PARAM_LOAD(weight_4_);
	PARAM_LOAD(mk_exponent_);
	PARAM_LOAD(intensity_);
}

ParamMap VoronoiTexture::getAsParamMap(bool only_non_default) const
{
	auto param_map{ParentClassType_t::getAsParamMap(only_non_default)};
	param_map.setParam("type", type().print());
	PARAM_ENUM_SAVE(distance_metric_);
	PARAM_ENUM_SAVE(color_mode_);
	PARAM_SAVE(color_1_);
	PARAM_SAVE(color_2_);
	PARAM_SAVE(size_);
	PARAM_SAVE(weight_1_);
	PARAM_SAVE(weight_2_);
	PARAM_SAVE(weight_3_);
	PARAM_SAVE(weight_4_);
	PARAM_SAVE(mk_exponent_);
	PARAM_SAVE(intensity_);
	return param_map;
}

std::pair<std::unique_ptr<Texture>, ParamResult> VoronoiTexture::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	auto param_result{class_meta::check<Params>(param_map, {"type"}, {"ramp_item_"})};
	auto texture {std::make_unique<VoronoiTexture>(logger, param_result, param_map, scene.getTextures())};
	if(param_result.notOk()) logger.logWarning(param_result.print<ThisClassType_t>(name, {"type"}));
	return {std::move(texture), param_result};
}

VoronoiTexture::VoronoiTexture(Logger &logger, ParamResult &param_result, const ParamMap &param_map, const Items <Texture> &textures) : ParentClassType_t{logger, param_result, param_map, textures}, params_{param_result, param_map}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + getAsParamMap(true).print());
	if(intensity_scale_ != 0.f) intensity_scale_ = params_.intensity_ / intensity_scale_;
}

float VoronoiTexture::getFloat(const Point3f &p, const MipMapParams *mipmap_params) const
{
	const auto [da, pa] = v_gen_.getFeatures(p * params_.size_);
	return applyIntensityContrastAdjustments(intensity_scale_ * std::abs(params_.weight_1_ * VoronoiNoiseGenerator::getDistance(0, da) + params_.weight_2_ * VoronoiNoiseGenerator::getDistance(1, da) + params_.weight_3_ * VoronoiNoiseGenerator::getDistance(2, da) + params_.weight_4_ * VoronoiNoiseGenerator::getDistance(3, da)));
}

Rgba VoronoiTexture::getColor(const Point3f &p, const MipMapParams *mipmap_params) const
{
	const auto [da, pa] = v_gen_.getFeatures(p * params_.size_);
	const float inte = intensity_scale_ * std::abs(params_.weight_1_ * VoronoiNoiseGenerator::getDistance(0, da) + params_.weight_2_ * VoronoiNoiseGenerator::getDistance(1, da) + params_.weight_3_ * VoronoiNoiseGenerator::getDistance(2, da) + params_.weight_4_ * VoronoiNoiseGenerator::getDistance(3, da));
	Rgba col(0.0);
	if(color_ramp_) return applyColorAdjustments(color_ramp_->getColorInterpolated(inte));
	else if(params_.color_mode_ != ColorMode::IntensityWithoutColor)
	{
		col += aw_1_ * NoiseGenerator::cellNoiseColor(VoronoiNoiseGenerator::getPoint(0, pa));
		col += aw_2_ * NoiseGenerator::cellNoiseColor(VoronoiNoiseGenerator::getPoint(1, pa));
		col += aw_3_ * NoiseGenerator::cellNoiseColor(VoronoiNoiseGenerator::getPoint(2, pa));
		col += aw_4_ * NoiseGenerator::cellNoiseColor(VoronoiNoiseGenerator::getPoint(3, pa));
		if(params_.color_mode_ == ColorMode::PositionOutline || params_.color_mode_ == ColorMode::PositionOutlineIntensity)
		{
			float t_1 = (VoronoiNoiseGenerator::getDistance(1, da) - VoronoiNoiseGenerator::getDistance(0, da)) * 10.f;
			if(t_1 > 1) t_1 = 1;
			if(params_.color_mode_ == ColorMode::PositionOutlineIntensity) t_1 *= inte;
			else t_1 *= intensity_scale_;
			col *= t_1;
		}
		else col *= intensity_scale_;
		return applyAdjustments(col);
	}
	else return applyColorAdjustments(Rgba(inte, inte, inte, inte));
}

} //namespace yafaray
