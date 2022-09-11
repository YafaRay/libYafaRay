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

namespace yafaray {

float SurfacePoint::getDistToNearestEdge() const
{
	return primitive_->getDistToNearestEdge(uv_, dp_abs_);
}

std::unique_ptr<SurfaceDifferentials> SurfacePoint::calcSurfaceDifferentials(const RayDifferentials *ray_differentials) const
{
	if(ray_differentials)
	{
		// Estimate screen-space change in \pt and $(u,v)$
		// Compute auxiliary intersection points with plane
		const float d = -(n_ * p_);
		const Vec3f rxv{ray_differentials->xfrom_};
		const float tx = -((n_ * rxv) + d) / (n_ * ray_differentials->xdir_);
		const Point3f px{ray_differentials->xfrom_ + tx * ray_differentials->xdir_};
		const Vec3f ryv(ray_differentials->yfrom_);
		const float ty = -((n_ * ryv) + d) / (n_ * ray_differentials->ydir_);
		const Point3f py{ray_differentials->yfrom_ + ty * ray_differentials->ydir_};
		return std::make_unique<SurfaceDifferentials>(px - p_, py - p_);
	}
	else return nullptr;
}

std::unique_ptr<RayDifferentials> SurfacePoint::reflectedRay(const RayDifferentials *in_differentials, const Vec3f &in_dir, const Vec3f &out_dir) const
{
	if(!differentials_ || !in_differentials) return nullptr;
	auto out_differentials = std::make_unique<RayDifferentials>();
	// Compute ray differential _rd_ for specular reflection
	out_differentials->xfrom_ = p_ + differentials_->dp_dx_;
	out_differentials->yfrom_ = p_ + differentials_->dp_dy_;
	// Compute differential reflected directions
	//	Normal dndx = bsdf->dgShading.dndu * bsdf->dgShading.dudx +
	//				  bsdf->dgShading.dndv * bsdf->dgShading.dvdx;
	//	Normal dndy = bsdf->dgShading.dndu * bsdf->dgShading.dudy +
	//				  bsdf->dgShading.dndv * bsdf->dgShading.dvdy;
	const Vec3f dwodx{in_dir - in_differentials->xdir_};
	const Vec3f dwody{in_dir - in_differentials->ydir_};
	const float d_d_ndx = (dwodx * n_); // + (out.dir * dndx);
	const float d_d_ndy = (dwody * n_); // + (out.dir * dndy);
	out_differentials->xdir_ = out_dir - dwodx + 2.f * (/* (out.dir * sp.N) * dndx + */ d_d_ndx * n_);
	out_differentials->ydir_ = out_dir - dwody + 2.f * (/* (out.dir * sp.N) * dndy + */ d_d_ndy * n_);
	return out_differentials;
}

std::unique_ptr<RayDifferentials> SurfacePoint::refractedRay(const RayDifferentials *in_differentials, const Vec3f &in_dir, const Vec3f &out_dir, float ior) const
{
	if(!differentials_ || !in_differentials) return nullptr;
	auto out_differentials = std::make_unique<RayDifferentials>();
	//RayDifferential rd(p, wi);
	out_differentials->xfrom_ = p_ + differentials_->dp_dx_;
	out_differentials->yfrom_ = p_ + differentials_->dp_dy_;
	//if (Dot(wo, n) < 0) eta = 1.f / eta;
	//Normal dndx = bsdf->dgShading.dndu * bsdf->dgShading.dudx + bsdf->dgShading.dndv * bsdf->dgShading.dvdx;
	//Normal dndy = bsdf->dgShading.dndu * bsdf->dgShading.dudy + bsdf->dgShading.dndv * bsdf->dgShading.dvdy;
	const Vec3f dwodx{in_dir - in_differentials->xdir_};
	const Vec3f dwody{in_dir - in_differentials->ydir_};
	const float d_d_ndx = (dwodx * n_); // + Dot(wo, dndx);
	const float d_d_ndy = (dwody * n_); // + Dot(wo, dndy);
	//	float mu = IOR * (in.dir * sp.N) - (out.dir * sp.N);
	const float dmudx = (ior - (ior * ior * (in_dir * n_)) / (out_dir * n_)) * d_d_ndx;
	const float dmudy = (ior - (ior * ior * (in_dir * n_)) / (out_dir * n_)) * d_d_ndy;
	out_differentials->xdir_ = out_dir + ior * dwodx - (/* mu * dndx + */ dmudx * n_);
	out_differentials->ydir_ = out_dir + ior * dwody - (/* mu * dndy + */ dmudy * n_);
	return out_differentials;
}

float SurfacePoint::projectedPixelArea() const
{
	if(differentials_) return (differentials_->dp_dx_ ^ differentials_->dp_dy_).length();
	else return 0.f;
}

Uv<float> SurfacePoint::dUdvFromPointDifferentials(const Vec3f &dp, const Uv<Vec3f> &dp_duv)
{
	const float det_xy = (dp_duv.u_[Axis::X] * dp_duv.v_[Axis::Y]) - (dp_duv.v_[Axis::X] * dp_duv.u_[Axis::Y]);
	const float det_xz = (dp_duv.u_[Axis::X] * dp_duv.v_[Axis::Z]) - (dp_duv.v_[Axis::X] * dp_duv.u_[Axis::Z]);
	const float det_yz = (dp_duv.u_[Axis::Y] * dp_duv.v_[Axis::Z]) - (dp_duv.v_[Axis::Y] * dp_duv.u_[Axis::Z]);

	if(std::abs(det_xy) > 0.f && std::abs(det_xy) > std::abs(det_xz) && std::abs(det_xy) > std::abs(det_yz))
	{
		const float du = ((dp[Axis::X] * dp_duv.v_[Axis::Y]) - (dp_duv.v_[Axis::X] * dp[Axis::Y])) / det_xy;
		const float dv = ((dp_duv.u_[Axis::X] * dp[Axis::Y]) - (dp[Axis::X] * dp_duv.u_[Axis::Y])) / det_xy;
		return {du, dv};
	}
	else if(std::abs(det_xz) > 0.f && std::abs(det_xz) > std::abs(det_xy) && std::abs(det_xz) > std::abs(det_yz))
	{
		const float du = ((dp[Axis::X] * dp_duv.v_[Axis::Z]) - (dp_duv.v_[Axis::X] * dp[Axis::Z])) / det_xz;
		const float dv = ((dp_duv.u_[Axis::X] * dp[Axis::Z]) - (dp[Axis::X] * dp_duv.u_[Axis::Z])) / det_xz;
		return {du, dv};
	}
	else if(std::abs(det_yz) > 0.f && std::abs(det_yz) > std::abs(det_xy) && std::abs(det_yz) > std::abs(det_xz))
	{
		const float du = ((dp[Axis::Y] * dp_duv.v_[Axis::Z]) - (dp_duv.v_[Axis::Y] * dp[Axis::Z])) / det_yz;
		const float dv = ((dp_duv.u_[Axis::Y] * dp[Axis::Z]) - (dp[Axis::Y] * dp_duv.u_[Axis::Z])) / det_yz;
		return {du, dv};
	}
	return {0.f, 0.f};
}

std::array<Uv<float>, 2> SurfacePoint::getUVdifferentialsXY() const
{
	if(differentials_)
	{
		const Uv<float> d_dx{dUdvFromPointDifferentials(differentials_->dp_dx_, dp_abs_)};
		const Uv<float> d_dy{dUdvFromPointDifferentials(differentials_->dp_dy_, dp_abs_)};
		return {d_dx, d_dy};
	}
	else return {{{0.f, 0.f}, {0.f, 0.f}}};
}

} //namespace yafaray
