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

#include "volume/region/density/volume_grid.h"
#include "texture/texture.h"
#include "param/param.h"
#include <fstream>

namespace yafaray {

std::map<std::string, const ParamMeta *> GridVolumeRegion::Params::getParamMetaMap()
{
	auto param_meta_map{ParentClassType_t::Params::getParamMetaMap()};
	PARAM_META(density_file_);
	return param_meta_map;
}

GridVolumeRegion::Params::Params(ParamResult &param_result, const ParamMap &param_map)
{
	PARAM_LOAD(density_file_);
}

ParamMap GridVolumeRegion::getAsParamMap(bool only_non_default) const
{
	auto param_map{ParentClassType_t::getAsParamMap(only_non_default)};
	param_map.setParam("type", type().print());
	PARAM_SAVE(density_file_);
	return param_map;
}

std::pair<std::unique_ptr<VolumeRegion>, ParamResult> GridVolumeRegion::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	auto param_result{class_meta::check<Params>(param_map, {"type"}, {})};
	auto volume_region {std::make_unique<GridVolumeRegion>(logger, param_result, param_map)};
	if(param_result.notOk()) logger.logWarning(param_result.print<ThisClassType_t>(name, {"type"}));
	return {std::move(volume_region), param_result};
}

GridVolumeRegion::GridVolumeRegion(Logger &logger, ParamResult &param_result, const ParamMap &param_map) :
		ParentClassType_t{logger, param_result, param_map}, params_{param_result, param_map}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + getAsParamMap(true).print());

	std::ifstream input_stream;
	input_stream.open(params_.density_file_);
	if(!input_stream) logger_.logError("GridVolume: Error opening input stream for file '" + params_.density_file_ + "'");

	//It was not previously documented and even the file name used to be hardcoded, but I think this is using POVRay density_file *.df3 format. For more information about the POVRay density_file format refer to: https://www.povray.org/documentation/view/3.6.1/374/
	input_stream.seekg(0, std::ios_base::beg);
	std::ifstream::pos_type begin_pos = input_stream.tellg();
	input_stream.seekg(0, std::ios_base::end);
	int file_size = static_cast<int>(input_stream.tellg() - begin_pos);
	file_size -= 6;
	input_stream.seekg(0, std::ios_base::beg);

	int dim[3];
	for(int &dim_entry : dim)
	{
		short i_0 = 0, i_1 = 0;
		input_stream.read((char *)&i_0, 1);
		input_stream.read((char *)&i_1, 1);
		if(logger_.isVerbose()) logger_.logVerbose(getClassName() + ": ", i_0, " ", i_1);
		dim_entry = (((unsigned short)i_0 << 8) | (unsigned short)i_1);
	}

	int size_per_voxel = file_size / (dim[0] * dim[1] * dim[2]);

	if(logger_.isVerbose()) logger_.logVerbose(getClassName() + ": ", dim[0], " ", dim[1], " ", dim[2], " ", file_size, " ", size_per_voxel);

	size_x_ = dim[0];
	size_y_ = dim[1];
	size_z_ = dim[2];

	/*
	sizeX = 60;
	sizeY = 60;
	sizeZ = 60;
	*/

	//FIXME DAVID: replace the malloc/free pattern by a more modern C++ pattern!
	grid_ = (float ***)malloc(size_x_ * sizeof(float));
	for(int x = 0; x < size_x_; ++x)
	{
		grid_[x] = (float **)malloc(size_y_ * sizeof(float));
		for(int y = 0; y < size_y_; ++y)
		{
			grid_[x][y] = (float *)malloc(size_z_ * sizeof(float));
		}
	}

	for(int z = 0; z < size_z_; ++z)
	{
		for(int y = 0; y < size_y_; ++y)
		{
			for(int x = 0; x < size_x_; ++x)
			{
				int voxel = 0;
				input_stream.read((char *)&voxel, 1);
				grid_[x][y][z] = voxel / 255.f;
				/*
				float r = sizeX / 2.f;
				float r2 = r*r;
				float dist = sqrt((x-r)*(x-r) + (y-r)*(y-r) + (z-r)*(z-r));
				grid[x][y][z] = (dist < r) ? 1.f-dist/r : 0.0f;
				*/
			}
		}
	}
	if(logger_.isVerbose()) logger_.logVerbose(getClassName() + ": Vol.[", s_a_, ", ", s_s_, ", ", l_e_, "]");
}

float GridVolumeRegion::density(const Point3f &p) const
{
	const float x = (p[Axis::X] - b_box_.a_[Axis::X]) / b_box_.length(Axis::X) * size_x_ - .5f;
	const float y = (p[Axis::Y] - b_box_.a_[Axis::Y]) / b_box_.length(Axis::Y) * size_y_ - .5f;
	const float z = (p[Axis::Z] - b_box_.a_[Axis::Z]) / b_box_.length(Axis::Z) * size_z_ - .5f;

	const int x_0 = std::max(0, static_cast<int>(std::floor(x)));
	const int y_0 = std::max(0, static_cast<int>(std::floor(y)));
	const int z_0 = std::max(0, static_cast<int>(std::floor(z)));

	const int x_1 = std::min(size_x_ - 1, static_cast<int>(std::ceil(x)));
	const int y_1 = std::min(size_y_ - 1, static_cast<int>(std::ceil(y)));
	const int z_1 = std::min(size_z_ - 1, static_cast<int>(std::ceil(z)));

	const float xd = x - x_0;
	const float yd = y - y_0;
	const float zd = z - z_0;

	const float i_1 = grid_[x_0][y_0][z_0] * (1 - zd) + grid_[x_0][y_0][z_1] * zd;
	const float i_2 = grid_[x_0][y_1][z_0] * (1 - zd) + grid_[x_0][y_1][z_1] * zd;
	const float j_1 = grid_[x_1][y_0][z_0] * (1 - zd) + grid_[x_1][y_0][z_1] * zd;
	const float j_2 = grid_[x_1][y_1][z_0] * (1 - zd) + grid_[x_1][y_1][z_1] * zd;

	const float w_1 = i_1 * (1 - yd) + i_2 * yd;
	const float w_2 = j_1 * (1 - yd) + j_2 * yd;

	const float dens = w_1 * (1 - xd) + w_2 * xd;
	return dens;
}

GridVolumeRegion::~GridVolumeRegion() {
	if(logger_.isVerbose()) logger_.logVerbose("GridVolume: Freeing grid data");

	for(int x = 0; x < size_x_; ++x)
	{
		for(int y = 0; y < size_y_; ++y)
		{
			free(grid_[x][y]);
		}
		free(grid_[x]);
	}
	free(grid_);
}

} //namespace yafaray
