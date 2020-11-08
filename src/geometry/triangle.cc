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

#include "geometry/triangle.h"

#ifdef __clang__
#define inline  // aka inline removal
#endif

BEGIN_YAFARAY

int triBoxClip__(const double b_min[3], const double b_max[3], const double triverts[3][3], Bound &box, void *n_dat);
int triPlaneClip__(double pos, int axis, bool lower, Bound &box, const void *o_dat, void *n_dat);

void Triangle::getSurface(SurfacePoint &sp, const Point3 &hit, IntersectData &data) const
{
	sp.ng_ = getNormal();
	const float barycentric_u = data.barycentric_u_, barycentric_v = data.barycentric_v_, barycentric_w = data.barycentric_w_;

	if(mesh_->is_smooth_ || mesh_->normals_exported_)
	{
		const Vec3 va = (na_ >= 0) ? mesh_->getVertexNormal(na_) : sp.ng_;
		const Vec3 vb = (nb_ >= 0) ? mesh_->getVertexNormal(nb_) : sp.ng_;
		const Vec3 vc = (nc_ >= 0) ? mesh_->getVertexNormal(nc_) : sp.ng_;
		sp.n_ = barycentric_u * va + barycentric_v * vb + barycentric_w * vc;
		sp.n_.normalize();
	}
	else sp.n_ = sp.ng_;

	if(mesh_->has_orco_)
	{
		// if the object is an instance, the vertex positions are the orcos
		const Point3 &p_0 = mesh_->getVertex(pa_ + 1);
		const Point3 &p_1 = mesh_->getVertex(pb_ + 1);
		const Point3 &p_2 = mesh_->getVertex(pc_ + 1);
		sp.orco_p_ = barycentric_u * p_0 + barycentric_v * p_1 + barycentric_w * p_2;
		sp.orco_ng_ = ((p_1 - p_0) ^ (p_2 - p_0)).normalize();
		sp.has_orco_ = true;
	}
	else
	{
		sp.orco_p_ = hit;
		sp.has_orco_ = false;
		sp.orco_ng_ = sp.ng_;
	}

	const Point3 &p_0 = mesh_->getVertex(pa_);
	const Point3 &p_1 = mesh_->getVertex(pb_);
	const Point3 &p_2 = mesh_->getVertex(pc_);
	if(mesh_->has_uv_)
	{
		const size_t uvi = self_index_ * 3;
		const Uv &uv_0 = mesh_->uv_values_[mesh_->uv_offsets_[uvi]];
		const Uv &uv_1 = mesh_->uv_values_[mesh_->uv_offsets_[uvi + 1]];
		const Uv &uv_2 = mesh_->uv_values_[mesh_->uv_offsets_[uvi + 2]];

		sp.u_ = barycentric_u * uv_0.u_ + barycentric_v * uv_1.u_ + barycentric_w * uv_2.u_;
		sp.v_ = barycentric_u * uv_0.v_ + barycentric_v * uv_1.v_ + barycentric_w * uv_2.v_;

		// calculate dPdU and dPdV
		const float du_1 = uv_1.u_ - uv_0.u_;
		const float du_2 = uv_2.u_ - uv_0.u_;
		const float dv_1 = uv_1.v_ - uv_0.v_;
		const float dv_2 = uv_2.v_ - uv_0.v_;
		const float det = du_1 * dv_2 - dv_1 * du_2;

		if(std::abs(det) > 1e-30f)
		{
			const float invdet = 1.f / det;
			const Vec3 dp_1 = p_1 - p_0;
			const Vec3 dp_2 = p_2 - p_0;
			sp.dp_du_ = (dv_2 * dp_1 - dv_1 * dp_2) * invdet;
			sp.dp_dv_ = (du_1 * dp_2 - du_2 * dp_1) * invdet;
		}
		else
		{
			// implicit mapping, p0 = 0/0, p1 = 1/0, p2 = 0/1 => sp.u_ = barycentric_u, sp.v_ = barycentric_v; (arbitrary choice)
			sp.dp_du_ = p_1 - p_0;
			sp.dp_dv_ = p_2 - p_0;
			sp.u_ = barycentric_u;
			sp.v_ = barycentric_v;
		}
	}
	else
	{
		// implicit mapping, p0 = 0/0, p1 = 1/0, p2 = 0/1 => sp.u_ = barycentric_u, sp.v_ = barycentric_v; (arbitrary choice)
		sp.dp_du_ = p_1 - p_0;
		sp.dp_dv_ = p_2 - p_0;
		sp.u_ = barycentric_u;
		sp.v_ = barycentric_v;
	}

	//Copy original dPdU and dPdV before normalization to the "absolute" dPdU and dPdV (for mipmap calculations)
	sp.dp_du_abs_ = sp.dp_du_;
	sp.dp_dv_abs_ = sp.dp_dv_;

	sp.dp_du_.normalize();
	sp.dp_dv_.normalize();

	sp.object_ = mesh_;
	sp.prim_num_ = self_index_;
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
	sp.light_ = mesh_->light_;
	sp.has_uv_ = mesh_->has_uv_;
}

