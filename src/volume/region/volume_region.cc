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

#include "volume/region/volume_uniform.h"
#include "volume/region/volume_sky.h"
#include "volume/region/density/volume_noise.h"
#include "volume/region/density/volume_grid.h"
#include "volume/region/density/volume_exp_density.h"
#include "common/logger.h"
#include "param/param.h"

namespace yafaray {

std::map<std::string, const ParamMeta *> VolumeRegion::Params::getParamMetaMap()
{
	std::map<std::string, const ParamMeta *> param_meta_map;
	PARAM_META(sigma_s_);
	PARAM_META(sigma_a_);
	PARAM_META(l_e_);
	PARAM_META(g_);
	PARAM_META(min_x_);
	PARAM_META(min_y_);
	PARAM_META(min_z_);
	PARAM_META(max_x_);
	PARAM_META(max_y_);
	PARAM_META(max_z_);
	PARAM_META(att_grid_scale_);
	return param_meta_map;
}

VolumeRegion::Params::Params(ParamResult &param_result, const ParamMap &param_map)
{
	PARAM_LOAD(sigma_s_);
	PARAM_LOAD(sigma_a_);
	PARAM_LOAD(l_e_);
	PARAM_LOAD(g_);
	PARAM_LOAD(min_x_);
	PARAM_LOAD(min_y_);
	PARAM_LOAD(min_z_);
	PARAM_LOAD(max_x_);
	PARAM_LOAD(max_y_);
	PARAM_LOAD(max_z_);
	PARAM_LOAD(att_grid_scale_);
}

ParamMap VolumeRegion::getAsParamMap(bool only_non_default) const
{
	ParamMap param_map;
	PARAM_SAVE(sigma_s_);
	PARAM_SAVE(sigma_a_);
	PARAM_SAVE(l_e_);
	PARAM_SAVE(g_);
	PARAM_SAVE(min_x_);
	PARAM_SAVE(min_y_);
	PARAM_SAVE(min_z_);
	PARAM_SAVE(max_x_);
	PARAM_SAVE(max_y_);
	PARAM_SAVE(max_z_);
	PARAM_SAVE(att_grid_scale_);
	return param_map;
}

std::pair<std::unique_ptr<VolumeRegion>, ParamResult> VolumeRegion::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	if(logger.isDebug()) logger.logDebug("** " + getClassName() + "::factory 'raw' ParamMap contents:\n" + param_map.logContents());
	const auto type{class_meta::getTypeFromParamMap<Type>(logger, getClassName(), param_map)};
	switch(type.value())
	{
		case Type::ExpDensity: return ExpDensityVolumeRegion::factory(logger, scene, name, param_map);
		case Type::Grid: return GridVolumeRegion::factory(logger, scene, name, param_map);
		case Type::Noise: return NoiseVolumeRegion::factory(logger, scene, name, param_map);
		case Type::Sky: return SkyVolumeRegion::factory(logger, scene, name, param_map);
		case Type::Uniform: return UniformVolumeRegion::factory(logger, scene, name, param_map);
		default: return {nullptr, ParamResult{YAFARAY_RESULT_ERROR_WHILE_CREATING}};
	}
}

VolumeRegion::VolumeRegion(Logger &logger, ParamResult &param_result, const ParamMap &param_map) :
		params_{param_result, param_map}, logger_{logger}
{
}

float VolumeRegion::attenuation(const Point3f &p, const Light *l) const
{
	if(attenuation_grid_map_.find(l) == attenuation_grid_map_.end())
	{
		logger_.logWarning("VolumeRegion: Attenuation Map is missing");
	}

	const float *attenuation_grid = attenuation_grid_map_.at(l);

	const float x = (p[Axis::X] - b_box_.a_[Axis::X]) / b_box_.length(Axis::X) * att_grid_x_ - 0.5f;
	const float y = (p[Axis::Y] - b_box_.a_[Axis::Y]) / b_box_.length(Axis::Y) * att_grid_y_ - 0.5f;
	const float z = (p[Axis::Z] - b_box_.a_[Axis::Z]) / b_box_.length(Axis::Z) * att_grid_z_ - 0.5f;

	//Check that the point is within the bounding box, return 0 if outside the box
	if(x < -0.5f || y < -0.5f || z < -0.5f) return 0.f;
	else if(x > (att_grid_x_ - 0.5f) || y > (att_grid_y_ - 0.5f) || z > (att_grid_z_ - 0.5f)) return 0.f;

	// cell vertices in which p lies
	const int x_0 = std::max(0.f, floorf(x));
	const int y_0 = std::max(0.f, floorf(y));
	const int z_0 = std::max(0.f, floorf(z));

	const int x_1 = std::min(att_grid_x_ - 1.f, ceilf(x));
	const int y_1 = std::min(att_grid_y_ - 1.f, ceilf(y));
	const int z_1 = std::min(att_grid_z_ - 1.f, ceilf(z));

	// offsets
	const float xd = x - x_0;
	const float yd = y - y_0;
	const float zd = z - z_0;

	// trilinear combination
	const float i_1 = attenuation_grid[x_0 + y_0 * att_grid_x_ + att_grid_y_ * att_grid_x_ * z_0] * (1 - zd) + attenuation_grid[x_0 + y_0 * att_grid_x_ + att_grid_y_ * att_grid_x_ * z_1] * zd;
	const float i_2 = attenuation_grid[x_0 + y_1 * att_grid_x_ + att_grid_y_ * att_grid_x_ * z_0] * (1 - zd) + attenuation_grid[x_0 + y_1 * att_grid_x_ + att_grid_y_ * att_grid_x_ * z_1] * zd;
	const float j_1 = attenuation_grid[x_1 + y_0 * att_grid_x_ + att_grid_y_ * att_grid_x_ * z_0] * (1 - zd) + attenuation_grid[x_1 + y_0 * att_grid_x_ + att_grid_y_ * att_grid_x_ * z_1] * zd;
	const float j_2 = attenuation_grid[x_1 + y_1 * att_grid_x_ + att_grid_y_ * att_grid_x_ * z_0] * (1 - zd) + attenuation_grid[x_1 + y_1 * att_grid_x_ + att_grid_y_ * att_grid_x_ * z_1] * zd;

	const float w_1 = i_1 * (1 - yd) + i_2 * yd;
	const float w_2 = j_1 * (1 - yd) + j_2 * yd;

	const float att = w_1 * (1 - xd) + w_2 * xd;

	return att;
}

} //namespace yafaray

