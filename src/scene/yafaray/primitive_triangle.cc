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
#include "scene/yafaray/primitive_triangle.h"
#include "scene/yafaray/object_mesh.h"
#include "geometry/axis.h"
#include "geometry/ray.h"
#include "geometry/bound.h"
#include "geometry/surface.h"
#include "geometry/uv.h"
#include "geometry/matrix4.h"
#include "geometry/poly_double.h"
#include "common/param.h"
#include <string.h>

BEGIN_YAFARAY

TrianglePrimitive::TrianglePrimitive(const std::vector<int> &vertices_indices, const std::vector<int> &vertices_uv_indices, const MeshObject &mesh_object) : FacePrimitive(vertices_indices, vertices_uv_indices, mesh_object)
{
	calculateGeometricNormal();
}

IntersectData TrianglePrimitive::intersect(const Ray &ray, const Matrix4 *obj_to_world) const
{
	return TrianglePrimitive::intersect(ray, { getVertex(0, obj_to_world), getVertex(1, obj_to_world), getVertex(2, obj_to_world) });
}

IntersectData TrianglePrimitive::intersect(const Ray &ray, const std::array<Point3, 3> &vertices)
{
	//Tomas Moller and Ben Trumbore ray intersection scheme
	const Vec3 edge_1 = vertices[1] - vertices[0];
	const Vec3 edge_2 = vertices[2] - vertices[0];
	const float epsilon = 0.1f * min_raydist_global * std::max(edge_1.length(), edge_2.length());
	const Vec3 pvec = ray.dir_ ^ edge_2;
	const float det = edge_1 * pvec;
	if(det > -epsilon && det < epsilon) return {};
	const float inv_det = 1.f / det;
	const Vec3 tvec = ray.from_ - vertices[0];
	const float u = (tvec * pvec) * inv_det;
	if(u < 0.f || u > 1.f) return {};
	const Vec3 qvec = tvec ^ edge_1;
	const float v = (ray.dir_ * qvec) * inv_det;
	if((v < 0.f) || ((u + v) > 1.f)) return {};
	const float t = edge_2 * qvec * inv_det;
	if(t < epsilon) return {};
	IntersectData intersect_data;
	intersect_data.hit_ = true;
	intersect_data.t_hit_ = t;
	//UV <-> Barycentric UVW relationship is not obvious, interesting explanation in: https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/barycentric-coordinates
	intersect_data.barycentric_u_ = 1.f - u - v;
	intersect_data.barycentric_v_ = u;
	intersect_data.barycentric_w_ = v;
	intersect_data.time_ = ray.time_;
	return intersect_data;
}

bool TrianglePrimitive::intersectsBound(const ExBound &ex_bound, const Matrix4 *obj_to_world) const
{
	return TrianglePrimitive::intersectsBound(ex_bound, { getVertex(0, obj_to_world), getVertex(1, obj_to_world), getVertex(2, obj_to_world) });
}

bool TrianglePrimitive::intersectsBound(const ExBound &ex_bound, const std::array<Point3, 3> &vertices)
{
	std::array<Vec3Double, 3> t_points;
	for(size_t i = 0; i < 3; ++i)
		for(size_t j = 0; j < 3; ++j)
			t_points[j][i] = vertices[j][i];
	return triBoxOverlap(ex_bound.center_, ex_bound.half_size_, t_points);
}

void TrianglePrimitive::calculateGeometricNormal()
{
	normal_geometric_ = calculateNormal({ getVertex(0, nullptr), getVertex(1, nullptr), getVertex(2, nullptr) });
}

Vec3 TrianglePrimitive::calculateNormal(const std::array<Point3, 3> &vertices)
{
	return ((vertices[1] - vertices[0]) ^ (vertices[2] - vertices[0])).normalize();
}

