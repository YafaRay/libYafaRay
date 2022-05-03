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

//#include "yafaray_config.h"
#include "geometry/primitive/primitive_triangle.h"
#include "geometry/object/object_mesh.h"
#include "geometry/axis.h"
#include "geometry/ray.h"
#include "geometry/bound.h"
#include "geometry/surface.h"
#include "geometry/uv.h"
#include "geometry/matrix4.h"
#include "geometry/poly_double.h"
#include "common/param.h"
#include <cstring>
#include <memory>

BEGIN_YAFARAY

std::unique_ptr<const SurfacePoint> TrianglePrimitive::getSurface(const RayDifferentials *ray_differentials, const Point3 &hit_point, const IntersectData &intersect_data, const Matrix4 *obj_to_world, const Camera *camera) const
{
	auto sp = std::make_unique<SurfacePoint>();
	sp->intersect_data_ = intersect_data;
	sp->ng_ = Primitive::getGeometricFaceNormal(obj_to_world);
	const float barycentric_u = intersect_data.barycentric_u_, barycentric_v = intersect_data.barycentric_v_, barycentric_w = intersect_data.barycentric_w_;
	if(base_mesh_object_.isSmooth() || base_mesh_object_.hasVerticesNormals())
	{
		const std::array<Vec3, 3> v {
			getVertexNormal(0, sp->ng_, obj_to_world),
			getVertexNormal(1, sp->ng_, obj_to_world),
			getVertexNormal(2, sp->ng_, obj_to_world)
		};
		sp->n_ = barycentric_u * v[0] + barycentric_v * v[1] + barycentric_w * v[2];
		sp->n_.normalize();
	}
	else sp->n_ = sp->ng_;
	if(base_mesh_object_.hasOrco())
	{
		const std::array<Point3, 3> orco_p { getOrcoVertex(0), getOrcoVertex(1), getOrcoVertex(2) };

		sp->orco_p_ = barycentric_u * orco_p[0] + barycentric_v * orco_p[1] + barycentric_w * orco_p[2];
		sp->orco_ng_ = ((orco_p[1] - orco_p[0]) ^ (orco_p[2] - orco_p[0])).normalize();
		sp->has_orco_ = true;
	}
	else
	{
		sp->orco_p_ = hit_point;
		sp->has_orco_ = false;
		sp->orco_ng_ = Primitive::getGeometricFaceNormal();
	}
	bool implicit_uv = true;
	const std::array<Point3, 3> p {
		obj_to_world ?
		std::array<Point3, 3>{ getVertex(0, obj_to_world), getVertex(1, obj_to_world), getVertex(2, obj_to_world) }
		:
		std::array<Point3, 3>{ getVertex(0), getVertex(1), getVertex(2) }
	};
	if(base_mesh_object_.hasUv())
	{
		const std::array<Uv, 3> uv { getVertexUv(0), getVertexUv(1), getVertexUv(2) };
		sp->u_ = barycentric_u * uv[0].u_ + barycentric_v * uv[1].u_ + barycentric_w * uv[2].u_;
		sp->v_ = barycentric_u * uv[0].v_ + barycentric_v * uv[1].v_ + barycentric_w * uv[2].v_;
		// calculate dPdU and dPdV
		const float du_1 = uv[1].u_ - uv[0].u_;
		const float du_2 = uv[2].u_ - uv[0].u_;
		const float dv_1 = uv[1].v_ - uv[0].v_;
		const float dv_2 = uv[2].v_ - uv[0].v_;
		const float det = du_1 * dv_2 - dv_1 * du_2;
		if(std::abs(det) > 1e-30f)
		{
			const float invdet = 1.f / det;
			const Vec3 dp_1{p[1] - p[0]};
			const Vec3 dp_2{p[2] - p[0]};
			sp->dp_du_ = (dv_2 * dp_1 - dv_1 * dp_2) * invdet;
			sp->dp_dv_ = (du_1 * dp_2 - du_2 * dp_1) * invdet;
			implicit_uv = false;
		}
	}
	if(implicit_uv)
	{
		// implicit mapping, p0 = 0/0, p1 = 1/0, p2 = 0/1 => sp->u_ = barycentric_u, sp->v_ = barycentric_v; (arbitrary choice)
		sp->dp_du_ = p[1] - p[0];
		sp->dp_dv_ = p[2] - p[0];
		sp->u_ = barycentric_u;
		sp->v_ = barycentric_v;
	}
	sp->has_uv_ = !implicit_uv;
	//Copy original dPdU and dPdV before normalization to the "absolute" dPdU and dPdV (for mipmap calculations)
	sp->dp_du_abs_ = sp->dp_du_;
	sp->dp_dv_abs_ = sp->dp_dv_;
	sp->dp_du_.normalize();
	sp->dp_dv_.normalize();
	sp->object_ = &base_mesh_object_;
	sp->light_ = base_mesh_object_.getLight();
	sp->has_uv_ = base_mesh_object_.hasUv();
	sp->prim_num_ = getSelfIndex();
	sp->p_ = hit_point;
	std::tie(sp->nu_, sp->nv_) = Vec3::createCoordsSystem(sp->n_);
	calculateShadingSpace(*sp);
	sp->material_ = getMaterial();
	sp->setRayDifferentials(ray_differentials);
	sp->mat_data_ = std::shared_ptr<const MaterialData>(sp->material_->initBsdf(*sp, camera));
	return sp;
}

