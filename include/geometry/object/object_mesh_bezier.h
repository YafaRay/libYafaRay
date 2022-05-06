#pragma once
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

#ifndef YAFARAY_OBJECT_MESH_BEZIER_H
#define YAFARAY_OBJECT_MESH_BEZIER_H

#include "common/logger.h"
#include "object_mesh.h"

BEGIN_YAFARAY

struct Uv;
class FacePrimitive;
class Material;
enum BezierTimeStep : int;

class MeshBezierObject final : public MeshObject
{
	public:
		static Object *factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params);
		MeshBezierObject(int num_vertices, int num_faces, bool has_uv, bool has_orco, float time_range_start, float time_range_end);
		void addFace(const std::vector<int> &vertices, const std::vector<int> &vertices_uv, const std::unique_ptr<const Material> *material) override;
		bool calculateObject(const std::unique_ptr<const Material> *material) override;
		Vec3 getVertexNormal(BezierTimeStep time_step, int index) const;
		Point3 getVertex(BezierTimeStep time_step, int index) const;
		Point3 getOrcoVertex(BezierTimeStep time_step, int index) const;
		float getTimeRangeStart() const { return time_range_start_; }
		float getTimeRangeEnd() const { return time_range_end_; }
		void convertToBezierControlPoints();

	private:
		float time_range_start_ = 0.f;
		float time_range_end_ = 1.f;
};

END_YAFARAY

#endif //YAFARAY_OBJECT_MESH_BEZIER_H
