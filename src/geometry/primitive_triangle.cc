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

#include "geometry/primitive_triangle.h"
#include "geometry/primitive_triangle_bspline_time.h"
#include "geometry/ray.h"
#include "geometry/bound.h"
#include "geometry/surface.h"
#include "geometry/triangle.h"
#include "geometry/triangle_instance.h"
#include "geometry/object_geom_mesh.h"
#include "geometry/uv.h"

BEGIN_YAFARAY

bool VTriangle::intersect(const Ray &ray, float *t, IntersectData &data) const
{
	//Tomas Moller and Ben Trumbore ray intersection scheme
	const Point3 &a = mesh_->getPoints()[pa_], &b = mesh_->getPoints()[pb_], &c = mesh_->getPoints()[pc_];
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

Bound VTriangle::getBound() const
{
	const Point3 &a = mesh_->getPoints()[pa_], &b = mesh_->getPoints()[pb_], &c = mesh_->getPoints()[pc_];
	const Point3 l { math::min(a.x_, b.x_, c.x_), math::min(a.y_, b.y_, c.y_), math::min(a.z_, b.z_, c.z_) };
	const Point3 h { math::max(a.x_, b.x_, c.x_), math::max(a.y_, b.y_, c.y_), math::max(a.z_, b.z_, c.z_) };
	return Bound(l, h);
}

void VTriangle::getSurface(SurfacePoint &sp, const Point3 &hit, IntersectData &data) const
{
	sp.ng_ = normal_;
	// gives the index in triangle array, according to my latest informations
	// it _should be_ safe to rely on array-like contiguous memory in std::vector<>!
	const int tri_index = this - &(mesh_->getVTriangles().front());
	// the "u" and "v" in triangle intersection code are actually "v" and "w" when u=>p1, v=>p2, w=>p3
	const float barycentric_u = data.barycentric_u_, barycentric_v = data.barycentric_v_, barycentric_w = data.barycentric_w_;
	if(mesh_->isSmooth())
	{
		const Vec3 va(na_ > 0 ? mesh_->getNormals()[na_] : normal_), vb(nb_ > 0 ? mesh_->getNormals()[nb_] : normal_), vc(nc_ > 0 ? mesh_->getNormals()[nc_] : normal_);
		sp.n_ = barycentric_u * va + barycentric_v * vb + barycentric_w * vc;
		sp.n_.normalize();
	}
	else sp.n_ = normal_;

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
		const auto uvi = mesh_->getUvOffsets().begin() + 3 * tri_index;
		const int uvi_1 = *uvi, uvi_2 = *(uvi + 1), uvi_3 = *(uvi + 2);
		const auto it = mesh_->getUvValues().begin();

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
			Vec3 dp_1 = mesh_->getPoints()[pa_] - mesh_->getPoints()[pc_];
			Vec3 dp_2 = mesh_->getPoints()[pb_] - mesh_->getPoints()[pc_];
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

	sp.prim_num_ = tri_index;
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

bool VTriangle::intersectsBound(ExBound &eb) const
{
	const Point3 &a = mesh_->getPoints()[pa_], &b = mesh_->getPoints()[pb_], &c = mesh_->getPoints()[pc_];
	double t_points[3][3];
	for(int j = 0; j < 3; ++j)
	{
		t_points[0][j] = a[j];
		t_points[1][j] = b[j];
		t_points[2][j] = c[j];
	}
	// triBoxOverlap() is in src/yafraycore/tribox3_d.cc!
	return triBoxOverlap__(eb.center_, eb.half_size_, (double **) t_points);
}

bool VTriangle::clipToBound(double bound[2][3], int axis, Bound &clipped, void *d_old, void *d_new) const
{
	if(axis >= 0) // re-clip
	{
		const bool lower = axis & ~3;
		const int axis_calc = axis & 3;
		const double split = lower ? bound[0][axis_calc] : bound[1][axis_calc];
		const int tri_plane_clip = Triangle::triPlaneClip(split, axis_calc, lower, clipped, d_old, d_new);
		// if an error occured due to precision limits...ugly solution i admitt
		if(tri_plane_clip > 1) goto JUMP_WHOOPS; //FIXME: Try to find a better way rather than doing jumps
		return (tri_plane_clip == 0) ? true : false;
	}
	else // initial clip
	{
		//		std::cout << "+";
		JUMP_WHOOPS: //FIXME: Try to find a better way rather than doing jumps
		//		std::cout << "!";
		const Point3 &a = mesh_->getPoints()[pa_], &b = mesh_->getPoints()[pb_], &c = mesh_->getPoints()[pc_];
		double t_points[3][3];
		for(int i = 0; i < 3; ++i)
		{
			t_points[0][i] = a[i];
			t_points[1][i] = b[i];
			t_points[2][i] = c[i];
		}
		const int tri_box_clip = Triangle::triBoxClip(bound[0], bound[1], t_points, clipped, d_new);
		return (tri_box_clip == 0) ? true : false;
	}
}

float VTriangle::surfaceArea() const
{
	const Point3 &a = mesh_->getPoints()[pa_], &b = mesh_->getPoints()[pb_], &c = mesh_->getPoints()[pc_];
	const Vec3 edge_1 = b - a;
	const Vec3 edge_2 = c - a;
	return 0.5f * (edge_1 ^ edge_2).length();
}

void VTriangle::sample(float s_1, float s_2, Point3 &p, Vec3 &n) const
{
	const Point3 &a = mesh_->getPoints()[pa_], &b = mesh_->getPoints()[pb_], &c = mesh_->getPoints()[pc_];
	const float su_1 = math::sqrt(s_1);
	const float u = 1.f - su_1;
	const float v = s_2 * su_1;
	p = u * a + v * b + (1.f - u - v) * c;
	n = Vec3(normal_);
}

void VTriangle::recNormal()
{
	const Point3 &a = mesh_->getPoints()[pa_], &b = mesh_->getPoints()[pb_], &c = mesh_->getPoints()[pc_];
	normal_ = ((b - a) ^ (c - a)).normalize();
}

END_YAFARAY