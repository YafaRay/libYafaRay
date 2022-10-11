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

namespace yafaray {

MusgraveTexture::Params::Params(ParamError &param_error, const ParamMap &param_map)
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

ParamMap MusgraveTexture::Params::getAsParamMap(bool only_non_default) const
{
	PARAM_SAVE_START;
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
	PARAM_SAVE_END;
}

ParamMap MusgraveTexture::getAsParamMap(bool only_non_default) const
{
	ParamMap result{Texture::getAsParamMap(only_non_default)};
	result.append(params_.getAsParamMap(only_non_default));
	return result;
}

std::pair<Texture *, ParamError> MusgraveTexture::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	auto param_error{Params::meta_.check(param_map, {"type"}, {"ramp_item_"})};
	auto result {new MusgraveTexture(logger, param_error, param_map)};
	if(param_error.notOk()) logger.logWarning(param_error.print<MusgraveTexture>(name, {"type"}));
	return {result, param_error};
}

MusgraveTexture::MusgraveTexture(Logger &logger, ParamError &param_error, const ParamMap &param_map) : Texture{logger, param_error, param_map}, params_{param_error, param_map}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + params_.getAsParamMap(true).print());
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