SurfacePoint TrianglePrimitive::getSurface(const Point3 &hit_point, const IntersectData &intersect_data, const Matrix4 *obj_to_world) const
{
	SurfacePoint sp;
	sp.intersect_data_ = intersect_data;
	sp.ng_ = getGeometricNormal(obj_to_world);
	const float barycentric_u = intersect_data.barycentric_u_, barycentric_v = intersect_data.barycentric_v_, barycentric_w = intersect_data.barycentric_w_;
	const MeshObject &base_mesh_object = static_cast<const MeshObject &>(base_object_);
	if(base_mesh_object.isSmooth() || base_mesh_object.hasNormalsExported())
	{
		const std::array<Vec3, 3> v {
			getVertexNormal(0, sp.ng_, obj_to_world),
			getVertexNormal(1, sp.ng_, obj_to_world),
			getVertexNormal(2, sp.ng_, obj_to_world)
		};
		sp.n_ = barycentric_u * v[0] + barycentric_v * v[1] + barycentric_w * v[2];
		sp.n_.normalize();
	}
	else sp.n_ = sp.ng_;
	if(base_mesh_object.hasOrco())
	{
		const std::array<Point3, 3> orco_p { getOrcoVertex(0), getOrcoVertex(1), getOrcoVertex(2) };

		sp.orco_p_ = barycentric_u * orco_p[0] + barycentric_v * orco_p[1] + barycentric_w * orco_p[2];
		sp.orco_ng_ = ((orco_p[1] - orco_p[0]) ^ (orco_p[2] - orco_p[0])).normalize();
		sp.has_orco_ = true;
	}
	else
	{
		sp.orco_p_ = hit_point;
		sp.has_orco_ = false;
		sp.orco_ng_ = getGeometricNormal();
	}
	bool implicit_uv = true;
	const std::array<Point3, 3> p { getVertex(0, obj_to_world), getVertex(1, obj_to_world), getVertex(2, obj_to_world) };
	if(base_mesh_object.hasUv())
	{
		const std::array<Uv, 3> uv { getVertexUv(0), getVertexUv(1), getVertexUv(2) };
		sp.u_ = barycentric_u * uv[0].u_ + barycentric_v * uv[1].u_ + barycentric_w * uv[2].u_;
		sp.v_ = barycentric_u * uv[0].v_ + barycentric_v * uv[1].v_ + barycentric_w * uv[2].v_;
		// calculate dPdU and dPdV
		const float du_1 = uv[1].u_ - uv[0].u_;
		const float du_2 = uv[2].u_ - uv[0].u_;
		const float dv_1 = uv[1].v_ - uv[0].v_;
		const float dv_2 = uv[2].v_ - uv[0].v_;
		const float det = du_1 * dv_2 - dv_1 * du_2;
		if(std::abs(det) > 1e-30f)
		{
			const float invdet = 1.f / det;
			const Vec3 dp_1 = p[1] - p[0];
			const Vec3 dp_2 = p[2] - p[0];
			sp.dp_du_ = (dv_2 * dp_1 - dv_1 * dp_2) * invdet;
			sp.dp_dv_ = (du_1 * dp_2 - du_2 * dp_1) * invdet;
			implicit_uv = false;
		}
	}
	if(implicit_uv)
	{
		// implicit mapping, p0 = 0/0, p1 = 1/0, p2 = 0/1 => sp.u_ = barycentric_u, sp.v_ = barycentric_v; (arbitrary choice)
		sp.dp_du_ = p[1] - p[0];
		sp.dp_dv_ = p[2] - p[0];
		sp.u_ = barycentric_u;
		sp.v_ = barycentric_v;
	}
	sp.has_uv_ = !implicit_uv;
	//Copy original dPdU and dPdV before normalization to the "absolute" dPdU and dPdV (for mipmap calculations)
	sp.dp_du_abs_ = sp.dp_du_;
	sp.dp_dv_abs_ = sp.dp_dv_;
	sp.dp_du_.normalize();
	sp.dp_dv_.normalize();
	sp.object_ = &base_object_;
	sp.light_ = base_mesh_object.getLight();
	sp.has_uv_ = base_mesh_object.hasUv();
	sp.prim_num_ = getSelfIndex();
	sp.material_ = getMaterial();
	sp.p_ = hit_point;
	Vec3::createCs(sp.n_, sp.nu_, sp.nv_);
	calculateShadingSpace(sp);
	return sp;
}

