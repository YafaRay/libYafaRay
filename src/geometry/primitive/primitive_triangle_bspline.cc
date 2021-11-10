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

#include "geometry/primitive/primitive_triangle_bspline.h"
#include "geometry/ray.h"
#include "geometry/bound.h"
#include "geometry/surface.h"
#include "geometry/object/object_mesh.h"
#include "geometry/uv.h"

BEGIN_YAFARAY

BsTrianglePrimitive::BsTrianglePrimitive(const std::vector<int> &vertices_indices, const std::vector<int> &vertices_uv_indices, const MeshObject &mesh_object) : FacePrimitive(vertices_indices, vertices_uv_indices, mesh_object)
{
	//calculateGeometricNormal(); //FIXME?
}

IntersectData BsTrianglePrimitive::intersect(const Ray &ray, const Matrix4 *obj_to_world) const
{
	const std::vector<int> vertices_indices = getVerticesIndices();
	const std::vector<Point3> &points = base_mesh_object_.getPoints();
	const Point3 *an = &points[vertices_indices[0]];
	const Point3 *bn = &points[vertices_indices[1]];
	const Point3 *cn = &points[vertices_indices[2]];
	const float tc = 1.f - ray.time_;
	const float b_1 = tc * tc, b_2 = 2.f * ray.time_ * tc, b_3 = ray.time_ * ray.time_;
	const Point3 a = b_1 * an[0] + b_2 * an[1] + b_3 * an[2];
	const Point3 b = b_1 * bn[0] + b_2 * bn[1] + b_3 * bn[2];
	const Point3 c = b_1 * cn[0] + b_2 * cn[1] + b_3 * cn[2];
	const Vec3 edge_1 = b - a;
	const Vec3 edge_2 = c - a;
	const Vec3 pvec = ray.dir_ ^ edge_2;
	const float det = edge_1 * pvec;
	if(/*(det>-0.000001) && (det<0.000001)*/ det == 0.0) return {};
	const float inv_det = 1.f / det;
	const Vec3 tvec = ray.from_ - a;
	const float u = (tvec * pvec) * inv_det;
	if(u < 0.0 || u > 1.0) return {};
	const Vec3 qvec = tvec ^ edge_1;
	const float v = (ray.dir_ * qvec) * inv_det;
	if((v < 0.0) || ((u + v) > 1.0)) return {};
	IntersectData intersect_data;
	intersect_data.hit_ = true;
	intersect_data.t_hit_ = edge_2 * qvec * inv_det;
	//UV <-> Barycentric UVW relationship is not obvious, interesting explanation in: https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/barycentric-coordinates
	intersect_data.barycentric_u_ = 1.f - u - v;
	intersect_data.barycentric_v_ = u;
	intersect_data.barycentric_w_ = v;
	intersect_data.time_ = ray.time_;
	return intersect_data;
}

