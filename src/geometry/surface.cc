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

#include "geometry/surface.h"
#include "geometry/ray.h"
#include "math/interpolation.h"

BEGIN_YAFARAY

SurfacePoint SurfacePoint::blendSurfacePoints(SurfacePoint const &sp_0, SurfacePoint const &sp_1, float alpha)
{
	SurfacePoint sp_result(sp_0);

	sp_result.n_ = math::lerp(sp_0.n_, sp_1.n_, alpha);

	sp_result.nu_ = math::lerp(sp_0.nu_, sp_1.nu_, alpha);
	sp_result.nv_ = math::lerp(sp_0.nv_, sp_1.nv_, alpha);
	sp_result.dp_du_ = math::lerp(sp_0.dp_du_, sp_1.dp_du_, alpha);
	sp_result.dp_dv_ = math::lerp(sp_0.dp_dv_, sp_1.dp_dv_, alpha);
	sp_result.ds_du_ = math::lerp(sp_0.ds_du_, sp_1.ds_du_, alpha);
	sp_result.ds_dv_ = math::lerp(sp_0.ds_dv_, sp_1.ds_dv_, alpha);

	return sp_result;
}

SpDifferentials::SpDifferentials(const SurfacePoint &spoint, const DiffRay &ray): sp_(spoint)
{
	if(ray.has_differentials_)
	{
		// Estimate screen-space change in \pt and $(u,v)$
		// Compute auxiliary intersection points with plane
		float d = -(sp_.n_ * Vec3(sp_.p_));
		Vec3 rxv(ray.xfrom_);
		float tx = -((sp_.n_ * rxv) + d) / (sp_.n_ * ray.xdir_);
		Point3 px = ray.xfrom_ + tx * ray.xdir_;
		Vec3 ryv(ray.yfrom_);
		float ty = -((sp_.n_ * ryv) + d) / (sp_.n_ * ray.ydir_);
		Point3 py = ray.yfrom_ + ty * ray.ydir_;
		dp_dx_ = px - sp_.p_;
		dp_dy_ = py - sp_.p_;
	}
	else
	{
		//dudx = dvdx = 0.;
		//dudy = dvdy = 0.;
		dp_dx_ = dp_dy_ = Vec3(0, 0, 0);
	}
}

void SpDifferentials::reflectedRay(const DiffRay &in, DiffRay &out) const
{
	if(!in.has_differentials_)
	{
		out.has_differentials_ = false;
		return;
	}
	// Compute ray differential _rd_ for specular reflection
	out.has_differentials_ = true;
	out.xfrom_ = sp_.p_ + dp_dx_;
	out.yfrom_ = sp_.p_ + dp_dy_;
	// Compute differential reflected directions
	//	Normal dndx = bsdf->dgShading.dndu * bsdf->dgShading.dudx +
	//				  bsdf->dgShading.dndv * bsdf->dgShading.dvdx;
	//	Normal dndy = bsdf->dgShading.dndu * bsdf->dgShading.dudy +
	//				  bsdf->dgShading.dndv * bsdf->dgShading.dvdy;
	Vec3 dwodx = in.dir_ - in.xdir_, dwody = in.dir_ - in.ydir_;
	float d_d_ndx = (dwodx * sp_.n_); // + (out.dir * dndx);
	float d_d_ndy = (dwody * sp_.n_); // + (out.dir * dndy);
	out.xdir_ = out.dir_ - dwodx + 2 * (/* (out.dir * sp.N) * dndx + */ d_d_ndx * sp_.n_);
	out.ydir_ = out.dir_ - dwody + 2 * (/* (out.dir * sp.N) * dndy + */ d_d_ndy * sp_.n_);
}

