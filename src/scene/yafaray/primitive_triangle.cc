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

#include "yafaray_config.h"
#include "scene/yafaray/primitive_triangle.h"
#include "scene/yafaray/object_mesh.h"
#include "geometry/ray.h"
#include "geometry/bound.h"
#include "geometry/surface.h"
#include "geometry/uv.h"
#include "geometry/matrix4.h"
#include "scene/yafaray/vector_double.h"
#include "common/logger.h"
#include "common/param.h"
#include <string.h>

BEGIN_YAFARAY

TrianglePrimitive::TrianglePrimitive(const std::vector<int> &vertices_indices, const std::vector<int> &vertices_uv_indices, MeshObject *mesh_object) : FacePrimitive(vertices_indices, vertices_uv_indices, mesh_object)
{
	calculateGeometricNormal();
}

IntersectData TrianglePrimitive::intersect(const Ray &ray, const Matrix4 *obj_to_world) const
{
	const std::vector<Point3> vertices = getVertices(obj_to_world);
	return TrianglePrimitive::intersect(ray, {vertices[0], vertices[1], vertices[2]});
}

IntersectData TrianglePrimitive::intersect(const Ray &ray, const std::array<Point3, 3> &vertices)
{
	//Tomas Moller and Ben Trumbore ray intersection scheme
	const Vec3 edge_1 = vertices[1] - vertices[0];
	const Vec3 edge_2 = vertices[2] - vertices[0];
	const float epsilon = 0.1f * min_raydist__ * std::max(edge_1.length(), edge_2.length());
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
	const std::vector<Point3> vertices = getVertices(obj_to_world);
	return TrianglePrimitive::intersectsBound(ex_bound, {vertices[0], vertices[1], vertices[2]});
}

bool TrianglePrimitive::intersectsBound(const ExBound &ex_bound, const std::array<Point3, 3> &vertices)
{
	double t_points[3][3];
	for(size_t i = 0; i < 3; ++i)
	{
		t_points[0][i] = vertices[0][i];
		t_points[1][i] = vertices[1][i];
		t_points[2][i] = vertices[2][i];
	}
	// triBoxOverlap__() is in src/yafraycore/tribox3_d.cc!
	return triBoxOverlap__(ex_bound.center_, ex_bound.half_size_, (double **) t_points);
}