bool Triangle::clipToBound(const double bound[2][3], int axis, Bound &clipped, const void *d_old, void *d_new) const
{
	if(axis >= 0) // re-clip
	{
		const bool lower = axis & ~3;
		const int axis_calc = axis & 3;
		const double split = lower ? bound[0][axis_calc] : bound[1][axis_calc];
		const int tri_plane_clip = triPlaneClip__(split, axis_calc, lower, clipped, d_old, d_new);
		// if an error occured due to precision limits...ugly solution i admitt
		if(tri_plane_clip > 1) goto WHOOPS;
		return (tri_plane_clip == 0) ? true : false;
	}
	else // initial clip
	{
WHOOPS:

		const Point3 &a = mesh_->getVertex(pa_);
		const Point3 &b = mesh_->getVertex(pb_);
		const Point3 &c = mesh_->getVertex(pc_);
		double t_points[3][3];
		for(int i = 0; i < 3; ++i)
		{
			t_points[0][i] = a[i];
			t_points[1][i] = b[i];
			t_points[2][i] = c[i];
		}
		const int tri_box_clip = triBoxClip__(bound[0], bound[1], t_points, clipped, d_new);
		return (tri_box_clip == 0) ? true : false;
	}
	return true;
}

float Triangle::surfaceArea() const
{
	const Point3 &a = mesh_->getVertex(pa_);
	const Point3 &b = mesh_->getVertex(pb_);
	const Point3 &c = mesh_->getVertex(pc_);
	const Vec3 edge_1 = b - a;
	const Vec3 edge_2 = c - a;
	return 0.5f * (edge_1 ^ edge_2).length();
}

void Triangle::sample(float s_1, float s_2, Point3 &p, Vec3 &n) const
{
	const Point3 &a = mesh_->getVertex(pa_);
	const Point3 &b = mesh_->getVertex(pb_);
	const Point3 &c = mesh_->getVertex(pc_);
	const float su_1 = math::sqrt(s_1);
	const float u = 1.f - su_1;
	const float v = s_2 * su_1;
	p = u * a + v * b + (1.f - u - v) * c;
	n = getNormal();
}

// triangleInstance_t Methods