void TrianglePrimitive::calculateShadingSpace(SurfacePoint &sp) const
{
	// transform dPdU and dPdV in shading space
	sp.ds_du_.x_ = sp.nu_ * sp.dp_du_;
	sp.ds_du_.y_ = sp.nv_ * sp.dp_du_;
	sp.ds_du_.z_ = sp.n_ * sp.dp_du_;
	sp.ds_dv_.x_ = sp.nu_ * sp.dp_dv_;
	sp.ds_dv_.y_ = sp.nv_ * sp.dp_dv_;
	sp.ds_dv_.z_ = sp.n_ * sp.dp_dv_;
}

PolyDouble::ClipResultWithBound TrianglePrimitive::clipToBound(Logger &logger, const std::array<Vec3Double, 2> &bound, const ClipPlane &clip_plane, const PolyDouble &poly, const Matrix4 *obj_to_world) const
{
	if(clip_plane.pos_ != ClipPlane::Pos::None) // re-clip
	{
		const double split = (clip_plane.pos_ == ClipPlane::Pos::Lower) ? bound[0][clip_plane.axis_] : bound[1][clip_plane.axis_];
		const PolyDouble::ClipResultWithBound clip_result = PolyDouble::planeClipWithBound(logger, split, clip_plane, poly);
		if(clip_result.clip_result_code_ == PolyDouble::ClipResultWithBound::Correct) return clip_result;
		else if(clip_result.clip_result_code_ == PolyDouble::ClipResultWithBound::NoOverlapDisappeared) return {PolyDouble::ClipResultWithBound::NoOverlapDisappeared};
		//else: do initial clipping below, if there are any other PolyDouble::ClipResult results (errors)
	}
	// initial clip
	const std::array<Point3, 3> triangle_vertices {
		getVertex(0, obj_to_world), getVertex(1, obj_to_world), getVertex(2, obj_to_world)
	};
	PolyDouble poly_triangle;
	for(const auto &vert : triangle_vertices) poly_triangle.addVertex({vert.x_, vert.y_, vert.z_ });
	return PolyDouble::boxClip(logger, bound[1], poly_triangle, bound[0]);
}

float TrianglePrimitive::surfaceArea(const std::array<Point3, 3> &vertices)
{
	const Vec3 vec_0_1 = vertices[1] - vertices[0];
	const Vec3 vec_0_2 = vertices[2] - vertices[0];
	return 0.5f * (vec_0_1 ^ vec_0_2).length();
}

float TrianglePrimitive::surfaceArea(const Matrix4 *obj_to_world) const
{
	return surfaceArea({ getVertex(0, obj_to_world), getVertex(1, obj_to_world), getVertex(2, obj_to_world) });
}

void TrianglePrimitive::sample(float s_1, float s_2, Point3 &p, Vec3 &n, const Matrix4 *obj_to_world) const
{
	TrianglePrimitive::sample(s_1, s_2, p, { getVertex(0, obj_to_world), getVertex(1, obj_to_world), getVertex(2, obj_to_world) });
	n = getGeometricNormal(obj_to_world);
}

void TrianglePrimitive::sample(float s_1, float s_2, Point3 &p, const std::array<Point3, 3> &vertices)
{
	const float su_1 = math::sqrt(s_1);
	const float u = 1.f - su_1;
	const float v = s_2 * su_1;
	p = u * vertices[0] + v * vertices[1] + (1.f - u - v) * vertices[2];
}

/*************************************************************
 *      The code below (triBoxOverlap and related functions)
 *      is based on "AABB-triangle overlap test code"
 *          by Tomas Akenine-Möller
 *      (see information below about the original code)
 ************************************************************/
