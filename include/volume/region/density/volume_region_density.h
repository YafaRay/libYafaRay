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

#ifndef LIBYAFARAY_VOLUME_REGION_DENSITY_H
#define LIBYAFARAY_VOLUME_REGION_DENSITY_H

#include "volume/region/volume_region.h"

namespace yafaray {

class DensityVolumeRegion : public VolumeRegion
{
		using ThisClassType_t = DensityVolumeRegion; using ParentClassType_t = VolumeRegion;

	protected:
		DensityVolumeRegion(Logger &logger, ParamError &param_error, const ParamMap &param_map);
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return Params::meta_.print(excluded_params); }
		const struct Params
		{
			PARAM_INIT_PARENT(ParentClassType_t);
		} params_;
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;

		virtual float density(const Point3f &p) const = 0;

		Rgb tau(const Ray &ray, float step_size, float offset) const override;

		Rgb sigmaA(const Point3f &p, const Vec3f &v) const override
		{
			if(!have_s_a_) return Rgb{0.f};
			if(b_box_.includes(p))
			{
				return s_a_ * density(p);
			}
			else
				return Rgb{0.f};

		}

		Rgb sigmaS(const Point3f &p, const Vec3f &v) const override
		{
			if(!have_s_s_) return Rgb{0.f};
			if(b_box_.includes(p))
			{
				return s_s_ * density(p);
			}
			else
				return Rgb{0.f};
		}

		Rgb emission(const Point3f &p, const Vec3f &v) const override
		{
			if(!have_l_e_) return Rgb{0.f};
			if(b_box_.includes(p))
			{
				return l_e_ * density(p);
			}
			else
				return Rgb{0.f};
		}

};

} //namespace yafaray

#endif // LIBYAFARAY_VOLUME_REGION_DENSITY_H
