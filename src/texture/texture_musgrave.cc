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

#include "texture/texture_musgrave.h"
#include "noise/musgrave/musgrave_fbm.h"
#include "noise/musgrave/musgrave_hetero_terrain.h"
#include "noise/musgrave/musgrave_hybrid_mfractal.h"
#include "noise/musgrave/musgrave_mfractal.h"
#include "noise/musgrave/musgrave_ridged_mfractal.h"
#include "param/param.h"
#include "scene/scene.h"

namespace yafaray {

std::map<std::string, const ParamMeta *> MusgraveTexture::Params::getParamMetaMap()
{
	auto param_meta_map{ParentClassType_t::Params::getParamMetaMap()};
	PARAM_META(musgrave_type_);
	PARAM_META(noise_type_);
	PARAM_META(color_1_);
	PARAM_META(color_2_);
	PARAM_META(h_);
	PARAM_META(lacunarity_);
	PARAM_META(octaves_);
	PARAM_META(offset_);
	PARAM_META(gain_);
	PARAM_META(intensity_);
	PARAM_META(size_);
	return param_meta_map;
}

MusgraveTexture::Params::Params(ParamResult &param_result, const ParamMap &param_map)
{
	PARAM_ENUM_LOAD(musgrave_type_);
	PARAM_ENUM_LOAD(noise_type_);
	PARAM_LOAD(color_1_);
	PARAM_LOAD(color_2_);
	PARAM_LOAD(h_);
	PARAM_LOAD(lacunarity_);
	PARAM_LOAD(octaves_);
	PARAM_LOAD(offset_);
	PARAM_LOAD(gain_);
	PARAM_LOAD(intensity_);
	PARAM_LOAD(size_);
}

ParamMap MusgraveTexture::getAsParamMap(bool only_non_default) const
{
	auto param_map{ParentClassType_t::getAsParamMap(only_non_default)};
	param_map.setParam("type", type().print());
	PARAM_ENUM_SAVE(musgrave_type_);
	PARAM_ENUM_SAVE(noise_type_);
	PARAM_SAVE(color_1_);
	PARAM_SAVE(color_2_);
	PARAM_SAVE(h_);
	PARAM_SAVE(lacunarity_);
	PARAM_SAVE(octaves_);
	PARAM_SAVE(offset_);
	PARAM_SAVE(gain_);
	PARAM_SAVE(intensity_);
	PARAM_SAVE(size_);
	return param_map;
}

std::pair<std::unique_ptr<Texture>, ParamResult> MusgraveTexture::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	auto param_result{class_meta::check<Params>(param_map, {"type"}, {"ramp_item_"})};
	auto texture {std::make_unique<MusgraveTexture>(logger, param_result, param_map, scene.getTextures())};
	if(param_result.notOk()) logger.logWarning(param_result.print<ThisClassType_t>(name, {"type"}));
	return {std::move(texture), param_result};
}

MusgraveTexture::MusgraveTexture(Logger &logger, ParamResult &param_result, const ParamMap &param_map, const Items <Texture> &textures) : ParentClassType_t{logger, param_result, param_map, textures}, params_{param_result, param_map}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + getAsParamMap(true).print());
	switch(params_.musgrave_type_.value())
	{
		case MusgraveType::MultiFractal: m_gen_ = std::make_unique<MFractalMusgrave>(params_.h_, params_.lacunarity_, params_.octaves_, n_gen_.get()); break;
		case MusgraveType::HeteroTerrain: m_gen_ = std::make_unique<HeteroTerrainMusgrave>(params_.h_, params_.lacunarity_, params_.octaves_, params_.offset_, n_gen_.get()); break;
		case MusgraveType::HybridDmf: m_gen_ = std::make_unique<HybridMFractalMusgrave>(params_.h_, params_.lacunarity_, params_.octaves_, params_.offset_, params_.gain_, n_gen_.get()); break;
		case MusgraveType::RidgedDmf: m_gen_ = std::make_unique<RidgedMFractalMusgrave>(params_.h_, params_.lacunarity_, params_.octaves_, params_.offset_, params_.gain_, n_gen_.get()); break;
		case MusgraveType::Fbm:
		default: m_gen_ = std::make_unique<FBmMusgrave>(params_.h_, params_.lacunarity_, params_.octaves_, n_gen_.get()); break;
	}
}

float MusgraveTexture::getFloat(const Point3f &p, const MipMapParams *mipmap_params) const
{
	return applyIntensityContrastAdjustments(params_.intensity_ * (*m_gen_)(p * params_.size_));
}

Rgba MusgraveTexture::getColor(const Point3f &p, const MipMapParams *mipmap_params) const
{
	if(!color_ramp_) return applyColorAdjustments(Rgba{params_.color_1_} + Texture::getFloat(p) * Rgba{params_.color_2_ - params_.color_1_});
	else return applyColorAdjustments(color_ramp_->getColorInterpolated(Texture::getFloat(p)));
}

} //namespace yafaray