void TrianglePrimitive::calculateShadingSpace(SurfacePoint &sp)
{
	// transform dPdU and dPdV in shading space
	sp.ds_du_.x() = sp.nu_ * sp.dp_du_;
	sp.ds_du_.y() = sp.nv_ * sp.dp_du_;
	sp.ds_du_.z() = sp.n_ * sp.dp_du_;
	sp.ds_dv_.x() = sp.nu_ * sp.dp_dv_;
	sp.ds_dv_.y() = sp.nv_ * sp.dp_dv_;
	sp.ds_dv_.z() = sp.n_ * sp.dp_dv_;
}

PolyDouble::ClipResultWithBound TrianglePrimitive::clipToBound(Logger &logger, const std::array<Vec3Double, 2> &bound, const ClipPlane &clip_plane, const PolyDouble &poly, const Matrix4 *obj_to_world) const
{
	if(clip_plane.pos_ != ClipPlane::Pos::None) // re-clip
	{
		const double split = (clip_plane.pos_ == ClipPlane::Pos::Lower) ? bound[0][clip_plane.axis_] : bound[1][clip_plane.axis_];
		PolyDouble::ClipResultWithBound clip_result = PolyDouble::planeClipWithBound(logger, split, clip_plane, poly);
		if(clip_result.clip_result_code_ == PolyDouble::ClipResultWithBound::Correct) return clip_result;
		else if(clip_result.clip_result_code_ == PolyDouble::ClipResultWithBound::NoOverlapDisappeared) return PolyDouble::ClipResultWithBound(PolyDouble::ClipResultWithBound::NoOverlapDisappeared);
		//else: do initial clipping below, if there are any other PolyDouble::ClipResult results (errors)
	}
	// initial clip
	const std::array<Point3, 3> triangle_vertices {
		obj_to_world ?
		std::array<Point3, 3>{ getVertex(0, obj_to_world), getVertex(1, obj_to_world), getVertex(2, obj_to_world) }
		:
		std::array<Point3, 3>{ getVertex(0), getVertex(1), getVertex(2) }
	};
	PolyDouble poly_triangle;
	for(const auto &vert : triangle_vertices) poly_triangle.addVertex({vert.x(), vert.y(), vert.z() });
	return PolyDouble::boxClip(logger, bound[1], poly_triangle, bound[0]);
}

END_YAFARAY