void TriangleInstance::getSurface(SurfacePoint &sp, const Point3 &hit, IntersectData &data) const
{
	sp.ng_ = getNormal();
	const int pa = m_base_->pa_;
	const int pb = m_base_->pb_;
	const int pc = m_base_->pc_;
	const int na = m_base_->na_;
	const int nb = m_base_->nb_;
	const int nc = m_base_->nc_;
	const size_t self_index = m_base_->self_index_;
	const float barycentric_u = data.barycentric_u_, barycentric_v = data.barycentric_v_, barycentric_w = data.barycentric_w_;
	if(mesh_->is_smooth_ || mesh_->normals_exported_)
	{
		// assume the smoothed normals exist, if the mesh is smoothed; if they don't, fix this
		// assert(na > 0 && nb > 0 && nc > 0);
		const Vec3 va = (na > 0) ? mesh_->getVertexNormal(na) : sp.ng_;
		const Vec3 vb = (nb > 0) ? mesh_->getVertexNormal(nb) : sp.ng_;
		const Vec3 vc = (nc > 0) ? mesh_->getVertexNormal(nc) : sp.ng_;
		sp.n_ = barycentric_u * va + barycentric_v * vb + barycentric_w * vc;
		sp.n_.normalize();
	}
	else sp.n_ = sp.ng_;

	if(mesh_->has_orco_)
	{
		// if the object is an instance, the vertex positions are the orcos
		const Point3 &p_0 = m_base_->mesh_->getVertex(pa + 1);
		const Point3 &p_1 = m_base_->mesh_->getVertex(pb + 1);
		const Point3 &p_2 = m_base_->mesh_->getVertex(pc + 1);
		sp.orco_p_ = barycentric_u * p_0 + barycentric_v * p_1 + barycentric_w * p_2;
		sp.orco_ng_ = ((p_1 - p_0) ^ (p_2 - p_0)).normalize();
		sp.has_orco_ = true;
	}
	else
	{
		sp.orco_p_ = hit;
		sp.has_orco_ = false;
		sp.orco_ng_ = sp.ng_;
	}

	const Point3 &p_0 = mesh_->getVertex(pa);
	const Point3 &p_1 = mesh_->getVertex(pb);
	const Point3 &p_2 = mesh_->getVertex(pc);

	if(mesh_->has_uv_)
	{
		const size_t uvi = self_index * 3;
		const Uv &uv_0 = mesh_->m_base_->uv_values_[mesh_->m_base_->uv_offsets_[uvi]];
		const Uv &uv_1 = mesh_->m_base_->uv_values_[mesh_->m_base_->uv_offsets_[uvi + 1]];
		const Uv &uv_2 = mesh_->m_base_->uv_values_[mesh_->m_base_->uv_offsets_[uvi + 2]];

		sp.u_ = barycentric_u * uv_0.u_ + barycentric_v * uv_1.u_ + barycentric_w * uv_2.u_;
		sp.v_ = barycentric_u * uv_0.v_ + barycentric_v * uv_1.v_ + barycentric_w * uv_2.v_;

		// calculate dPdU and dPdV
		const float du_1 = uv_0.u_ - uv_2.u_;
		const float du_2 = uv_1.u_ - uv_2.u_;
		const float dv_1 = uv_0.v_ - uv_2.v_;
		const float dv_2 = uv_1.v_ - uv_2.v_;
		const float det = du_1 * dv_2 - dv_1 * du_2;

		if(std::abs(det) > 1e-30f)
		{
			const float invdet = 1.f / det;
			const Vec3 dp_1 = p_0 - p_2;
			const Vec3 dp_2 = p_1 - p_2;
			sp.dp_du_ = (dv_2 * dp_1 - dv_1 * dp_2) * invdet;
			sp.dp_dv_ = (du_1 * dp_2 - du_2 * dp_1) * invdet;
		}
		else
		{
			const Vec3 dp_1 = p_0 - p_2;
			const Vec3 dp_2 = p_1 - p_2;
			Vec3::createCs((dp_2 ^ dp_1).normalize(), sp.dp_du_, sp.dp_dv_);
		}
	}
	else
	{
		// implicit mapping, p0 = 0/0, p1 = 1/0, p2 = 0/1 => sp.u_ = barycentric_u, sp.v_ = barycentric_v; (arbitrary choice)
		sp.dp_du_ = p_1 - p_0;
		sp.dp_dv_ = p_2 - p_0;
		sp.u_ = barycentric_u;
		sp.v_ = barycentric_v;
	}

	//Copy original dPdU and dPdV before normalization to the "absolute" dPdU and dPdV (for mipmap calculations)
	sp.dp_du_abs_ = sp.dp_du_;
	sp.dp_dv_abs_ = sp.dp_dv_;

	sp.dp_du_.normalize();
	sp.dp_dv_.normalize();

	sp.object_ = mesh_;
	sp.prim_num_ = self_index;
	sp.material_ = m_base_->material_;
	sp.p_ = hit;
	Vec3::createCs(sp.n_, sp.nu_, sp.nv_);
	Vec3 u_vec, v_vec;
	Vec3::createCs(sp.ng_, u_vec, v_vec);
	// transform dPdU and dPdV in shading space
	sp.ds_du_.x_ = u_vec * sp.dp_du_;
	sp.ds_du_.y_ = v_vec * sp.dp_du_;
	sp.ds_du_.z_ = sp.ng_ * sp.dp_du_;
	sp.ds_dv_.x_ = u_vec * sp.dp_dv_;
	sp.ds_dv_.y_ = v_vec * sp.dp_dv_;
	sp.ds_dv_.z_ = sp.ng_ * sp.dp_dv_;
	sp.ds_du_.normalize();
	sp.ds_dv_.normalize();
	sp.light_ = mesh_->m_base_->light_;
	sp.has_uv_ = mesh_->has_uv_;
}