void SpDifferentials::refractedRay(const DiffRay &in, DiffRay &out, float ior) const
{
	if(!in.has_differentials_)
	{
		out.has_differentials_ = false;
		return;
	}
	//RayDifferential rd(p, wi);
	out.has_differentials_ = true;
	out.xfrom_ = sp_.p_ + dp_dx_;
	out.yfrom_ = sp_.p_ + dp_dy_;
	//if (Dot(wo, n) < 0) eta = 1.f / eta;

	//Normal dndx = bsdf->dgShading.dndu * bsdf->dgShading.dudx + bsdf->dgShading.dndv * bsdf->dgShading.dvdx;
	//Normal dndy = bsdf->dgShading.dndu * bsdf->dgShading.dudy + bsdf->dgShading.dndv * bsdf->dgShading.dvdy;

	Vec3 dwodx = in.dir_ - in.xdir_, dwody = in.dir_ - in.ydir_;
	float d_d_ndx = (dwodx * sp_.n_); // + Dot(wo, dndx);
	float d_d_ndy = (dwody * sp_.n_); // + Dot(wo, dndy);

	//	float mu = IOR * (in.dir * sp.N) - (out.dir * sp.N);
	float dmudx = (ior - (ior * ior * (in.dir_ * sp_.n_)) / (out.dir_ * sp_.n_)) * d_d_ndx;
	float dmudy = (ior - (ior * ior * (in.dir_ * sp_.n_)) / (out.dir_ * sp_.n_)) * d_d_ndy;

	out.xdir_ = out.dir_ + ior * dwodx - (/* mu * dndx + */ dmudx * sp_.n_);
	out.ydir_ = out.dir_ + ior * dwody - (/* mu * dndy + */ dmudy * sp_.n_);
}

float SpDifferentials::projectedPixelArea()
{
	return (dp_dx_ ^ dp_dy_).length();
}

void SpDifferentials::dUdvFromDpdPdUdPdV(float &du, float &dv, const Point3 &dp, const Vec3 &dp_du, const Vec3 &dp_dv) const
{
	float det_xy = (dp_du.x_ * dp_dv.y_) - (dp_dv.x_ * dp_du.y_);
	float det_xz = (dp_du.x_ * dp_dv.z_) - (dp_dv.x_ * dp_du.z_);
	float det_yz = (dp_du.y_ * dp_dv.z_) - (dp_dv.y_ * dp_du.z_);

	if(fabsf(det_xy) > 0.f && fabsf(det_xy) > fabsf(det_xz) && fabsf(det_xy) > fabsf(det_yz))
	{
		du = ((dp.x_ * dp_dv.y_) - (dp_dv.x_ * dp.y_)) / det_xy;
		dv = ((dp_du.x_ * dp.y_) - (dp.x_ * dp_du.y_)) / det_xy;
	}

	else if(fabsf(det_xz) > 0.f && fabsf(det_xz) > fabsf(det_xy) && fabsf(det_xz) > fabsf(det_yz))
	{
		du = ((dp.x_ * dp_dv.z_) - (dp_dv.x_ * dp.z_)) / det_xz;
		dv = ((dp_du.x_ * dp.z_) - (dp.x_ * dp_du.z_)) / det_xz;
	}

	else if(fabsf(det_yz) > 0.f && fabsf(det_yz) > fabsf(det_xy) && fabsf(det_yz) > fabsf(det_xz))
	{
		du = ((dp.y_ * dp_dv.z_) - (dp_dv.y_ * dp.z_)) / det_yz;
		dv = ((dp_du.y_ * dp.z_) - (dp.y_ * dp_du.z_)) / det_yz;
	}
}

void SpDifferentials::getUVdifferentials(float &du_dx, float &dv_dx, float &du_dy, float &dv_dy) const
{
	dUdvFromDpdPdUdPdV(du_dx, dv_dx, dp_dx_, sp_.dp_du_abs_, sp_.dp_dv_abs_);
	dUdvFromDpdPdUdPdV(du_dy, dv_dy, dp_dy_, sp_.dp_du_abs_, sp_.dp_dv_abs_);
}

END_YAFARAY
