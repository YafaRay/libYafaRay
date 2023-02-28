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

#ifndef LIBYAFARAY_VOLUME_EXP_DENSITY_H
#define LIBYAFARAY_VOLUME_EXP_DENSITY_H

#include "volume_region_density.h"

namespace yafaray {

class ExpDensityVolumeRegion final : public DensityVolumeRegion
{
		using ThisClassType_t = ExpDensityVolumeRegion; using ParentClassType_t = DensityVolumeRegion;

	public:
		inline static std::string getClassName() { return "ExpDensityVolumeRegion"; }
		static std::pair<std::unique_ptr<VolumeRegion>, ParamResult> factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map);
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return class_meta::print<Params>(excluded_params); }
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;
		ExpDensityVolumeRegion(Logger &logger, ParamResult &param_result, const ParamMap &param_map);

	private:
		[[nodiscard]] Type type() const override { return Type::ExpDensity; }
		const struct Params
		{
			Params(ParamResult &param_result, const ParamMap &param_map);
			static std::map<std::string, const ParamMeta *> getParamMetaMap();
			PARAM_DECL(float, a_, 1.f, "a", "");
			PARAM_DECL(float, b_, 1.f, "b", "");
		} params_;
		float density(const Point3f &p) const override;
};

} //namespace yafaray

#endif // LIBYAFARAY_VOLUME_EXP_DENSITY_H