bool TriangleInstance::clipToBound(const double bound[2][3], int axis, Bound &clipped, const void *d_old, void *d_new) const
{
	if(axis >= 0) // re-clip
	{
		const bool lower = axis & ~3;
		const int axis_calc = axis & 3;
		const double split = lower ? bound[0][axis_calc] : bound[1][axis_calc];
		const int tri_plane_clip = triPlaneClip__(split, axis_calc, lower, clipped, d_old, d_new);
		// if an error occured due to precision limits...ugly solution i admit
		if(tri_plane_clip > 1)
		{
			const Point3 &a = mesh_->getVertex(m_base_->pa_);
			const Point3 &b = mesh_->getVertex(m_base_->pb_);
			const Point3 &c = mesh_->getVertex(m_base_->pc_);

			double t_points[3][3];
			for(int i = 0; i < 3; ++i)
			{
				t_points[0][i] = a[i];
				t_points[1][i] = b[i];
				t_points[2][i] = c[i];
			}
			const int tri_box_clip = triBoxClip__(bound[0], bound[1], t_points, clipped, d_new);
			return (tri_box_clip == 0);
		}
		return (tri_plane_clip == 0);
	}
	else // initial clip
	{
		const Point3 &a = mesh_->getVertex(m_base_->pa_);
		const Point3 &b = mesh_->getVertex(m_base_->pb_);
		const Point3 &c = mesh_->getVertex(m_base_->pc_);

		double t_points[3][3];
		for(int i = 0; i < 3; ++i)
		{
			t_points[0][i] = a[i];
			t_points[1][i] = b[i];
			t_points[2][i] = c[i];
		}
		const int tri_box_clip = triBoxClip__(bound[0], bound[1], t_points, clipped, d_new);
		return (tri_box_clip == 0);
	}
}

float TriangleInstance::surfaceArea() const
{
	const Point3 &a = mesh_->getVertex(m_base_->pa_);
	const Point3 &b = mesh_->getVertex(m_base_->pb_);
	const Point3 &c = mesh_->getVertex(m_base_->pc_);
	const Vec3 edge_1 = b - a;
	const Vec3 edge_2 = c - a;
	return 0.5f * (edge_1 ^ edge_2).length();
}

void TriangleInstance::sample(float s_1, float s_2, Point3 &p, Vec3 &n) const
{
	const Point3 &a = mesh_->getVertex(m_base_->pa_);
	const Point3 &b = mesh_->getVertex(m_base_->pb_);
	const Point3 &c = mesh_->getVertex(m_base_->pc_);
	const float su_1 = math::sqrt(s_1);
	const float u = 1.f - su_1;
	const float v = s_2 * su_1;
	p = u * a + v * b + (1.f - u - v) * c;
	n = getNormal();
}

//==========================================
// vTriangle_t methods, mosty c&p...
//==========================================

bool VTriangle::intersect(const Ray &ray, float *t, IntersectData &data) const
{
	//Tomas Moller and Ben Trumbore ray intersection scheme
	const Point3 &a = mesh_->points_[pa_], &b = mesh_->points_[pb_], &c = mesh_->points_[pc_];
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
	const Point3 &a = mesh_->points_[pa_], &b = mesh_->points_[pb_], &c = mesh_->points_[pc_];
	const Point3 l { math::min(a.x_, b.x_, c.x_), math::min(a.y_, b.y_, c.y_), math::min(a.z_, b.z_, c.z_) };
	const Point3 h { math::max(a.x_, b.x_, c.x_), math::max(a.y_, b.y_, c.y_), math::max(a.z_, b.z_, c.z_) };
	return Bound(l, h);
}

