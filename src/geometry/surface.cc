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

SurfacePoint SurfacePoint::blendSurfacePoints(SurfacePoint const &sp_1, SurfacePoint const &sp_2, float alpha)
{
	SurfacePoint result(sp_1);
	result.n_ = math::lerp(sp_1.n_, sp_2.n_, alpha);
	result.nu_ = math::lerp(sp_1.nu_, sp_2.nu_, alpha);
	result.nv_ = math::lerp(sp_1.nv_, sp_2.nv_, alpha);
	result.dp_du_ = math::lerp(sp_1.dp_du_, sp_2.dp_du_, alpha);
	result.dp_dv_ = math::lerp(sp_1.dp_dv_, sp_2.dp_dv_, alpha);
	result.ds_du_ = math::lerp(sp_1.ds_du_, sp_2.ds_du_, alpha);
	result.ds_dv_ = math::lerp(sp_1.ds_dv_, sp_2.ds_dv_, alpha);
	result.mat_data_ = nullptr;
	return result;
}

SpDifferentials::SpDifferentials(const SurfacePoint &sp, const RayDifferentials *ray_differentials): sp_(sp)
{
	if(ray_differentials)
	{
		// Estimate screen-space change in \pt and $(u,v)$
		// Compute auxiliary intersection points with plane
		const float d = -(sp_.n_ * sp_.p_);
		const Vec3 rxv(ray_differentials->xfrom_);
		const float tx = -((sp_.n_ * rxv) + d) / (sp_.n_ * ray_differentials->xdir_);
		const Point3 px = ray_differentials->xfrom_ + tx * ray_differentials->xdir_;
		const Vec3 ryv(ray_differentials->yfrom_);
		const float ty = -((sp_.n_ * ryv) + d) / (sp_.n_ * ray_differentials->ydir_);
		const Point3 py = ray_differentials->yfrom_ + ty * ray_differentials->ydir_;
		dp_dx_ = px - sp_.p_;
		dp_dy_ = py - sp_.p_;
	}
	else
	{
		//dudx = dvdx = 0.;
		//dudy = dvdy = 0.;
		dp_dx_ = dp_dy_ = {0, 0, 0};
	}
}

std::unique_ptr<RayDifferentials> SpDifferentials::reflectedRay(const RayDifferentials *in_differentials, const Vec3 &in_dir, const Vec3 &out_dir) const
{
	if(!in_differentials) return nullptr;
	auto out_differentials = std::unique_ptr<RayDifferentials>(new RayDifferentials());
	// Compute ray differential _rd_ for specular reflection
	out_differentials->xfrom_ = sp_.p_ + dp_dx_;
	out_differentials->yfrom_ = sp_.p_ + dp_dy_;
	// Compute differential reflected directions
	//	Normal dndx = bsdf->dgShading.dndu * bsdf->dgShading.dudx +
	//				  bsdf->dgShading.dndv * bsdf->dgShading.dvdx;
	//	Normal dndy = bsdf->dgShading.dndu * bsdf->dgShading.dudy +
	//				  bsdf->dgShading.dndv * bsdf->dgShading.dvdy;
	const Vec3 dwodx = in_dir - in_differentials->xdir_, dwody = in_dir - in_differentials->ydir_;
	const float d_d_ndx = (dwodx * sp_.n_); // + (out.dir * dndx);
	const float d_d_ndy = (dwody * sp_.n_); // + (out.dir * dndy);
	out_differentials->xdir_ = out_dir - dwodx + 2 * (/* (out.dir * sp.N) * dndx + */ d_d_ndx * sp_.n_);
	out_differentials->ydir_ = out_dir - dwody + 2 * (/* (out.dir * sp.N) * dndy + */ d_d_ndy * sp_.n_);
	return out_differentials;
}

std::unique_ptr<RayDifferentials> SpDifferentials::refractedRay(const RayDifferentials *in_differentials, const Vec3 &in_dir, const Vec3 &out_dir, float ior) const
{
	if(!in_differentials) return nullptr;
	auto out_differentials = std::unique_ptr<RayDifferentials>(new RayDifferentials());
	//RayDifferential rd(p, wi);
	out_differentials->xfrom_ = sp_.p_ + dp_dx_;
	out_differentials->yfrom_ = sp_.p_ + dp_dy_;
	//if (Dot(wo, n) < 0) eta = 1.f / eta;
	//Normal dndx = bsdf->dgShading.dndu * bsdf->dgShading.dudx + bsdf->dgShading.dndv * bsdf->dgShading.dvdx;
	//Normal dndy = bsdf->dgShading.dndu * bsdf->dgShading.dudy + bsdf->dgShading.dndv * bsdf->dgShading.dvdy;
	const Vec3 dwodx = in_dir - in_differentials->xdir_, dwody = in_dir - in_differentials->ydir_;
	const float d_d_ndx = (dwodx * sp_.n_); // + Dot(wo, dndx);
	const float d_d_ndy = (dwody * sp_.n_); // + Dot(wo, dndy);
	//	float mu = IOR * (in.dir * sp.N) - (out.dir * sp.N);
	const float dmudx = (ior - (ior * ior * (in_dir * sp_.n_)) / (out_dir * sp_.n_)) * d_d_ndx;
	const float dmudy = (ior - (ior * ior * (in_dir * sp_.n_)) / (out_dir * sp_.n_)) * d_d_ndy;
	out_differentials->xdir_ = out_dir + ior * dwodx - (/* mu * dndx + */ dmudx * sp_.n_);
	out_differentials->ydir_ = out_dir + ior * dwody - (/* mu * dndy + */ dmudy * sp_.n_);
	return out_differentials;
}

float SpDifferentials::projectedPixelArea()
{
	return (dp_dx_ ^ dp_dy_).length();
}

void SpDifferentials::dUdvFromDpdPdUdPdV(float &du, float &dv, const Point3 &dp, const Vec3 &dp_du, const Vec3 &dp_dv) const
{
	const float det_xy = (dp_du.x_ * dp_dv.y_) - (dp_dv.x_ * dp_du.y_);
	const float det_xz = (dp_du.x_ * dp_dv.z_) - (dp_dv.x_ * dp_du.z_);
	const float det_yz = (dp_du.y_ * dp_dv.z_) - (dp_dv.y_ * dp_du.z_);

	if(std::abs(det_xy) > 0.f && std::abs(det_xy) > std::abs(det_xz) && std::abs(det_xy) > std::abs(det_yz))
	{
		du = ((dp.x_ * dp_dv.y_) - (dp_dv.x_ * dp.y_)) / det_xy;
		dv = ((dp_du.x_ * dp.y_) - (dp.x_ * dp_du.y_)) / det_xy;
	}

	else if(std::abs(det_xz) > 0.f && std::abs(det_xz) > std::abs(det_xy) && std::abs(det_xz) > std::abs(det_yz))
	{
		du = ((dp.x_ * dp_dv.z_) - (dp_dv.x_ * dp.z_)) / det_xz;
		dv = ((dp_du.x_ * dp.z_) - (dp.x_ * dp_du.z_)) / det_xz;
	}

	else if(std::abs(det_yz) > 0.f && std::abs(det_yz) > std::abs(det_xy) && std::abs(det_yz) > std::abs(det_xz))
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