void TrianglePrimitive::calculateGeometricNormal()
{
	const std::vector<Point3> vertices = getVertices();
	normal_geometric_ = calculateNormal({vertices[0], vertices[1], vertices[2]});
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
	const MeshObject *base_mesh_object = static_cast<const MeshObject *>(base_object_);

	if(base_mesh_object->isSmooth() || base_mesh_object->hasNormalsExported())
	{
		const std::vector<Vec3> v = getVerticesNormals(sp.ng_, obj_to_world);
		sp.n_ = barycentric_u * v[0] + barycentric_v * v[1] + barycentric_w * v[2];
		sp.n_.normalize();
	}
	else sp.n_ = sp.ng_;

	if(base_mesh_object->hasOrco())
	{
		const std::vector<Point3> orco_p = getOrcoVertices();
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
	const std::vector<Point3> p = getVertices(obj_to_world);
	if(base_mesh_object->hasUv())
	{
		const std::vector<Uv> &uv = getVerticesUvs();
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

	sp.object_ = base_object_;
	sp.light_ = base_mesh_object->getLight();
	sp.has_uv_ = base_mesh_object->hasUv();
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

bool TrianglePrimitive::clipToBound(const std::array<std::array<double, 3>, 2> &bound, int axis, Bound &clipped, const void *d_old, void *d_new, const Matrix4 *obj_to_world) const
{
	if(axis >= 0) // re-clip
	{
		const bool lower = axis & ~3;
		const int axis_calc = axis & 3;
		const double split = lower ? bound[0][axis_calc] : bound[1][axis_calc];
		const int tri_plane_clip = TrianglePrimitive::triPlaneClip(split, axis_calc, lower, clipped, d_old, d_new);
		// if an error occured due to precision limits...ugly solution i admitt
		if(tri_plane_clip > 1) goto WHOOPS;
		return (tri_plane_clip == 0) ? true : false;
	}
	else // initial clip
	{
		WHOOPS:
		const std::vector<Point3> p = getVertices(obj_to_world);
		std::array<std::array<double, 3>, 3> t_points;
		for(int i = 0; i < 3; ++i)
		{
			t_points[0][i] = p[0][i];
			t_points[1][i] = p[1][i];
			t_points[2][i] = p[2][i];
		}
		const int tri_box_clip = TrianglePrimitive::triBoxClip(bound[0], bound[1], t_points, clipped, d_new);
		return (tri_box_clip == 0) ? true : false;
	}
}

float TrianglePrimitive::surfaceArea(const std::array<Point3, 3> &vertices)
{
	const Vec3 vec_0_1 = vertices[1] - vertices[0];
	const Vec3 vec_0_2 = vertices[2] - vertices[0];
	return 0.5f * (vec_0_1 ^ vec_0_2).length();
}

float TrianglePrimitive::surfaceArea(const Matrix4 *obj_to_world) const
{
	const std::vector<Point3> vertices = getVertices(obj_to_world);
	return surfaceArea({vertices[0], vertices[1], vertices[2]});
}


void TrianglePrimitive::sample(float s_1, float s_2, Point3 &p, Vec3 &n, const Matrix4 *obj_to_world) const
{
	const std::vector<Point3> vertices = getVertices(obj_to_world);
	TrianglePrimitive::sample(s_1, s_2, p, {vertices[0], vertices[1], vertices[2]});
	n = getGeometricNormal(obj_to_world);
}

void TrianglePrimitive::sample(float s_1, float s_2, Point3 &p, const std::array<Point3, 3> &vertices)
{
	const float su_1 = math::sqrt(s_1);
	const float u = 1.f - su_1;
	const float v = s_2 * su_1;
	p = u * vertices[0] + v * vertices[1] + (1.f - u - v) * vertices[2];
}


template <class T>
void swap__(T **a, T **b)
{
	T *x;
	x = *a;
	*a = *b;
	*b = x;
}

struct ClipDump
{
	int nverts_;
	Vec3Double poly_[10];
};

/*! function to clip a triangle against an axis aligned bounding box and return new bound
	\param box the AABB of the clipped triangle
	\return 0: triangle was clipped successfully
			1: triangle didn't overlap the bound at all => disappeared
			2: fatal error occured (didn't ever happen to me :)
			3: resulting polygon degenerated to less than 3 edges (never happened either)
*/

int TrianglePrimitive::triBoxClip(const std::array<double, 3> &b_min, const std::array<double, 3> &b_max, const std::array<std::array<double, 3>, 3> &triverts, Bound &box, void *n_dat)
{
	Vec3Double dump_1[11], dump_2[11]; double t;
	Vec3Double *poly = dump_1, *cpoly = dump_2;

	for(int q = 0; q < 3; q++)
	{
		poly[q][0] = triverts[q][0], poly[q][1] = triverts[q][1], poly[q][2] = triverts[q][2];
		poly[3][q] = triverts[0][q];
	}

	int n = 3, nc;
	bool p_1_inside;

	//for each axis
	for(int axis = 0; axis < 3; axis++)
	{
		Vec3Double *p_1, *p_2;
		int next_axis = (axis + 1) % 3, prev_axis = (axis + 2) % 3;

		// === clip lower ===
		nc = 0;
		p_1_inside = (poly[0][axis] >= b_min[axis]) ? true : false;
		for(int i = 0; i < n; i++) // for each poly edge
		{
			p_1 = &poly[i], p_2 = &poly[i + 1];
			if(p_1_inside)   // p1 inside
			{
				if((*p_2)[axis] >= b_min[axis]) //both "inside"
				{
					// copy p2 to new poly
					cpoly[nc] = *p_2;
					nc++;
					p_1_inside = true;
				}
				else
				{
					// clip line, add intersection to new poly
					t = (b_min[axis] - (*p_1)[axis]) / ((*p_2)[axis] - (*p_1)[axis]);
					cpoly[nc][axis] = b_min[axis];
					cpoly[nc][next_axis] = (*p_1)[next_axis] + t * ((*p_2)[next_axis] - (*p_1)[next_axis]);
					cpoly[nc][prev_axis] = (*p_1)[prev_axis] + t * ((*p_2)[prev_axis] - (*p_1)[prev_axis]);
					nc++;
					p_1_inside = false;
				}
			}
			else //p1 < b_min -> outside
			{
				if((*p_2)[axis] > b_min[axis]) //p2 inside, add s and p2
				{
					t = (b_min[axis] - (*p_2)[axis]) / ((*p_1)[axis] - (*p_2)[axis]);
					cpoly[nc][axis] = b_min[axis];
					cpoly[nc][next_axis] = (*p_2)[next_axis] + t * ((*p_1)[next_axis] - (*p_2)[next_axis]);
					cpoly[nc][prev_axis] = (*p_2)[prev_axis] + t * ((*p_1)[prev_axis] - (*p_2)[prev_axis]);
					nc++;
					cpoly[nc] = *p_2;
					nc++;
					p_1_inside = true;
				}
				else if((*p_2)[axis] == b_min[axis]) //p2 and s are identical, only add p2
				{
					cpoly[nc] = *p_2;
					nc++;
					p_1_inside = true;
				}
				else p_1_inside = false;
			}
			//else: both outse, do nothing.
		} //for all edges

		if(nc > 9)
		{
			Y_VERBOSE << "TriangleClip: after min n is now " << nc << ", that's bad!" << YENDL;
			return 2;
		}

		cpoly[nc] = cpoly[0];
		n = nc;
		swap__(&cpoly, &poly);


		// === clip upper ===
		nc = 0;
		p_1_inside = (poly[0][axis] <= b_max[axis]) ? true : false;
		for(int i = 0; i < n; i++) // for each poly edge
		{
			p_1 = &poly[i], p_2 = &poly[i + 1];
			if(p_1_inside)
			{
				if((*p_2)[axis] <= b_max[axis]) //both "inside"
				{
					// copy p2 to new poly
					cpoly[nc] = *p_2;
					nc++;
					p_1_inside = true;
				}
				else
				{
					// clip line, add intersection to new poly
					t = (b_max[axis] - (*p_1)[axis]) / ((*p_2)[axis] - (*p_1)[axis]);
					cpoly[nc][axis] = b_max[axis];
					cpoly[nc][next_axis] = (*p_1)[next_axis] + t * ((*p_2)[next_axis] - (*p_1)[next_axis]);
					cpoly[nc][prev_axis] = (*p_1)[prev_axis] + t * ((*p_2)[prev_axis] - (*p_1)[prev_axis]);
					nc++;
					p_1_inside = false;
				}
			}
			else //p1 > b_max -> outside
			{
				if((*p_2)[axis] < b_max[axis]) //p2 inside, add s and p2
				{
					t = (b_max[axis] - (*p_2)[axis]) / ((*p_1)[axis] - (*p_2)[axis]);
					cpoly[nc][axis] = b_max[axis];
					cpoly[nc][next_axis] = (*p_2)[next_axis] + t * ((*p_1)[next_axis] - (*p_2)[next_axis]);
					cpoly[nc][prev_axis] = (*p_2)[prev_axis] + t * ((*p_1)[prev_axis] - (*p_2)[prev_axis]);
					nc++;
					cpoly[nc] = *p_2;
					nc++;
					p_1_inside = true;
				}
				else if((*p_2)[axis] == b_max[axis]) //p2 and s are identical, only add p2
				{
					cpoly[nc] = *p_2;
					nc++;
					p_1_inside = true;
				}
				else p_1_inside = false;
			}
			//else: both outse, do nothing.
		} //for all edges

		if(nc > 9)
		{
			Y_VERBOSE << "TriangleClip: After max n is now " << nc << ", that's bad!" << YENDL;
			return 2;
		}

		if(nc == 0) return 1;

		cpoly[nc] = cpoly[0];
		n = nc;
		swap__(&cpoly, &poly);
	} //for all axis

	if(n < 2)
	{
		static bool foobar = false;
		if(foobar) return 3;
		Y_VERBOSE << "TriangleClip: Clip degenerated! n=" << n << YENDL;
		Y_VERBOSE << "TriangleClip: b_min:\t" << b_min[0] << ",\t" << b_min[1] << ",\t" << b_min[2] << YENDL;
		Y_VERBOSE << "TriangleClip: b_max:\t" << b_max[0] << ",\t" << b_max[1] << ",\t" << b_max[2] << YENDL;
		Y_VERBOSE << "TriangleClip: delta:\t" << b_max[0] - b_min[0] << ",\t" << b_max[1] - b_min[1] << ",\t" << b_max[2] - b_min[2] << YENDL;

		for(int j = 0; j < 3; j++)
		{
			Y_VERBOSE << "TriangleClip: point" << j << ": " << triverts[j][0] << ",\t" << triverts[j][1] << ",\t" << triverts[j][2] << YENDL;
		}
		foobar = true;
		return 3;
	}

	double a[3], g[3];
	for(int i = 0; i < 3; ++i) a[i] = g[i] = poly[0][i];
	for(int i = 1; i < n; i++)
	{
		a[0] = std::min(a[0], poly[i][0]);
		a[1] = std::min(a[1], poly[i][1]);
		a[2] = std::min(a[2], poly[i][2]);
		g[0] = std::max(g[0], poly[i][0]);
		g[1] = std::max(g[1], poly[i][1]);
		g[2] = std::max(g[2], poly[i][2]);
	}

	box.a_[0] = a[0], box.g_[0] = g[0];
	box.a_[1] = a[1], box.g_[1] = g[1];
	box.a_[2] = a[2], box.g_[2] = g[2];

	ClipDump *output = (ClipDump *)n_dat;
	output->nverts_ = n;
	memcpy(output->poly_, poly, (n + 1) * sizeof(Vec3Double));

	return 0;
}

int TrianglePrimitive::triPlaneClip(double pos, int axis, bool lower, Bound &box, const void *o_dat, void *n_dat)
{
	ClipDump *input = (ClipDump *) o_dat; //FIXME: casting const away and writing to it using swap__, dangerous!
	ClipDump *output = (ClipDump *) n_dat;
	Vec3Double *poly = input->poly_;
	Vec3Double *cpoly = output->poly_;
	int nverts = input->nverts_;
	int next_axis = (axis + 1) % 3, prev_axis = (axis + 2) % 3;

	if(lower)
	{
		// === clip lower ===
		int nc = 0;
		bool p_1_inside = (poly[0][axis] >= pos) ? true : false;
		for(int i = 0; i < nverts; i++) // for each poly edge
		{
			const Vec3Double *p_1 = &poly[i], *p_2 = &poly[i + 1];
			if(p_1_inside)   // p1 inside
			{
				if((*p_2)[axis] >= pos) //both "inside"
				{
					// copy p2 to new poly
					cpoly[nc] = *p_2;
					nc++;
					p_1_inside = true;
				}
				else
				{
					// clip line, add intersection to new poly
					const double t = (pos - (*p_1)[axis]) / ((*p_2)[axis] - (*p_1)[axis]);
					cpoly[nc][axis] = pos;
					cpoly[nc][next_axis] = (*p_1)[next_axis] + t * ((*p_2)[next_axis] - (*p_1)[next_axis]);
					cpoly[nc][prev_axis] = (*p_1)[prev_axis] + t * ((*p_2)[prev_axis] - (*p_1)[prev_axis]);
					nc++;
					p_1_inside = false;
				}
			}
			else //p1 < b_min -> outside
			{
				if((*p_2)[axis] > pos) //p2 inside, add s and p2
				{
					const double t = (pos - (*p_2)[axis]) / ((*p_1)[axis] - (*p_2)[axis]);
					cpoly[nc][axis] = pos;
					cpoly[nc][next_axis] = (*p_2)[next_axis] + t * ((*p_1)[next_axis] - (*p_2)[next_axis]);
					cpoly[nc][prev_axis] = (*p_2)[prev_axis] + t * ((*p_1)[prev_axis] - (*p_2)[prev_axis]);
					nc++;
					cpoly[nc] = *p_2;
					nc++;
					p_1_inside = true;
				}
				else if((*p_2)[axis] == pos) //p2 and s are identical, only add p2
				{
					cpoly[nc] = *p_2;
					nc++;
					p_1_inside = true;
				}
				else p_1_inside = false;
			}
			//else: both outse, do nothing.
		} //for all edges

		if(nc == 0) return 1;

		if(nc > 9)
		{
			Y_VERBOSE << "TriangleClip: After min n is now " << nc << ", that's bad!" << YENDL;
			return 2;
		}

		cpoly[nc] = cpoly[0];
		nverts = nc;
		swap__(&cpoly, &poly);
	}
	else
	{
		// === clip upper ===
		int nc = 0;
		bool p_1_inside = (poly[0][axis] <= pos) ? true : false;
		for(int i = 0; i < nverts; i++) // for each poly edge
		{
			const Vec3Double *p_1 = &poly[i], *p_2 = &poly[i + 1];
			if(p_1_inside)
			{
				if((*p_2)[axis] <= pos) //both "inside"
				{
					// copy p2 to new poly
					cpoly[nc] = *p_2;
					nc++;
					p_1_inside = true;
				}
				else
				{
					// clip line, add intersection to new poly
					const double t = (pos - (*p_1)[axis]) / ((*p_2)[axis] - (*p_1)[axis]);
					cpoly[nc][axis] = pos;
					cpoly[nc][next_axis] = (*p_1)[next_axis] + t * ((*p_2)[next_axis] - (*p_1)[next_axis]);
					cpoly[nc][prev_axis] = (*p_1)[prev_axis] + t * ((*p_2)[prev_axis] - (*p_1)[prev_axis]);
					nc++;
					p_1_inside = false;
				}
			}
			else //p1 > pos -> outside
			{
				if((*p_2)[axis] < pos) //p2 inside, add s and p2
				{
					const double t = (pos - (*p_2)[axis]) / ((*p_1)[axis] - (*p_2)[axis]);
					cpoly[nc][axis] = pos;
					cpoly[nc][next_axis] = (*p_2)[next_axis] + t * ((*p_1)[next_axis] - (*p_2)[next_axis]);
					cpoly[nc][prev_axis] = (*p_2)[prev_axis] + t * ((*p_1)[prev_axis] - (*p_2)[prev_axis]);
					nc++;
					cpoly[nc] = *p_2;
					nc++;
					p_1_inside = true;
				}
				else if((*p_2)[axis] == pos) //p2 and s are identical, only add p2
				{
					cpoly[nc] = *p_2;
					nc++;
					p_1_inside = true;
				}
				else p_1_inside = false;
			}
			//else: both outse, do nothing.
		} //for all edges

		if(nc == 0) return 1;

		if(nc > 9)
		{
			Y_VERBOSE << "TriangleClip: after max n is now " << nc << ", that's bad!" << YENDL;
			return 2;
		}

		cpoly[nc] = cpoly[0];
		nverts = nc;
		swap__(&cpoly, &poly);
	} //for all axis

	if(nverts < 2)
	{
		static bool foobar = false;
		if(foobar) return 3;
		Y_VERBOSE << "TriangleClip: Clip degenerated! n=" << nverts << YENDL;
		foobar = true;
		return 3;
	}

	double a[3], g[3];
	for(int i = 0; i < 3; ++i) a[i] = g[i] = poly[0][i];
	for(int i = 1; i < nverts; i++)
	{
		a[0] = std::min(a[0], poly[i][0]);
		a[1] = std::min(a[1], poly[i][1]);
		a[2] = std::min(a[2], poly[i][2]);
		g[0] = std::max(g[0], poly[i][0]);
		g[1] = std::max(g[1], poly[i][1]);
		g[2] = std::max(g[2], poly[i][2]);
	}

	box.a_[0] = a[0], box.g_[0] = g[0];
	box.a_[1] = a[1], box.g_[1] = g[1];
	box.a_[2] = a[2], box.g_[2] = g[2];

	output->nverts_ = nverts;

	return 0;
}

END_YAFARAY