void VTriangle::getSurface(SurfacePoint &sp, const Point3 &hit, IntersectData &data) const
{
	sp.ng_ = normal_;
	// gives the index in triangle array, according to my latest informations
	// it _should be_ safe to rely on array-like contiguous memory in std::vector<>!
	const int tri_index = this - &(mesh_->triangles_.front());
	// the "u" and "v" in triangle intersection code are actually "v" and "w" when u=>p1, v=>p2, w=>p3
	const float barycentric_u = data.barycentric_u_, barycentric_v = data.barycentric_v_, barycentric_w = data.barycentric_w_;
	if(mesh_->is_smooth_)
	{
		const Vec3 va(na_ > 0 ? mesh_->normals_[na_] : normal_), vb(nb_ > 0 ? mesh_->normals_[nb_] : normal_), vc(nc_ > 0 ? mesh_->normals_[nc_] : normal_);
		sp.n_ = barycentric_u * va + barycentric_v * vb + barycentric_w * vc;
		sp.n_.normalize();
	}
	else sp.n_ = normal_;

	if(mesh_->has_orco_)
	{
		sp.orco_p_ = barycentric_u * mesh_->points_[pa_ + 1] + barycentric_v * mesh_->points_[pb_ + 1] + barycentric_w * mesh_->points_[pc_ + 1];
		sp.orco_ng_ = ((mesh_->points_[pb_ + 1] - mesh_->points_[pa_ + 1]) ^ (mesh_->points_[pc_ + 1] - mesh_->points_[pa_ + 1])).normalize();
		sp.has_orco_ = true;
	}
	else
	{
		sp.orco_p_ = hit;
		sp.orco_ng_ = sp.ng_;
		sp.has_orco_ = false;
	}
	if(mesh_->has_uv_)
	{
		const auto uvi = mesh_->uv_offsets_.begin() + 3 * tri_index;
		const int uvi_1 = *uvi, uvi_2 = *(uvi + 1), uvi_3 = *(uvi + 2);
		const auto it = mesh_->uv_values_.begin();

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
			Vec3 dp_1 = mesh_->points_[pa_] - mesh_->points_[pc_];
			Vec3 dp_2 = mesh_->points_[pb_] - mesh_->points_[pc_];
			sp.dp_du_ = (dv_2 * invdet) * dp_1 - (dv_1 * invdet) * dp_2;
			sp.dp_dv_ = (du_1 * invdet) * dp_2 - (du_2 * invdet) * dp_1;
		}
		else
		{
			// implicit mapping, p0 = 0/0, p1 = 1/0, p2 = 0/1 => sp.u_ = barycentric_u, sp.v_ = barycentric_v; (arbitrary choice)
			sp.dp_du_ = mesh_->points_[pb_] - mesh_->points_[pa_];
			sp.dp_dv_ = mesh_->points_[pc_] - mesh_->points_[pa_];
			sp.u_ = barycentric_u;
			sp.v_ = barycentric_v;
		}
	}
	else
	{
		// implicit mapping, p0 = 0/0, p1 = 1/0, p2 = 0/1 => sp.u_ = barycentric_u, sp.v_ = barycentric_v; (arbitrary choice)
		sp.dp_du_ = mesh_->points_[pb_] - mesh_->points_[pa_];
		sp.dp_dv_ = mesh_->points_[pc_] - mesh_->points_[pa_];
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
	sp.light_ = mesh_->light_;
	sp.has_uv_ = mesh_->has_uv_;
}

bool VTriangle::intersectsBound(ExBound &eb) const
{
	const Point3 &a = mesh_->points_[pa_], &b = mesh_->points_[pb_], &c = mesh_->points_[pc_];
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
		const int tri_plane_clip = triPlaneClip__(split, axis_calc, lower, clipped, d_old, d_new);
		// if an error occured due to precision limits...ugly solution i admitt
		if(tri_plane_clip > 1) goto JUMP_WHOOPS; //FIXME: Try to find a better way rather than doing jumps
		return (tri_plane_clip == 0) ? true : false;
	}
	else // initial clip
	{
		//		std::cout << "+";
JUMP_WHOOPS: //FIXME: Try to find a better way rather than doing jumps
		//		std::cout << "!";
		const Point3 &a = mesh_->points_[pa_], &b = mesh_->points_[pb_], &c = mesh_->points_[pc_];
		double t_points[3][3];
		for(int i = 0; i < 3; ++i)
		{
			t_points[0][i] = a[i];
			t_points[1][i] = b[i];
			t_points[2][i] = c[i];
		}
		const int tri_box_clip = triBoxClip__(bound[0], bound[1], t_points, clipped, d_new);
		return (tri_box_clip == 0) ? true : false;
	}
}

