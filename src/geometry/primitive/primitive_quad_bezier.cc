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

#include "geometry/primitive/primitive_quad_bezier.h"
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

std::unique_ptr<const SurfacePoint> QuadBezierPrimitive::getSurface(const RayDifferentials *ray_differentials, const Point3 &hit_point, float time, const Uv<float> &intersect_uv, const Camera *camera) const
{
	auto sp = std::make_unique<SurfacePoint>(this);
	sp->time_ = time;
	sp->ng_ = Primitive::getGeometricNormal(time);
	if(base_mesh_object_.isSmooth() || base_mesh_object_.hasVerticesNormals(0))
	{
		const std::array<Vec3, 4> v {
				getVertexNormal(0, sp->ng_, 0),
				getVertexNormal(1, sp->ng_, 0),
				getVertexNormal(2, sp->ng_, 0),
				getVertexNormal(3, sp->ng_, 0)};
		sp->n_ = ShapeQuad::interpolate(intersect_uv, v);
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
		sp->orco_p_ = ShapeQuad::interpolate(intersect_uv, orco_p);
		sp->orco_ng_ = ((orco_p[1] - orco_p[0]) ^ (orco_p[2] - orco_p[0])).normalize();
		sp->has_orco_ = true;
	}
	else
	{
		sp->orco_p_ = hit_point;
		sp->has_orco_ = false;
		sp->orco_ng_ = Primitive::getGeometricNormal(time);
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
		const std::array<Uv<float>, 4> uv {
				getVertexUv(0),
				getVertexUv(1),
				getVertexUv(2),
				getVertexUv(3)
		};
		sp->uv_ = ShapeQuad::interpolate(intersect_uv, uv);
		// calculate dPdU and dPdV
		const Uv<float> d_1 = uv[1] - uv[0];
		const Uv<float> d_2 = uv[2] - uv[0];
		const float det = d_1.u_ * d_2.v_ - d_1.v_ * d_2.u_;
		if(std::abs(det) > 1e-30f)
		{
			const float invdet = 1.f / det;
			const Vec3 dp_1{p[1] - p[0]};
			const Vec3 dp_2{p[2] - p[0]};
			sp->dp_ = {
					(d_2.v_ * dp_1 - d_1.v_ * dp_2) * invdet,
					(d_1.u_ * dp_2 - d_2.u_ * dp_1) * invdet
			};
			implicit_uv = false;
		}
	}
	if(implicit_uv)
	{
		// implicit mapping, p0 = 0/0, p1 = 1/0, p2 = 0/1 => sp->u_ = barycentric_u, sp->v_ = barycentric_v; (arbitrary choice)
		sp->dp_ = { p[1] - p[0], p[2] - p[0]};
		sp->uv_ = intersect_uv;
	}
	//Copy original dPdU and dPdV before normalization to the "absolute" dPdU and dPdV (for mipmap calculations)
	sp->dp_abs_ = sp->dp_;
	sp->dp_.u_.normalize();
	sp->dp_.v_.normalize();
	sp->has_uv_ = base_mesh_object_.hasUv();
	sp->p_ = hit_point;
	sp->uvn_ = Vec3::createCoordsSystem(sp->n_);
	// transform dPdU and dPdV in shading space
	sp->ds_ = {
			{sp->uvn_.u_ * sp->dp_.u_, sp->uvn_.v_ * sp->dp_.u_, sp->n_ * sp->dp_.u_},
			{sp->uvn_.u_ * sp->dp_.v_, sp->uvn_.v_ * sp->dp_.v_, sp->n_ * sp->dp_.v_}
	};
	sp->differentials_ = sp->calcSurfaceDifferentials(ray_differentials);
	sp->mat_data_ = std::unique_ptr<const MaterialData>(sp->getMaterial()->initBsdf(*sp, camera));
	return sp;
}

std::unique_ptr<const SurfacePoint> QuadBezierPrimitive::getSurface(const RayDifferentials *ray_differentials, const Point3 &hit_point, float time, const Uv<float> &intersect_uv, const Matrix4 &obj_to_world, const Camera *camera) const
{
	auto sp = std::make_unique<SurfacePoint>(this);
	sp->time_ = time;
	sp->ng_ = Primitive::getGeometricNormal(obj_to_world, time);
	if(base_mesh_object_.isSmooth() || base_mesh_object_.hasVerticesNormals(0))
	{
		const std::array<Vec3, 4> v {
				getVertexNormal(0, sp->ng_, obj_to_world, 0),
				getVertexNormal(1, sp->ng_, obj_to_world, 0),
				getVertexNormal(2, sp->ng_, obj_to_world, 0),
				getVertexNormal(3, sp->ng_, obj_to_world, 0)};
		sp->n_ = ShapeQuad::interpolate(intersect_uv, v);
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
		sp->orco_p_ = ShapeQuad::interpolate(intersect_uv, orco_p);
		sp->orco_ng_ = ((orco_p[1] - orco_p[0]) ^ (orco_p[2] - orco_p[0])).normalize();
		sp->has_orco_ = true;
	}
	else
	{
		sp->orco_p_ = hit_point;
		sp->has_orco_ = false;
		sp->orco_ng_ = Primitive::getGeometricNormal(time);
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
		const std::array<Uv<float>, 4> uv {
				getVertexUv(0),
				getVertexUv(1),
				getVertexUv(2),
				getVertexUv(3)
		};
		sp->uv_ = ShapeQuad::interpolate(intersect_uv, uv);
		// calculate dPdU and dPdV
		const Uv<float> d_1 = uv[1] - uv[0];
		const Uv<float> d_2 = uv[2] - uv[0];
		const float det = d_1.u_ * d_2.v_ - d_1.v_ * d_2.u_;
		if(std::abs(det) > 1e-30f)
		{
			const float invdet = 1.f / det;
			const Vec3 dp_1{p[1] - p[0]};
			const Vec3 dp_2{p[2] - p[0]};
			sp->dp_ = {
					(d_2.v_ * dp_1 - d_1.v_ * dp_2) * invdet,
					(d_1.u_ * dp_2 - d_2.u_ * dp_1) * invdet
			};
			implicit_uv = false;
		}
	}
	if(implicit_uv)
	{
		// implicit mapping, p0 = 0/0, p1 = 1/0, p2 = 0/1 => sp->u_ = barycentric_u, sp->v_ = barycentric_v; (arbitrary choice)
		sp->dp_ = { p[1] - p[0], p[2] - p[0]};
		sp->uv_ = intersect_uv;
	}
	//Copy original dPdU and dPdV before normalization to the "absolute" dPdU and dPdV (for mipmap calculations)
	sp->dp_abs_ = sp->dp_;
	sp->dp_.u_.normalize();
	sp->dp_.v_.normalize();
	sp->has_uv_ = base_mesh_object_.hasUv();
	sp->p_ = hit_point;
	sp->uvn_ = Vec3::createCoordsSystem(sp->n_);
	// transform dPdU and dPdV in shading space
	sp->ds_ = {
			{sp->uvn_.u_ * sp->dp_.u_, sp->uvn_.v_ * sp->dp_.u_, sp->n_ * sp->dp_.u_},
			{sp->uvn_.u_ * sp->dp_.v_, sp->uvn_.v_ * sp->dp_.v_, sp->n_ * sp->dp_.v_}
	};
	sp->differentials_ = sp->calcSurfaceDifferentials(ray_differentials);
	sp->mat_data_ = std::unique_ptr<const MaterialData>(sp->getMaterial()->initBsdf(*sp, camera));
	return sp;
}

END_YAFARAY
