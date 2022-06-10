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

#include "geometry/primitive/primitive_quad.h"
#include "common/param.h"
#include "geometry/bound.h"
#include "geometry/clip_plane.h"
#include "geometry/matrix4.h"
#include "geometry/object/object_mesh.h"
#include "geometry/poly_double.h"
#include "geometry/ray.h"
#include "geometry/surface.h"
#include "geometry/uv.h"
#include <memory>

BEGIN_YAFARAY

std::unique_ptr<const SurfacePoint> QuadPrimitive::getSurface(const RayDifferentials *ray_differentials, const Point3 &hit_point, const IntersectData &intersect_data, const Camera *camera) const
{
	auto sp = std::make_unique<SurfacePoint>(this);
	sp->intersect_data_ = intersect_data;
	sp->ng_ = getGeometricNormal();
	if(base_mesh_object_.isSmooth() || base_mesh_object_.hasVerticesNormals(0))
	{
		const std::array<Vec3, 4> v {
				getVertexNormal(0, sp->ng_, 0),
				getVertexNormal(1, sp->ng_, 0),
				getVertexNormal(2, sp->ng_, 0),
				getVertexNormal(3, sp->ng_, 0)};
		sp->n_ = ShapeQuad::interpolate(intersect_data.uv(), v);
		sp->n_.normalize();
	}
	else sp->n_ = sp->ng_;
	if(base_mesh_object_.hasOrco(0))
	{
		const std::array<Point3, 4> orco_p {
				getOrcoVertex(0, 0),
				getOrcoVertex(1, 0),
				getOrcoVertex(2, 0),
				getOrcoVertex(3, 0)};
		sp->orco_p_ = ShapeQuad::interpolate(intersect_data.uv(), orco_p);
		sp->orco_ng_ = ((orco_p[1] - orco_p[0]) ^ (orco_p[2] - orco_p[0])).normalize();
		sp->has_orco_ = true;
	}
	else
	{
		sp->orco_p_ = hit_point;
		sp->has_orco_ = false;
		sp->orco_ng_ = getGeometricNormal();
	}
	bool implicit_uv = true;
	const std::array<Point3, 4> p {
			getVertex(0, 0),
			getVertex(1, 0),
			getVertex(2, 0),
			getVertex(3, 0),
	};
	if(base_mesh_object_.hasUv())
	{
		const std::array<Uv, 4> uv {
				getVertexUv(0),
				getVertexUv(1),
				getVertexUv(2),
				getVertexUv(3)
		};
		sp->uv_ = ShapeQuad::interpolate(intersect_data.uv(), uv);
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
		sp->uv_ = intersect_data.uv();
	}
	//Copy original dPdU and dPdV before normalization to the "absolute" dPdU and dPdV (for mipmap calculations)
	sp->dp_du_abs_ = sp->dp_du_;
	sp->dp_dv_abs_ = sp->dp_dv_;
	sp->dp_du_.normalize();
	sp->dp_dv_.normalize();
	sp->has_uv_ = base_mesh_object_.hasUv();
	sp->p_ = hit_point;
	std::tie(sp->nu_, sp->nv_) = Vec3::createCoordsSystem(sp->n_);
	// transform dPdU and dPdV in shading space
	sp->ds_du_ = {sp->nu_ * sp->dp_du_, sp->nv_ * sp->dp_du_, sp->n_ * sp->dp_du_};
	sp->ds_dv_ = {sp->nu_ * sp->dp_dv_, sp->nv_ * sp->dp_dv_, sp->n_ * sp->dp_dv_};
	sp->differentials_ = sp->calcSurfaceDifferentials(ray_differentials);
	sp->mat_data_ = std::unique_ptr<const MaterialData>(sp->getMaterial()->initBsdf(*sp, camera));
	return sp;
}

