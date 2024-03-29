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

#include "geometry/primitive/primitive_polygon.h"
#include "geometry/surface.h"
#include "geometry/clip_plane.h"

namespace yafaray {

template <typename T, size_t N, MotionBlurType MotionBlur>
std::unique_ptr<const SurfacePoint> PrimitivePolygon<T, N, MotionBlur>::getSurface(const RayDifferentials *ray_differentials, const Point<T, 3> &hit_point, T time, const Uv<T> &intersect_uv, const Camera *camera) const
{
	return getSurfacePolygon(ray_differentials, hit_point, time, intersect_uv, camera);
}

template <typename T, size_t N, MotionBlurType MotionBlur>
std::unique_ptr<const SurfacePoint> PrimitivePolygon<T, N, MotionBlur>::getSurface(const RayDifferentials *ray_differentials, const Point<T, 3> &hit_point, T time, const Uv<T> &intersect_uv, const Camera *camera, const SquareMatrix<T, 4> &obj_to_world) const
{
	return getSurfacePolygon(ray_differentials, hit_point, time, intersect_uv, camera, obj_to_world);
}

template <typename T, size_t N, MotionBlurType MotionBlur>
template<typename M>
std::unique_ptr<const SurfacePoint> PrimitivePolygon<T, N, MotionBlur>::getSurfacePolygon(const RayDifferentials *ray_differentials, const Point<T, 3> &hit_point, T time, const Uv<T> &intersect_uv, const Camera *camera, const M &obj_to_world) const
{
	auto sp{std::make_unique<SurfacePoint>(this)};
	sp->time_ = time;
	sp->ng_ = getGeometricNormal({}, time, obj_to_world);
	T barycentric_u, barycentric_v, barycentric_w;
	if constexpr(N == 3)
	{
		std::tie(barycentric_u, barycentric_v, barycentric_w) = ShapePolygon<T, N>::getTriangleBarycentricUVW(intersect_uv);
	}
	if(base_mesh_object_.isSmooth() || base_mesh_object_.hasVerticesNormals(0))
	{
		const std::array<Vec<T, 3>, N> v {getVerticesNormals(0, sp->ng_, obj_to_world)};
		if constexpr(N == 3) sp->n_ = barycentric_u * v[0] + barycentric_v * v[1] + barycentric_w * v[2];
		else sp->n_ = ShapePolygon<T, N>::interpolate(intersect_uv, v);
		sp->n_.normalize();
	}
	else sp->n_ = sp->ng_;
	sp->has_orco_ = base_mesh_object_.hasOrco(0);
	if(sp->has_orco_)
	{
		const std::array<Point<T, 3>, N> orco_p {getOrcoVertices(0)};
		if constexpr(N == 3) sp->orco_p_ = barycentric_u * orco_p[0] + barycentric_v * orco_p[1] + barycentric_w * orco_p[2];
		else sp->orco_p_ = ShapePolygon<T, N>::interpolate(intersect_uv, orco_p);
		sp->orco_ng_ = ((orco_p[1] - orco_p[0]) ^ (orco_p[2] - orco_p[0])).normalize();
	}
	else
	{
		sp->orco_p_ = hit_point;
		sp->orco_ng_ = getGeometricNormal({}, time, false);
	}
	bool implicit_uv = true;
	const std::array<Point<T, 3>, N> p {getVerticesAsArray(0, obj_to_world)};
	if(base_mesh_object_.hasUv())
	{
		const std::array<Uv<T>, N> uv {getVerticesUvs()};
		if constexpr(N == 3) sp->uv_ = barycentric_u * uv[0] + barycentric_v * uv[1] + barycentric_w * uv[2];
		else sp->uv_ = ShapePolygon<T, N>::interpolate(intersect_uv, uv);
		// calculate dPdU and dPdV
		const Uv<T> d_1 = uv[1] - uv[0];
		const Uv<T> d_2 = uv[2] - uv[0];
		const T det = d_1.u_ * d_2.v_ - d_1.v_ * d_2.u_;
		if(std::abs(det) > T{1e-30})
		{
			const T invdet = T{1} / det;
			const Vec<T, 3> dp_1{p[1] - p[0]};
			const Vec<T, 3> dp_2{p[2] - p[0]};
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
		if constexpr(N == 3) sp->uv_ = {barycentric_u, barycentric_v};
		else sp->uv_ = intersect_uv;
	}
	sp->p_ = hit_point;
	sp->has_uv_ = base_mesh_object_.hasUv();
	sp->differentials_ = sp->calcSurfaceDifferentials(ray_differentials);
	//Copy original dPdU and dPdV before normalization to the "absolute" dPdU and dPdV (for mipmap calculations)
	sp->dp_abs_ = sp->dp_;
	sp->dp_.u_.normalize();
	sp->dp_.v_.normalize();
	sp->uvn_ = Vec<T, 3>::createCoordsSystem(sp->n_);
	// transform dPdU and dPdV in shading space
	sp->ds_ = {
			{{sp->uvn_.u_ * sp->dp_.u_, sp->uvn_.v_ * sp->dp_.u_, sp->n_ * sp->dp_.u_}},
			{{sp->uvn_.u_ * sp->dp_.v_, sp->uvn_.v_ * sp->dp_.v_, sp->n_ * sp->dp_.v_}}
	};
	sp->mat_data_ = sp->getMaterial()->initBsdf(*sp, camera);
	return sp;
}

template <typename T, size_t N, MotionBlurType MotionBlur>
PolyDouble::ClipResultWithBound PrimitivePolygon<T, N, MotionBlur>::clipToBound(Logger &logger, const std::array<Vec3d, 2> &bound, const ClipPlane &clip_plane, const PolyDouble &poly) const
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
	const std::array<Point<T, 3>, N> vertices{getVerticesAsArray(0)};
	PolyDouble poly_triangle;
	for(const auto &vert : vertices) poly_triangle.addVertex({{vert[Axis::X], vert[Axis::Y], vert[Axis::Z]}});
	return PolyDouble::boxClip(logger, bound[1], poly_triangle, bound[0]);
}

template <typename T, size_t N, MotionBlurType MotionBlur>
PolyDouble::ClipResultWithBound PrimitivePolygon<T, N, MotionBlur>::clipToBound(Logger &logger, const std::array<Vec3d, 2> &bound, const ClipPlane &clip_plane, const PolyDouble &poly, const SquareMatrix<T, 4> &obj_to_world) const
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
	const std::array<Point<T, 3>, N> vertices{getVerticesAsArray(0, obj_to_world)};
	PolyDouble poly_triangle;
	for(const auto &vert : vertices) poly_triangle.addVertex({{vert[Axis::X], vert[Axis::Y], vert[Axis::Z]}});
	return PolyDouble::boxClip(logger, bound[1], poly_triangle, bound[0]);
}

//Important: to avoid undefined symbols in CLang, always keep the template explicit specializations **at the end** of the source file!
template class PrimitivePolygon<float, 3, MotionBlurType::None>;
template class PrimitivePolygon<float, 4, MotionBlurType::None>;
template class PrimitivePolygon<float, 3, MotionBlurType::Bezier>;
template class PrimitivePolygon<float, 4, MotionBlurType::Bezier>;

} //namespace yafaray
