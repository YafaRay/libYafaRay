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

#include "volume/volume.h"
#include "volume/volume_uniform.h"
#include "volume/volume_sky.h"
#include "volume/volume_noise.h"
#include "volume/volume_grid.h"
#include "volume/volume_exp_density.h"
#include "volume/volumehandler_beer.h"
#include "volume/volumehandler_sss.h"
#include "common/logger.h"
#include "common/param.h"

BEGIN_YAFARAY

VolumeRegion * VolumeRegion::factory(Logger &logger, const ParamMap &params, const Scene &scene)
{
	if(logger.isDebug())
	{
		logger.logDebug("**VolumeRegion");
		params.logContents(logger);
	}
	std::string type;
	params.getParam("type", type);
	if(type == "ExpDensityVolume") return ExpDensityVolumeRegion::factory(logger, params, scene);
	else if(type == "GridVolume") return GridVolumeRegion::factory(logger, params, scene);
	else if(type == "NoiseVolume") return NoiseVolumeRegion::factory(logger, params, scene);
	else if(type == "SkyVolume") return SkyVolumeRegion::factory(logger, params, scene);
	else if(type == "UniformVolume") return UniformVolumeRegion::factory(logger, params, scene);
	else return nullptr;
}

VolumeHandler * VolumeHandler::factory(Logger &logger, const ParamMap &params, const Scene &scene)
{
	if(logger.isDebug())
	{
		logger.logDebug("**VolumeHandler");
		params.logContents(logger);
	}
	std::string type;
	params.getParam("type", type);
	if(type == "beer") return BeerVolumeHandler::factory(logger, params, scene);
	else if(type == "sss") return SssVolumeHandler::factory(logger, params, scene);
	else return nullptr;
}

VolumeRegion::VolumeRegion(Logger &logger, const Rgb &sa, const Rgb &ss, const Rgb &le, float gg, const Point3 &pmin, const Point3 &pmax, int attgrid_scale) : logger_(logger)
{
	b_box_ = Bound(pmin, pmax);
	s_a_ = sa;
	s_s_ = ss;
	l_e_ = le;
	g_ = gg;
	have_s_a_ = (s_a_.energy() > 1e-4f);
	have_s_s_ = (s_s_.energy() > 1e-4f);
	have_l_e_ = (l_e_.energy() > 1e-4f);
	att_grid_x_ = 8 * attgrid_scale;
	att_grid_y_ = 8 * attgrid_scale;
	att_grid_z_ = 8 * attgrid_scale;
}

Rgb DensityVolumeRegion::tau(const Ray &ray, float step_size, float offset) const
{
	Bound::Cross cross = crossBound(ray);
	// ray doesn't hit the BB
	if(!cross.crossed_) return Rgb{0.f};
	if(ray.tmax_ < cross.enter_ && ray.tmax_ >= 0.f) return Rgb{0.f};
	if(ray.tmax_ < cross.leave_ && ray.tmax_ >= 0.f) cross.leave_ = ray.tmax_;
	if(cross.enter_ < 0.f) cross.enter_ = 0.f;

	// distance travelled in the volume
	float step = step_size; // length between two sample points along the ray
	float pos = cross.enter_ + offset * step;
	Rgb tau_val(0.f);


	Rgb tau_prev(0.f);

	bool adaptive = false; //FIXME: unused, always false??

	while(pos < cross.leave_)
	{
		Rgb tau_tmp = sigmaT(ray.from_ + (ray.dir_ * pos), ray.dir_);


		if(adaptive) //FIXME: unused, always false??
		{

			float epsilon = 0.01f;

			if(std::abs(tau_tmp.energy() - tau_prev.energy()) > epsilon && step > step_size / 50.f)
			{
				pos -= step;
				step /= 2.f;
			}
			else
			{
				tau_val += tau_tmp * step;
				tau_prev = tau_tmp;
			}
		}
		else
		{
			tau_val += tau_tmp * step;
		}

		pos += step;
	}

	return tau_val;
}

float VolumeRegion::attenuation(const Point3 &p, const Light *l) const
{
	if(attenuation_grid_map_.find(l) == attenuation_grid_map_.end())
	{
		logger_.logWarning("VolumeRegion: Attenuation Map is missing");
	}

	const float *attenuation_grid = attenuation_grid_map_.at(l);

	const float x = (p.x_ - b_box_.a_.x_) / b_box_.longX() * att_grid_x_ - 0.5f;
	const float y = (p.y_ - b_box_.a_.y_) / b_box_.longY() * att_grid_y_ - 0.5f;
	const float z = (p.z_ - b_box_.a_.z_) / b_box_.longZ() * att_grid_z_ - 0.5f;

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

END_YAFARAY

