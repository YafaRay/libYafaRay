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

#include "geometry/primitive_triangle_bspline_time.h"
#include "geometry/ray.h"
#include "geometry/bound.h"
#include "geometry/surface.h"
#include "geometry/primitive_triangle.h"
#include "geometry/triangle_instance.h"
#include "geometry/triangle.h"
#include "geometry/object_geom_mesh.h"
#include "geometry/uv.h"

BEGIN_YAFARAY

bool BsTriangle::intersect(const Ray &ray, float *t, IntersectData &data) const
{
	const Point3 *an = &mesh_->getPoints()[pa_], *bn = &mesh_->getPoints()[pb_], *cn = &mesh_->getPoints()[pc_];
	const float tc = 1.f - ray.time_;
	const float b_1 = tc * tc, b_2 = 2.f * ray.time_ * tc, b_3 = ray.time_ * ray.time_;
	const Point3 a = b_1 * an[0] + b_2 * an[1] + b_3 * an[2];
	const Point3 b = b_1 * bn[0] + b_2 * bn[1] + b_3 * bn[2];
	const Point3 c = b_1 * cn[0] + b_2 * cn[1] + b_3 * cn[2];
	const Vec3 edge_1 = b - a;
	const Vec3 edge_2 = c - a;
	const Vec3 pvec = ray.dir_ ^ edge_2;
	const float det = edge_1 * pvec;
	if(/*(det>-0.000001) && (det<0.000001)*/ det == 0.0) return false;
	const float inv_det = 1.f / det;
	const Vec3 tvec = ray.from_ - a;
	const float u = (tvec * pvec) * inv_det;
	if(u < 0.0 || u > 1.0) return false;
	const Vec3 qvec = tvec ^ edge_1;
	const float v = (ray.dir_ * qvec) * inv_det;
	if((v < 0.0) || ((u + v) > 1.0)) return false;
	*t = edge_2 * qvec * inv_det;
	//UV <-> Barycentric UVW relationship is not obvious, interesting explanation in: https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/barycentric-coordinates
	data.barycentric_u_ = 1.f - u - v;
	data.barycentric_v_ = u;
	data.barycentric_w_ = v;
	data.time_ = ray.time_;
	return true;
}

Bound BsTriangle::getBound() const
{
	const Point3 *an = &mesh_->getPoints()[pa_], *bn = &mesh_->getPoints()[pb_], *cn = &mesh_->getPoints()[pc_];
	const Point3 amin { math::min(an[0].x_, an[1].x_, an[2].x_), math::min(an[0].y_, an[1].y_, an[2].y_), math::min(an[0].z_, an[1].z_, an[2].z_) };
	const Point3 bmin { math::min(bn[0].x_, bn[1].x_, bn[2].x_), math::min(bn[0].y_, bn[1].y_, bn[2].y_), math::min(bn[0].z_, bn[1].z_, bn[2].z_) };
	const Point3 cmin { math::min(cn[0].x_, cn[1].x_, cn[2].x_), math::min(cn[0].y_, cn[1].y_, cn[2].y_), math::min(cn[0].z_, cn[1].z_, cn[2].z_) };
	const Point3 amax { math::max(an[0].x_, an[1].x_, an[2].x_), math::max(an[0].y_, an[1].y_, an[2].y_), math::max(an[0].z_, an[1].z_, an[2].z_) };
	const Point3 bmax { math::max(bn[0].x_, bn[1].x_, bn[2].x_), math::max(bn[0].y_, bn[1].y_, bn[2].y_), math::max(bn[0].z_, bn[1].z_, bn[2].z_) };
	const Point3 cmax { math::max(cn[0].x_, cn[1].x_, cn[2].x_), math::max(cn[0].y_, cn[1].y_, cn[2].y_), math::max(cn[0].z_, cn[1].z_, cn[2].z_) };
	const Point3 l { math::min(amin.x_, bmin.x_, cmin.x_), math::min(amin.y_, bmin.y_, cmin.y_), math::min(amin.z_, bmin.z_, cmin.z_) };
	const Point3 h { math::max(amax.x_, bmax.x_, cmax.x_), math::max(amax.y_, bmax.y_, cmax.y_), math::max(amax.z_, bmax.z_, cmax.z_) };
	return Bound(l, h);
}

