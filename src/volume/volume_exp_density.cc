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

#include "volume/volume_exp_density.h"
#include "common/logger.h"
#include "geometry/surface.h"
#include "common/param.h"

BEGIN_YAFARAY

struct PSample;

float ExpDensityVolumeRegion::density(Point3 p) const
{
	float height = p.z_ - b_box_.a_.z_;
	return a_ * math::exp(-b_ * height);
}

std::unique_ptr<VolumeRegion> ExpDensityVolumeRegion::factory(Logger &logger, const ParamMap &params, const Scene &scene)
{
	float ss = .1f;
	float sa = .1f;
	float le = .0f;
	float g = .0f;
	float a = 1.f;
	float b = 1.f;
	float min[] = {0, 0, 0};
	float max[] = {0, 0, 0};
	int att_sc = 1;

	params.getParam("sigma_s", ss);
	params.getParam("sigma_a", sa);
	params.getParam("l_e", le);
	params.getParam("g", g);
	params.getParam("a", a);
	params.getParam("b", b);
	params.getParam("minX", min[0]);
	params.getParam("minY", min[1]);
	params.getParam("minZ", min[2]);
	params.getParam("maxX", max[0]);
	params.getParam("maxY", max[1]);
	params.getParam("maxZ", max[2]);
	params.getParam("attgridScale", att_sc);

	return std::unique_ptr<VolumeRegion>(new ExpDensityVolumeRegion(logger, Rgb(sa), Rgb(ss), Rgb(le), g, Point3(min[0], min[1], min[2]), Point3(max[0], max[1], max[2]), att_sc, a, b));
}

ExpDensityVolumeRegion::ExpDensityVolumeRegion(Logger &logger, Rgb sa, Rgb ss, Rgb le, float gg, Point3 pmin, Point3 pmax, int attgrid_scale, float aa, float bb) :
		DensityVolumeRegion(logger, sa, ss, le, gg, pmin, pmax, attgrid_scale)
{
	a_ = aa;
	b_ = bb;
	if(logger_.isVerbose()) logger_.logVerbose("ExpDensityVolume vol: ", s_a_, " ", s_s_, " ", l_e_, " ", a_, " ", b_);
}

END_YAFARAY