std::unique_ptr<const SurfacePoint> QuadPrimitive::getSurface(const RayDifferentials *ray_differentials, const Point3 &hit_point, const IntersectData &intersect_data, const Matrix4 &obj_to_world, const Camera *camera) const
{
	auto sp = std::make_unique<SurfacePoint>(this);
	sp->intersect_data_ = intersect_data;
	sp->ng_ = getGeometricNormal(obj_to_world);
	if(base_mesh_object_.isSmooth() || base_mesh_object_.hasVerticesNormals(0))
	{
		const std::array<Vec3, 4> v {
				getVertexNormal(0, sp->ng_, obj_to_world, 0),
				getVertexNormal(1, sp->ng_, obj_to_world, 0),
				getVertexNormal(2, sp->ng_, obj_to_world, 0),
				getVertexNormal(3, sp->ng_, obj_to_world, 0)};
		sp->n_ = ShapeQuad::interpolate(intersect_data.uv(), v);
		sp->n_.normalize();
	}
	else sp->n_ = sp->ng_;
	if(base_mesh_object_.hasOrco(0))
	{
		const std::array<Point3, 4> orco_p {
				getOrcoVertex(0, 0),
				getOrcoVertex(1, 0),
				getOrcoVertex(2, 0),
				getOrcoVertex(3, 0)};
		sp->orco_p_ = ShapeQuad::interpolate(intersect_data.uv(), orco_p);
		sp->orco_ng_ = ((orco_p[1] - orco_p[0]) ^ (orco_p[2] - orco_p[0])).normalize();
		sp->has_orco_ = true;
	}
	else
	{
		sp->orco_p_ = hit_point;
		sp->has_orco_ = false;
		sp->orco_ng_ = getGeometricNormal();
	}
	bool implicit_uv = true;
	const std::array<Point3, 4> p {
			getVertex(0, obj_to_world, 0),
			getVertex(1, obj_to_world, 0),
			getVertex(2, obj_to_world, 0),
			getVertex(3, obj_to_world, 0),
	};
	if(base_mesh_object_.hasUv())
	{
		const std::array<Uv, 4> uv {
				getVertexUv(0),
				getVertexUv(1),
				getVertexUv(2),
				getVertexUv(3)
		};
		sp->uv_ = ShapeQuad::interpolate(intersect_data.uv(), uv);
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
		sp->uv_ = intersect_data.uv();
	}
	//Copy original dPdU and dPdV before normalization to the "absolute" dPdU and dPdV (for mipmap calculations)
	sp->dp_du_abs_ = sp->dp_du_;
	sp->dp_dv_abs_ = sp->dp_dv_;
	sp->dp_du_.normalize();
	sp->dp_dv_.normalize();
	sp->has_uv_ = base_mesh_object_.hasUv();
	sp->p_ = hit_point;
	std::tie(sp->nu_, sp->nv_) = Vec3::createCoordsSystem(sp->n_);
	// transform dPdU and dPdV in shading space
	sp->ds_du_ = {sp->nu_ * sp->dp_du_, sp->nv_ * sp->dp_du_, sp->n_ * sp->dp_du_};
	sp->ds_dv_ = {sp->nu_ * sp->dp_dv_, sp->nv_ * sp->dp_dv_, sp->n_ * sp->dp_dv_};
	sp->differentials_ = sp->calcSurfaceDifferentials(ray_differentials);
	sp->mat_data_ = std::unique_ptr<const MaterialData>(sp->getMaterial()->initBsdf(*sp, camera));
	return sp;
}

PolyDouble::ClipResultWithBound QuadPrimitive::clipToBound(Logger &logger, const std::array<Vec3Double, 2> &bound, const ClipPlane &clip_plane, const PolyDouble &poly) const
{
	if(clip_plane.pos_ != ClipPlane::Pos::None) // re-clip
	{
		const double split = (clip_plane.pos_ == ClipPlane::Pos::Lower) ? bound[0][clip_plane.axis_] : bound[1][clip_plane.axis_];
		PolyDouble::ClipResultWithBound clip_result = PolyDouble::planeClipWithBound(logger, split, clip_plane, poly);
		if(clip_result.clip_result_code_ == PolyDouble::ClipResultWithBound::Code::Correct) return clip_result;
		else if(clip_result.clip_result_code_ == PolyDouble::ClipResultWithBound::Code::NoOverlapDisappeared) return PolyDouble::ClipResultWithBound(PolyDouble::ClipResultWithBound::Code::NoOverlapDisappeared);
		//else: do initial clipping below, if there are any other PolyDouble::ClipResult results (errors)
	}
	// initial clip
	const std::array<Point3, 4> vertices{
			getVertex(0, 0),
			getVertex(1, 0),
			getVertex(2, 0),
			getVertex(3, 0),
	};
	PolyDouble poly_triangle;
	for(const auto &vert : vertices) poly_triangle.addVertex({vert.x(), vert.y(), vert.z() });
	return PolyDouble::boxClip(logger, bound[1], poly_triangle, bound[0]);
}

PolyDouble::ClipResultWithBound QuadPrimitive::clipToBound(Logger &logger, const std::array<Vec3Double, 2> &bound, const ClipPlane &clip_plane, const PolyDouble &poly, const Matrix4 &obj_to_world) const
{
	if(clip_plane.pos_ != ClipPlane::Pos::None) // re-clip
	{
		const double split = (clip_plane.pos_ == ClipPlane::Pos::Lower) ? bound[0][clip_plane.axis_] : bound[1][clip_plane.axis_];
		PolyDouble::ClipResultWithBound clip_result = PolyDouble::planeClipWithBound(logger, split, clip_plane, poly);
		if(clip_result.clip_result_code_ == PolyDouble::ClipResultWithBound::Code::Correct) return clip_result;
		else if(clip_result.clip_result_code_ == PolyDouble::ClipResultWithBound::Code::NoOverlapDisappeared) return PolyDouble::ClipResultWithBound(PolyDouble::ClipResultWithBound::Code::NoOverlapDisappeared);
		//else: do initial clipping below, if there are any other PolyDouble::ClipResult results (errors)
	}
	// initial clip
	const std::array<Point3, 4> vertices{
			getVertex(0, obj_to_world, 0),
			getVertex(1, obj_to_world, 0),
			getVertex(2, obj_to_world, 0),
			getVertex(3, obj_to_world, 0),
	};
	PolyDouble poly_triangle;
	for(const auto &vert : vertices) poly_triangle.addVertex({vert.x(), vert.y(), vert.z() });
	return PolyDouble::boxClip(logger, bound[1], poly_triangle, bound[0]);
}

END_YAFARAY
