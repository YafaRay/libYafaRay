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

#include "geometry/object_triangle.h"
#include "geometry/triangle.h"
#include "geometry/uv.h"
#include "common/logger.h"

BEGIN_YAFARAY

TriangleObject::TriangleObject(int ntris, bool has_uv, bool has_orco):
		has_orco_(has_orco), has_uv_(has_uv), is_smooth_(false), normals_exported_(false)
{
	triangles_.reserve(ntris);
	if(has_uv) uv_offsets_.reserve(ntris);
	if(has_orco) points_.reserve(2 * 3 * ntris);
	else points_.reserve(3 * ntris);
}

int TriangleObject::getPrimitives(const Triangle **prims) const
{
	for(unsigned int i = 0; i < triangles_.size(); ++i) prims[i] = &(triangles_[i]);
	return triangles_.size();
}

Triangle *TriangleObject::addTriangle(const Triangle &t)
{
	triangles_.push_back(t);
	triangles_.back().setSelfIndex(triangles_.size() - 1);
	return &(triangles_.back());
}

void TriangleObject::finish()
{
	for(auto &triangle : triangles_) triangle.recNormal();
}

void TriangleObject::addNormal(const Vec3 &n, size_t last_vert_id)
{
	const size_t points_size = points_.size();
	const size_t normals_size = normals_.size();
	if(points_size > last_vert_id && points_size > normals_size)
	{
		if(normals_size < points_size)
			normals_.resize(points_size);
		normals_[last_vert_id] = n;
		normals_exported_ = true;
	}
}

void prepareEdges__(const std::array<int, 3> &triangle_indices, const std::vector<Point3> &vertices, Vec3 &edge_1, Vec3 &edge_2)
{
	edge_1 = vertices[triangle_indices[1]] - vertices[triangle_indices[0]];
	edge_2 = vertices[triangle_indices[2]] - vertices[triangle_indices[0]];
}

bool TriangleObject::smoothMesh(float angle)
{
	const size_t points_size = points_.size();
	normals_.reserve(points_size);
	normals_.resize(points_size, {0, 0, 0});

	if(angle >= 180)
	{
		for(auto &tri : triangles_)
		{

			const Vec3 n = tri.getNormal();
			const std::array<int, 3> tri_id = tri.getVerticesIndices();
			Vec3 e_1, e_2;

			prepareEdges__(tri_id, points_, e_1, e_2);
			float alpha = e_1.sinFromVectors(e_2);

			normals_[tri_id[0]] += n * alpha;

			prepareEdges__({tri_id[1], tri_id[0], tri_id[2]}, points_, e_1, e_2);
			alpha = e_1.sinFromVectors(e_2);

			normals_[tri_id[1]] += n * alpha;

			prepareEdges__({tri_id[2], tri_id[0], tri_id[1]}, points_, e_1, e_2);
			alpha = e_1.sinFromVectors(e_2);

			normals_[tri_id[2]] += n * alpha;

			tri.setNormalsIndices(tri_id);
		}

		for(size_t idx = 0; idx < normals_.size(); ++idx) normals_[idx].normalize();

	}
	else if(angle > 0.1f) // angle dependant smoothing
	{
		float thresh = math::cos(math::degToRad(angle));
		std::vector<Vec3> vnormals;
		std::vector<int> vn_index;
		// create list of faces that include given vertex
		std::vector<std::vector<Triangle *>> vface(points_size);
		std::vector<std::vector<float>> alphas(points_size);
		for(auto &tri : triangles_)
		{
			Vec3 e_1, e_2;
			const std::array<int, 3> tri_id = tri.getVerticesIndices();

			prepareEdges__(tri_id, points_, e_1, e_2);
			alphas[tri_id[0]].push_back(e_1.sinFromVectors(e_2));
			vface[tri_id[0]].push_back(&tri);

			prepareEdges__({tri_id[1], tri_id[0], tri_id[2]}, points_, e_1, e_2);
			alphas[tri_id[1]].push_back(e_1.sinFromVectors(e_2));
			vface[tri_id[1]].push_back(&tri);

			prepareEdges__({tri_id[2], tri_id[0], tri_id[1]}, points_, e_1, e_2);
			alphas[tri_id[2]].push_back(e_1.sinFromVectors(e_2));
			vface[tri_id[2]].push_back(&tri);
		}
		for(int i = 0; i < (int)vface.size(); ++i)
		{
			std::vector<Triangle *> &tris = vface[i];
			int j = 0;
			for(auto fi = tris.begin(); fi != tris.end(); ++fi)
			{
				Triangle *f = *fi;
				bool smooth = false;
				// calculate vertex normal for face
				Vec3 vnorm, fnorm;

				fnorm = f->getNormal();
				vnorm = fnorm * alphas[i][j];
				int k = 0;
				for(auto f_2 = tris.begin(); f_2 != tris.end(); ++f_2)
				{
					if(**fi == **f_2)
					{
						k++;
						continue;
					}
					Vec3 f_2_norm = (*f_2)->getNormal();
					if((fnorm * f_2_norm) > thresh)
					{
						smooth = true;
						vnorm += f_2_norm * alphas[i][k];
					}
					k++;
				}
				int n_idx = -1;
				if(smooth)
				{
					vnorm.normalize();
					//search for existing normal
					for(unsigned int l = 0; l < vnormals.size(); ++l)
					{
						if(vnorm * vnormals[l] > 0.999)
						{
							n_idx = vn_index[l];
							break;
						}
					}
					// create new if none found
					if(n_idx == -1)
					{
						n_idx = normals_.size();
						vnormals.push_back(vnorm);
						vn_index.push_back(n_idx);
						normals_.push_back(vnorm);
					}
				}
				// set vertex normal to idx
				const std::array<int, 3> tri_p = f->getVerticesIndices();
				std::array<int, 3> tri_n = f->getNormalsIndices();
				bool smooth_ok = false;
				for(int idx = 0; idx < 3; ++idx)
				{
					if(tri_p[idx] == i)
					{
						tri_n[i] = n_idx;
						smooth_ok = true;
						break;
					}
				}
				if(smooth_ok)
				{
					f->setNormalsIndices(tri_n);
					j++;
				}
				else
				{
					Y_ERROR << "Mesh smoothing error!" << YENDL;
					return false;
				}
			}
			vnormals.clear();
			vn_index.clear();
		}
	}
	setSmooth(true);
	return true;
}

END_YAFARAY