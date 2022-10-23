#pragma once
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

#ifndef LIBYAFARAY_VOLUME_GRID_H
#define LIBYAFARAY_VOLUME_GRID_H

#include "volume/region/density/volume_region_density.h"

namespace yafaray {

class GridVolumeRegion final : public DensityVolumeRegion
{
		using ThisClassType_t = GridVolumeRegion; using ParentClassType_t = DensityVolumeRegion;

	public:
		inline static std::string getClassName() { return "GridVolumeRegion"; }
		static std::pair<std::unique_ptr<VolumeRegion>, ParamResult> factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map);
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return Params::meta_.print(excluded_params); }
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;
		GridVolumeRegion(Logger &logger, ParamResult &param_result, const ParamMap &param_map);
		~GridVolumeRegion() override;

	private:
		[[nodiscard]] Type type() const override { return Type::Grid; }
		const struct Params
		{
			PARAM_INIT_PARENT(ParentClassType_t);
			PARAM_DECL(std::string, density_file_, "", "density_file", "Path to the *.df3 density file (in POVRay density_file format)"); //For more information about the POVRay density_file format refer to: https://www.povray.org/documentation/view/3.6.1/374/
			//FIXME DAVID: att_grid_scale_ not used
		} params_;
		float density(const Point3f &p) const override;

		float ***grid_ = nullptr;
		int size_x_{0}, size_y_{0}, size_z_{0};
};

} //namespace yafaray

#endif // LIBYAFARAY_VOLUME_GRID_H