/* AABB-triangle overlap test code                      */
/* by Tomas Akenine-Möller                              */
/* Function: int triBoxOverlap(float boxcenter[3],      */
/*          float boxhalfsize[3],float triverts[3][3]); */
/* History:                                             */
/*   2001-03-05: released the code in its first version */
/*   2001-06-18: changed the order of the tests, faster */
/*                                                      */
/* Acknowledgement: Many thanks to Pierre Terdiman for  */
/* suggestions and discussions on how to optimize code. */
/* Thanks to David Hunt for finding a ">="-bug!         */
/********************************************************/

struct MinMax
{
	double min_;
	double max_;
	static MinMax find(const Vec3Double values);
};

MinMax MinMax::find(const Vec3Double values)
{
	MinMax min_max;
	min_max.min_ = math::min(values[0], values[1], values[2]);
	min_max.max_ = math::max(values[0], values[1], values[2]);
	return min_max;
}

int planeBoxOverlap_global(const Vec3Double &normal, const Vec3Double &vert, const Vec3Double &maxbox)	// -NJMP-
{
	Vec3Double vmin, vmax;
	for(int axis = 0; axis < 3; ++axis)
	{
		const double v = vert[axis];					// -NJMP-
		if(normal[axis] > 0)
		{
			vmin[axis] = -maxbox[axis] - v;	// -NJMP-
			vmax[axis] = maxbox[axis] - v;	// -NJMP-
		}
		else
		{
			vmin[axis] = maxbox[axis] - v;	// -NJMP-
			vmax[axis] = -maxbox[axis] - v;	// -NJMP-
		}
	}
	if(Vec3Double::dot(normal, vmin) > 0) return 0;	// -NJMP-
	if(Vec3Double::dot(normal, vmax) >= 0) return 1;	// -NJMP-

	return 0;
}

bool axisTest_global(double a, double b, double f_a, double f_b, const Vec3Double &v_a, const Vec3Double &v_b, const Vec3Double &boxhalfsize, int axis)
{
	const int axis_a = (axis == Axis::X ? Axis::Y : Axis::X);
	const int axis_b = (axis == Axis::Z ? Axis::Y : Axis::Z);
	const int sign = (axis == Axis::Y ? -1 : 1);
	const double p_a = sign * (a * v_a[axis_a] - b * v_a[axis_b]);
	const double p_b = sign * (a * v_b[axis_a] - b * v_b[axis_b]);
	double min, max;
	if(p_a < p_b)
	{
		min = p_a;
		max = p_b;
	}
	else
	{
		min = p_b;
		max = p_a;
	}
	const double rad = f_a * boxhalfsize[axis_a] + f_b * boxhalfsize[axis_b];
	if(min > rad || max < -rad) return false;
	else return true;
}