Bound BsTrianglePrimitive::getBound(const Matrix4 *obj_to_world) const
{
	const std::vector<int> vertices_indices = getVerticesIndices();
	const std::vector<Point3> &points = base_mesh_object_.getPoints();
	const Point3 *an = &points[vertices_indices[0]];
	const Point3 *bn = &points[vertices_indices[1]];
	const Point3 *cn = &points[vertices_indices[2]];
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

SurfacePoint BsTrianglePrimitive::getSurface(const RayDifferentials *ray_differentials, const Point3 &hit, const IntersectData &intersect_data, const Matrix4 *obj_to_world, const Camera *camera) const
{
	// recalculating the points is not really the nicest solution...
	const std::vector<int> vertices_indices = getVerticesIndices();
	const std::vector<Point3> &points = base_mesh_object_.getPoints();
	const Point3 *an = &points[vertices_indices[0]];
	const Point3 *bn = &points[vertices_indices[1]];
	const Point3 *cn = &points[vertices_indices[2]];
	const float time = intersect_data.time_;
	const float tc = 1.f - time;
	const float b_1 = tc * tc, b_2 = 2.f * time * tc, b_3 = time * time;
	const Point3 a = b_1 * an[0] + b_2 * an[1] + b_3 * an[2];
	const Point3 b = b_1 * bn[0] + b_2 * bn[1] + b_3 * bn[2];
	const Point3 c = b_1 * cn[0] + b_2 * cn[1] + b_3 * cn[2];

	SurfacePoint sp;
	sp.intersect_data_ = intersect_data;
	sp.ng_ = ((b - a) ^ (c - a)).normalize();
	// the "u" and "v" in triangle intersection code are actually "v" and "w" when u=>p1, v=>p2, w=>p3
	const float barycentric_u = intersect_data.barycentric_u_, barycentric_v = intersect_data.barycentric_v_, barycentric_w = intersect_data.barycentric_w_;

	//todo: calculate smoothed normal...
	/* if(mesh->is_smooth || mesh->normals_exported)
	{
		vector3d_t va(na>0? mesh->normals[na] : normal), vb(nb>0? mesh->normals[nb] : normal), vc(nc>0? mesh->normals[nc] : normal);
		sp.N = u*va + v*vb + w*vc;
		sp.N.normalize();
	}
	else  */sp.n_ = sp.ng_;

	if(base_mesh_object_.hasOrco())
	{
		const std::vector<Point3> orco = getOrcoVertices();
		sp.orco_p_ = barycentric_u * orco[0] + barycentric_v * orco[1] + barycentric_w * orco[2];
		sp.orco_ng_ = ((orco[1] - orco[0]) ^ (orco[2] - orco[0])).normalize();
		sp.has_orco_ = true;
	}
	else
	{
		sp.orco_p_ = hit;
		sp.orco_ng_ = sp.ng_;
		sp.has_orco_ = false;
	}
	if(base_mesh_object_.hasUv())
	{
		const int uvi_1 = vertex_uvs_[0], uvi_2 = vertex_uvs_[1], uvi_3 = vertex_uvs_[2];
		const auto &it = base_mesh_object_.getUvValues().begin();
		sp.u_ = barycentric_u * it[uvi_1].u_ + barycentric_v * it[uvi_2].u_ + barycentric_w * it[uvi_3].u_;
		sp.v_ = barycentric_u * it[uvi_1].v_ + barycentric_v * it[uvi_2].v_ + barycentric_w * it[uvi_3].v_;

		// calculate dPdU and dPdV
		const float du_1 = it[uvi_1].u_ - it[uvi_3].u_;
		const float du_2 = it[uvi_2].u_ - it[uvi_3].u_;
		const float dv_1 = it[uvi_1].v_ - it[uvi_3].v_;
		const float dv_2 = it[uvi_2].v_ - it[uvi_3].v_;
		const float det = du_1 * dv_2 - dv_1 * du_2;

		const std::vector<Point3> vert = getVertices();
		if(std::abs(det) > 1e-30f)
		{
			const float invdet = 1.f / det;
			const Vec3 dp_1 = vert[0] - vert[2];
			const Vec3 dp_2 = vert[1] - vert[2];
			sp.dp_du_ = (dv_2 * invdet) * dp_1 - (dv_1 * invdet) * dp_2;
			sp.dp_dv_ = (du_1 * invdet) * dp_2 - (du_2 * invdet) * dp_1;
		}
		else
		{
			// implicit mapping, p0 = 0/0, p1 = 1/0, p2 = 0/1 => sp.u_ = barycentric_u, sp.v_ = barycentric_v; (arbitrary choice)
			sp.dp_du_ = vert[1] - vert[0];
			sp.dp_dv_ = vert[2] - vert[0];
			sp.u_ = barycentric_u;
			sp.v_ = barycentric_v;
		}
	}
	else
	{
		// implicit mapping, p0 = 0/0, p1 = 1/0, p2 = 0/1 => sp.u_ = barycentric_u, sp.v_ = barycentric_v; (arbitrary choice)
		const std::vector<Point3> vert = getVertices();
		sp.dp_du_ = vert[1] - vert[0];
		sp.dp_dv_ = vert[2] - vert[0];
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
	sp.light_ = base_mesh_object_.getLight();
	sp.has_uv_ = base_mesh_object_.hasUv();
	sp.prim_num_ = getSelfIndex();
	Vec3::createCs(sp.n_, sp.nu_, sp.nv_);
	sp.ray_differentials_ = ray_differentials;
	sp.material_ = getMaterial();
	sp.mat_data_ = sp.material_->initBsdf(sp, camera);
	return sp;
}

END_YAFARAY