void BsTriangle::getSurface(SurfacePoint &sp, const Point3 &hit, IntersectData &data) const
{
	// recalculating the points is not really the nicest solution...
	const Point3 *an = &mesh_->getPoints()[pa_], *bn = &mesh_->getPoints()[pb_], *cn = &mesh_->getPoints()[pc_];
	const float time = data.time_;
	const float tc = 1.f - time;
	const float b_1 = tc * tc, b_2 = 2.f * time * tc, b_3 = time * time;
	const Point3 a = b_1 * an[0] + b_2 * an[1] + b_3 * an[2];
	const Point3 b = b_1 * bn[0] + b_2 * bn[1] + b_3 * bn[2];
	const Point3 c = b_1 * cn[0] + b_2 * cn[1] + b_3 * cn[2];

	sp.ng_ = ((b - a) ^ (c - a)).normalize();
	// the "u" and "v" in triangle intersection code are actually "v" and "w" when u=>p1, v=>p2, w=>p3
	const float barycentric_u = data.barycentric_u_, barycentric_v = data.barycentric_v_, barycentric_w = data.barycentric_w_;

	//todo: calculate smoothed normal...
	/* if(mesh->is_smooth || mesh->normals_exported)
	{
		vector3d_t va(na>0? mesh->normals[na] : normal), vb(nb>0? mesh->normals[nb] : normal), vc(nc>0? mesh->normals[nc] : normal);
		sp.N = u*va + v*vb + w*vc;
		sp.N.normalize();
	}
	else  */sp.n_ = sp.ng_;

	if(mesh_->hasOrco())
	{
		sp.orco_p_ = barycentric_u * mesh_->getPoints()[pa_ + 1] + barycentric_v * mesh_->getPoints()[pb_ + 1] + barycentric_w * mesh_->getPoints()[pc_ + 1];
		sp.orco_ng_ = ((mesh_->getPoints()[pb_ + 1] - mesh_->getPoints()[pa_ + 1]) ^ (mesh_->getPoints()[pc_ + 1] - mesh_->getPoints()[pa_ + 1])).normalize();
		sp.has_orco_ = true;
	}
	else
	{
		sp.orco_p_ = hit;
		sp.orco_ng_ = sp.ng_;
		sp.has_orco_ = false;
	}
	if(mesh_->hasUv())
	{
		//		static bool test=true;

		// gives the index in triangle array, according to my latest informations
		// it _should be_ safe to rely on array-like contiguous memory in std::vector<>!
		const unsigned int tri_index = this - &(mesh_->getBsTriangles().front());
		const auto uvi = mesh_->getUvOffsets().begin() + 3 * tri_index;
		const int uvi_1 = *uvi, uvi_2 = *(uvi + 1), uvi_3 = *(uvi + 2);
		const auto &it = mesh_->getUvValues().begin();
		sp.u_ = barycentric_u * it[uvi_1].u_ + barycentric_v * it[uvi_2].u_ + barycentric_w * it[uvi_3].u_;
		sp.v_ = barycentric_u * it[uvi_1].v_ + barycentric_v * it[uvi_2].v_ + barycentric_w * it[uvi_3].v_;

		// calculate dPdU and dPdV
		const float du_1 = it[uvi_1].u_ - it[uvi_3].u_;
		const float du_2 = it[uvi_2].u_ - it[uvi_3].u_;
		const float dv_1 = it[uvi_1].v_ - it[uvi_3].v_;
		const float dv_2 = it[uvi_2].v_ - it[uvi_3].v_;
		const float det = du_1 * dv_2 - dv_1 * du_2;

		if(std::abs(det) > 1e-30f)
		{
			const float invdet = 1.f / det;
			const Vec3 dp_1 = mesh_->getPoints()[pa_] - mesh_->getPoints()[pc_];
			const Vec3 dp_2 = mesh_->getPoints()[pb_] - mesh_->getPoints()[pc_];
			sp.dp_du_ = (dv_2 * invdet) * dp_1 - (dv_1 * invdet) * dp_2;
			sp.dp_dv_ = (du_1 * invdet) * dp_2 - (du_2 * invdet) * dp_1;
		}
		else
		{
			// implicit mapping, p0 = 0/0, p1 = 1/0, p2 = 0/1 => sp.u_ = barycentric_u, sp.v_ = barycentric_v; (arbitrary choice)
			sp.dp_du_ = mesh_->getPoints()[pb_] - mesh_->getPoints()[pa_];
			sp.dp_dv_ = mesh_->getPoints()[pc_] - mesh_->getPoints()[pa_];
			sp.u_ = barycentric_u;
			sp.v_ = barycentric_v;
		}
	}
	else
	{
		// implicit mapping, p0 = 0/0, p1 = 1/0, p2 = 0/1 => sp.u_ = barycentric_u, sp.v_ = barycentric_v; (arbitrary choice)
		sp.dp_du_ = mesh_->getPoints()[pb_] - mesh_->getPoints()[pa_];
		sp.dp_dv_ = mesh_->getPoints()[pc_] - mesh_->getPoints()[pa_];
		sp.u_ = barycentric_u;
		sp.v_ = barycentric_v;
	}

	//Copy original dPdU and dPdV before normalization to the "absolute" dPdU and dPdV (for mipmap calculations)
	sp.dp_du_abs_ = sp.dp_du_;
	sp.dp_dv_abs_ = sp.dp_dv_;

	sp.dp_du_.normalize();
	sp.dp_dv_.normalize();

	sp.material_ = material_;
	sp.p_ = hit;
	Vec3::createCs(sp.n_, sp.nu_, sp.nv_);
	// transform dPdU and dPdV in shading space
	sp.ds_du_.x_ = sp.nu_ * sp.dp_du_;
	sp.ds_du_.y_ = sp.nv_ * sp.dp_du_;
	sp.ds_du_.z_ = sp.n_ * sp.dp_du_;
	sp.ds_dv_.x_ = sp.nu_ * sp.dp_dv_;
	sp.ds_dv_.y_ = sp.nv_ * sp.dp_dv_;
	sp.ds_dv_.z_ = sp.n_ * sp.dp_dv_;
	sp.light_ = mesh_->getLight();
	sp.has_uv_ = mesh_->hasUv();
}

END_YAFARAY