bool TrianglePrimitive::triBoxOverlap(const Vec3Double &boxcenter, const Vec3Double &boxhalfsize, const std::array<Vec3Double, 3> &triverts)
{

	/*    use separating axis theorem to test overlap between triangle and box */
	/*    need to test for overlap in these directions: */
	/*    1) the {x,y,z}-directions (actually, since we use the AABB of the triangle */
	/*       we do not even need to test these) */
	/*    2) normal of the triangle */
	/*    3) crossproduct(edge from tri, {x,y,z}-directin) */
	/*       this gives 3x3=9 more tests */
	/* This is the fastest branch on Sun */
	/* move everything so that the boxcenter is in (0,0,0) */
	const std::array<Vec3Double, 3> tri_verts {
			Vec3Double::sub(triverts[0], boxcenter),
			Vec3Double::sub(triverts[1], boxcenter),
			Vec3Double::sub(triverts[2], boxcenter)
	};
	const std::array<Vec3Double, 3> tri_edges {
			Vec3Double::sub(tri_verts[1], tri_verts[0]),
			Vec3Double::sub(tri_verts[2], tri_verts[1]),
			Vec3Double::sub(tri_verts[0], tri_verts[2])
	};
	/* Bullet 3:  */
	/*  test the 9 tests first (this was faster) */
	const std::array<Vec3Double, 3> fe {{
		{std::abs(tri_edges[0][Axis::X]), std::abs(tri_edges[0][Axis::Y]), std::abs(tri_edges[0][Axis::Z])},
		{std::abs(tri_edges[1][Axis::X]), std::abs(tri_edges[1][Axis::Y]), std::abs(tri_edges[1][Axis::Z])},
		{std::abs(tri_edges[2][Axis::X]), std::abs(tri_edges[2][Axis::Y]), std::abs(tri_edges[2][Axis::Z])}
	}};
	if(!axisTest_global(tri_edges[0][Axis::Z], tri_edges[0][Axis::Y], fe[0][Axis::Z], fe[0][Axis::Y], tri_verts[0], tri_verts[2], boxhalfsize, Axis::X)) return false;
	if(!axisTest_global(tri_edges[0][Axis::Z], tri_edges[0][Axis::X], fe[0][Axis::Z], fe[0][Axis::X], tri_verts[0], tri_verts[2], boxhalfsize, Axis::Y)) return false;
	if(!axisTest_global(tri_edges[0][Axis::Y], tri_edges[0][Axis::X], fe[0][Axis::Y], fe[0][Axis::X], tri_verts[1], tri_verts[2], boxhalfsize, Axis::Z)) return false;

	if(!axisTest_global(tri_edges[1][Axis::Z], tri_edges[1][Axis::Y], fe[1][Axis::Z], fe[1][Axis::Y], tri_verts[0], tri_verts[2], boxhalfsize, Axis::X)) return false;
	if(!axisTest_global(tri_edges[1][Axis::Z], tri_edges[1][Axis::X], fe[1][Axis::Z], fe[1][Axis::X], tri_verts[0], tri_verts[2], boxhalfsize, Axis::Y)) return false;
	if(!axisTest_global(tri_edges[1][Axis::Y], tri_edges[1][Axis::X], fe[1][Axis::Y], fe[1][Axis::X], tri_verts[0], tri_verts[1], boxhalfsize, Axis::Z)) return false;

	if(!axisTest_global(tri_edges[2][Axis::Z], tri_edges[2][Axis::Y], fe[2][Axis::Z], fe[2][Axis::Y], tri_verts[0], tri_verts[1], boxhalfsize, Axis::X)) return false;
	if(!axisTest_global(tri_edges[2][Axis::Z], tri_edges[2][Axis::X], fe[2][Axis::Z], fe[2][Axis::X], tri_verts[0], tri_verts[1], boxhalfsize, Axis::Y)) return false;
	if(!axisTest_global(tri_edges[2][Axis::Y], tri_edges[2][Axis::X], fe[2][Axis::Y], fe[2][Axis::X], tri_verts[1], tri_verts[2], boxhalfsize, Axis::Z)) return false;

	/* Bullet 1: */
	/*  first test overlap in the {x,y,z}-directions */
	/*  find min, max of the triangle each direction, and test for overlap in */
	/*  that direction -- this is equivalent to testing a minimal AABB around */
	/*  the triangle against the AABB */

	/* test in the 3 directions */
	for(int axis = 0; axis < 3; ++axis)
	{
		const MinMax min_max = MinMax::find(tri_verts[axis]);
		if(min_max.min_ > boxhalfsize[axis] || min_max.max_ < -boxhalfsize[axis]) return false;
	}

	/* Bullet 2: */
	/*  test if the box intersects the plane of the triangle */
	/*  compute plane equation of triangle: normal*x+d=0 */
	const Vec3Double normal = Vec3Double::cross(tri_edges[0], tri_edges[1]);
	// -NJMP- (line removed here)
	if(!planeBoxOverlap_global(normal, tri_verts[0], boxhalfsize)) return false;	// -NJMP-

	return true;   /* box and triangle overlaps */
}

END_YAFARAY
