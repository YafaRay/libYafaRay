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

#include "volume/volume_uniform.h"
#include "common/logger.h"
#include "geometry/surface.h"
#include "common/param.h"

BEGIN_YAFARAY

struct PSample;

Rgb UniformVolumeRegion::sigmaA(const Point3 &p, const Vec3 &v) const
{
	if(!have_s_a_) return Rgb(0.f);
	if(b_box_.includes(p))
	{
		return s_a_;
	}
	else
		return Rgb(0.f);

}

Rgb UniformVolumeRegion::sigmaS(const Point3 &p, const Vec3 &v) const
{
	if(!have_s_s_) return Rgb(0.f);
	if(b_box_.includes(p))
	{
		return s_s_;
	}
	else
		return Rgb(0.f);
}

Rgb UniformVolumeRegion::tau(const Ray &ray, float step, float offset) const
{
	Bound::Cross cross = crossBound(ray);
	if(!cross.crossed_) return {0.f};
	if(ray.tmax_ < cross.enter_ && ray.tmax_ >= 0) return Rgb(0.f);
	if(ray.tmax_ < cross.leave_ && ray.tmax_ >= 0) cross.leave_ = ray.tmax_;
	// t0 < 0 means, ray.from is in the volume
	if(cross.enter_ < 0.f) cross.enter_ = 0.f;
	// distance travelled in the volume
	float dist = cross.leave_ - cross.enter_;
	return dist * (s_s_ + s_a_);
}

Rgb UniformVolumeRegion::emission(const Point3 &p, const Vec3 &v) const
{
	if(!have_l_e_) return Rgb(0.f);
	if(b_box_.includes(p))
	{
		return l_e_;
	}
	else
		return Rgb(0.f);
}

std::unique_ptr<VolumeRegion> UniformVolumeRegion::factory(Logger &logger, const ParamMap &params, const Scene &scene)
{
	float ss = .1f;
	float sa = .1f;
	float le = .0f;
	float g = .0f;
	float min[] = {0, 0, 0};
	float max[] = {0, 0, 0};
	int att_sc = 5;

	params.getParam("sigma_s", ss);
	params.getParam("sigma_a", sa);
	params.getParam("l_e", le);
	params.getParam("g", g);
	params.getParam("minX", min[0]);
	params.getParam("minY", min[1]);
	params.getParam("minZ", min[2]);
	params.getParam("maxX", max[0]);
	params.getParam("maxY", max[1]);
	params.getParam("maxZ", max[2]);
	params.getParam("attgridScale", att_sc);

	return std::unique_ptr<VolumeRegion>(new UniformVolumeRegion(logger, Rgb(sa), Rgb(ss), Rgb(le), g, Point3(min[0], min[1], min[2]), Point3(max[0], max[1], max[2]), att_sc));
}

UniformVolumeRegion::UniformVolumeRegion(Logger &logger, Rgb sa, Rgb ss, Rgb le, float gg, Point3 pmin, Point3 pmax, int attgrid_scale) :
		VolumeRegion(logger, sa, ss, le, gg, pmin, pmax, attgrid_scale)
{
	if(logger_.isVerbose()) logger_.logVerbose("UniformVolume: Vol.[", s_a_, ", ", s_s_, ", ", l_e_, ", ", pmin, ", ", pmax, ", ", attgrid_scale, "]");
}

END_YAFARAY
