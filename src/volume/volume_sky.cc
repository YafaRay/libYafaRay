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

#include "volume/volume_sky.h"
#include "common/ray.h"
#include "common/color.h"
#include "common/bound.h"
#include "common/surface.h"
#include "texture/texture.h"
#include "common/param.h"
#include "utility/util_mcqmc.h"

BEGIN_YAFARAY

struct RenderState;
struct PSample;

Rgb SkyVolumeRegion::sigmaA(const Point3 &p, const Vec3 &v) const
{
	return Rgb(0.f);
}

Rgb SkyVolumeRegion::sigmaS(const Point3 &p, const Vec3 &v) const
{
	//if (bBox.includes(p)) {
	return s_ray_ + s_mie_;
	//}
	//else
	//	return Rgb(0.f);
}

Rgb SkyVolumeRegion::tau(const Ray &ray, float step, float offset) const
{
	float t_0 = -1, t_1 = -1;

	// ray doesn't hit the BB
	if(!intersect(ray, t_0, t_1))
	{
		return Rgb(0.f);
	}

	if(ray.tmax_ < t_0 && ray.tmax_ >= 0) return Rgb(0.f);

	if(ray.tmax_ < t_1 && ray.tmax_ >= 0) t_1 = ray.tmax_;

	// t0 < 0 means, ray.from is in the volume
	if(t_0 < 0.f) t_0 = 0.f;

	// distance travelled in the volume
	float dist = t_1 - t_0;

	return (s_ray_ + s_mie_) * dist;
}

Rgb SkyVolumeRegion::emission(const Point3 &p, const Vec3 &v) const
{
	if(b_box_.includes(p))
	{
		return l_e_;
	}
	else
		return Rgb(0.f);
}

float SkyVolumeRegion::p(const Vec3 &w_l, const Vec3 &w_s) const
{
	return phaseRayleigh(w_l, w_s) + phaseMie(w_l, w_s);
}

float SkyVolumeRegion::phaseRayleigh(const Vec3 &w_l, const Vec3 &w_s) const
{
	float costheta = (w_l * w_s);
	return 3.f / (16.f * M_PI) * (1.f + costheta * costheta) * s_ray_.energy();
}

float SkyVolumeRegion::phaseMie(const Vec3 &w_l, const Vec3 &w_s) const
{
	float k = 1.55f * g_ - .55f * g_ * g_ * g_;
	float kcostheta = k * (w_l * w_s);
	return 1.f / (4.f * M_PI) * (1.f - k * k) / ((1.f - kcostheta) * (1.f - kcostheta)) * s_mie_.energy();
}


VolumeRegion *SkyVolumeRegion::factory(const ParamMap &params, Scene &scene)
{
	float ss = .1f;
	float sa = .1f;
	float le = .0f;
	float g = .0f;
	float min[] = {0, 0, 0};
	float max[] = {0, 0, 0};
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

	SkyVolumeRegion *vol = new SkyVolumeRegion(Rgb(sa), Rgb(ss), Rgb(le),
											   Point3(min[0], min[1], min[2]), Point3(max[0], max[1], max[2]));
	return vol;
}

SkyVolumeRegion::SkyVolumeRegion(Rgb sa, Rgb ss, Rgb le, Point3 pmin, Point3 pmax) {
	b_box_ = Bound(pmin, pmax);
	s_a_ = Rgb(0.f);
	s_ray_ = sa;
	s_ray_.b_ /= 3.f;
	s_mie_ = ss;
	s_s_ = Rgb(0.f);
	l_e_ = le;
	g_ = 0.f;
	Y_VERBOSE << "SkyVolume: Vol. [" << s_ray_ << ", " << s_mie_ << ", " << l_e_ << "]" << YENDL;
}

END_YAFARAY
