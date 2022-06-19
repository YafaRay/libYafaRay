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
#include "geometry/primitive/primitive_triangle_bezier.h"
#include "geometry/surface.h"

namespace yafaray {

std::unique_ptr<const SurfacePoint> TriangleBezierPrimitive::getSurface(const RayDifferentials *ray_differentials, const Point3 &hit_point, float time, const Uv<float> &intersect_uv, const Camera *camera) const
{
	return getSurfaceTriangleBezier(ray_differentials, hit_point, time, intersect_uv, camera);
}

std::unique_ptr<const SurfacePoint> TriangleBezierPrimitive::getSurface(const RayDifferentials *ray_differentials, const Point3 &hit_point, float time, const Uv<float> &intersect_uv, const Camera *camera, const Matrix4 &obj_to_world) const
{
	return getSurfaceTriangleBezier(ray_differentials, hit_point, time, intersect_uv, camera, obj_to_world);
}

template<typename T>
std::unique_ptr<const SurfacePoint> TriangleBezierPrimitive::getSurfaceTriangleBezier(const RayDifferentials *ray_differentials, const Point3 &hit_point, float time, const Uv<float> &intersect_uv, const Camera *camera, const T &obj_to_world) const
{
	auto sp = std::make_unique<SurfacePoint>(this);
	sp->time_ = time;
	sp->ng_ = getGeometricNormal({}, time, obj_to_world);
	const auto [barycentric_u, barycentric_v, barycentric_w] = ShapeTriangle::getBarycentricUVW(intersect_uv);
	if(base_mesh_object_.isSmooth() || base_mesh_object_.hasVerticesNormals(0))
	{
		const std::array<Vec3, 3> v {getVerticesNormals(0, sp->ng_, obj_to_world)};
		sp->n_ = barycentric_u * v[0] + barycentric_v * v[1] + barycentric_w * v[2];
		sp->n_.normalize();
	}
	else sp->n_ = sp->ng_;
	sp->has_orco_ = base_mesh_object_.hasOrco(0);
	if(sp->has_orco_)
	{
		const std::array<Point3, 3> orco_p {getOrcoVertices(0)};
		sp->orco_p_ = barycentric_u * orco_p[0] + barycentric_v * orco_p[1] + barycentric_w * orco_p[2];
		sp->orco_ng_ = ((orco_p[1] - orco_p[0]) ^ (orco_p[2] - orco_p[0])).normalize();
	}
	else
	{
		sp->orco_p_ = hit_point;
		sp->orco_ng_ = getGeometricNormal({}, time, false);
	}
	bool implicit_uv = true;
	const std::array<Point3, 3> p {getVerticesAsArray(0, obj_to_world)};
	if(base_mesh_object_.hasUv())
	{
		const std::array<Uv<float>, 3> uv {getUvs() };
		sp->uv_ = barycentric_u * uv[0] + barycentric_v * uv[1] + barycentric_w * uv[2];
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
		sp->uv_ = {barycentric_u, barycentric_v};
	}
	sp->p_ = hit_point;
	sp->has_uv_ = base_mesh_object_.hasUv();
	sp->differentials_ = sp->calcSurfaceDifferentials(ray_differentials);
	//Copy original dPdU and dPdV before normalization to the "absolute" dPdU and dPdV (for mipmap calculations)
	sp->dp_abs_ = sp->dp_;
	sp->dp_.u_.normalize();
	sp->dp_.v_.normalize();
	sp->uvn_ = Vec3::createCoordsSystem(sp->n_);
	// transform dPdU and dPdV in shading space
	sp->ds_ = {
			{sp->uvn_.u_ * sp->dp_.u_, sp->uvn_.v_ * sp->dp_.u_, sp->n_ * sp->dp_.u_},
			{sp->uvn_.u_ * sp->dp_.v_, sp->uvn_.v_ * sp->dp_.v_, sp->n_ * sp->dp_.v_}
	};
	sp->mat_data_ = std::unique_ptr<const MaterialData>(sp->getMaterial()->initBsdf(*sp, camera));
	return sp;
}

} //namespace yafaray