float VTriangle::surfaceArea() const
{
	const Point3 &a = mesh_->points_[pa_], &b = mesh_->points_[pb_], &c = mesh_->points_[pc_];
	const Vec3 edge_1 = b - a;
	const Vec3 edge_2 = c - a;
	return 0.5f * (edge_1 ^ edge_2).length();
}

void VTriangle::sample(float s_1, float s_2, Point3 &p, Vec3 &n) const
{
	const Point3 &a = mesh_->points_[pa_], &b = mesh_->points_[pb_], &c = mesh_->points_[pc_];
	const float su_1 = math::sqrt(s_1);
	const float u = 1.f - su_1;
	const float v = s_2 * su_1;
	p = u * a + v * b + (1.f - u - v) * c;
	n = Vec3(normal_);
}

void VTriangle::recNormal()
{
	const Point3 &a = mesh_->points_[pa_], &b = mesh_->points_[pb_], &c = mesh_->points_[pc_];
	normal_ = ((b - a) ^ (c - a)).normalize();
}

//==========================================
// bsTriangle_t methods
//==========================================

bool BsTriangle::intersect(const Ray &ray, float *t, IntersectData &data) const
{
	const Point3 *an = &mesh_->points_[pa_], *bn = &mesh_->points_[pb_], *cn = &mesh_->points_[pc_];
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
	const Point3 *an = &mesh_->points_[pa_], *bn = &mesh_->points_[pb_], *cn = &mesh_->points_[pc_];
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
	const Point3 *an = &mesh_->points_[pa_], *bn = &mesh_->points_[pb_], *cn = &mesh_->points_[pc_];
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

	if(mesh_->has_orco_)
	{
		sp.orco_p_ = barycentric_u * mesh_->points_[pa_ + 1] + barycentric_v * mesh_->points_[pb_ + 1] + barycentric_w * mesh_->points_[pc_ + 1];
		sp.orco_ng_ = ((mesh_->points_[pb_ + 1] - mesh_->points_[pa_ + 1]) ^ (mesh_->points_[pc_ + 1] - mesh_->points_[pa_ + 1])).normalize();
		sp.has_orco_ = true;
	}
	else
	{
		sp.orco_p_ = hit;
		sp.orco_ng_ = sp.ng_;
		sp.has_orco_ = false;
	}
	if(mesh_->has_uv_)
	{
		//		static bool test=true;

		// gives the index in triangle array, according to my latest informations
		// it _should be_ safe to rely on array-like contiguous memory in std::vector<>!
		const unsigned int tri_index = this - &(mesh_->s_triangles_.front());
		const auto uvi = mesh_->uv_offsets_.begin() + 3 * tri_index;
		const int uvi_1 = *uvi, uvi_2 = *(uvi + 1), uvi_3 = *(uvi + 2);
		const auto it = mesh_->uv_values_.begin();
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
			const Vec3 dp_1 = mesh_->points_[pa_] - mesh_->points_[pc_];
			const Vec3 dp_2 = mesh_->points_[pb_] - mesh_->points_[pc_];
			sp.dp_du_ = (dv_2 * invdet) * dp_1 - (dv_1 * invdet) * dp_2;
			sp.dp_dv_ = (du_1 * invdet) * dp_2 - (du_2 * invdet) * dp_1;
		}
		else
		{
			// implicit mapping, p0 = 0/0, p1 = 1/0, p2 = 0/1 => sp.u_ = barycentric_u, sp.v_ = barycentric_v; (arbitrary choice)
			sp.dp_du_ = mesh_->points_[pb_] - mesh_->points_[pa_];
			sp.dp_dv_ = mesh_->points_[pc_] - mesh_->points_[pa_];
			sp.u_ = barycentric_u;
			sp.v_ = barycentric_v;
		}
	}
	else
	{
		// implicit mapping, p0 = 0/0, p1 = 1/0, p2 = 0/1 => sp.u_ = barycentric_u, sp.v_ = barycentric_v; (arbitrary choice)
		sp.dp_du_ = mesh_->points_[pb_] - mesh_->points_[pa_];
		sp.dp_dv_ = mesh_->points_[pc_] - mesh_->points_[pa_];
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
	sp.light_ = mesh_->light_;
	sp.has_uv_ = mesh_->has_uv_;
}


END_YAFARAY
