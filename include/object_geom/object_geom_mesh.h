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

#ifndef YAFARAY_OBJECT_GEOM_MESH_H
#define YAFARAY_OBJECT_GEOM_MESH_H

#include "object_geom/object_geom.h"
#include "common/vector.h"
#include "common/matrix4.h"
#include <vector>

BEGIN_YAFARAY

class BsTriangle;


struct Uv
{
	Uv(float u, float v): u_(u), v_(v) {};
	float u_, v_;
};

class Triangle;
class VTriangle;
class TriangleInstance;
class TriangleObjectInstance;

/*!	meshObject_t holds various polygonal primitives
*/

class MeshObject final : public ObjectGeometric
{
		friend class VTriangle;
		friend class BsTriangle;
		friend class Scene;
		friend class YafaRayScene;
	public:
		MeshObject(int ntris, bool has_uv = false, bool has_orco = false);
		/*! the number of primitives the object holds. Primitive is an element
			that by definition can perform ray-triangle intersection */
		int numPrimitives() const { return triangles_.size() + s_triangles_.size(); }
		int getPrimitives(const Primitive **prims) const;

		Primitive *addTriangle(const VTriangle &t);
		Primitive *addBsTriangle(const BsTriangle &t);

		//void setContext(std::vector<point3d_t>::iterator p, std::vector<normal_t>::iterator n);
		void setLight(const Light *l) { light_ = l; }
		void finish();
	protected:
		std::vector<VTriangle> triangles_;
		std::vector<BsTriangle> s_triangles_;
		std::vector<Point3> points_;
		std::vector<Vec3> normals_;
		std::vector<int> uv_offsets_;
		std::vector<Uv> uv_values_;
		bool has_orco_;
		bool has_uv_;
		bool is_smooth_;
		const Light *light_;
};

/*!	This is a special version of meshObject_t!
	The only difference is that it returns a triangle_t instead of vTriangle_t,
	see declaration if triangle_t for more details!
*/

class TriangleObject: public ObjectGeometric
{
		friend class Triangle;
		friend class TriangleInstance;
		friend class Scene;
		friend class YafaRayScene;
		friend class TriangleObjectInstance;

	public:
		TriangleObject() : has_orco_(false), has_uv_(false), is_smooth_(false), normals_exported_(false) { /* Empty */ }
		TriangleObject(int ntris, bool has_uv = false, bool has_orco = false);
		/*! the number of primitives the object holds. Primitive is an element
			that by definition can perform ray-triangle intersection */
		virtual int numPrimitives() const override { return triangles_.size(); }
		virtual int getPrimitives(const Triangle **prims) const override;
		Triangle *addTriangle(const Triangle &t);
		virtual void finish();
		virtual Vec3 getVertexNormal(int index) const { return Vec3(normals_[index]); }
		virtual Point3 getVertex(int index) const { return points_[index]; }

	private:
		std::vector<Triangle> triangles_;
		std::vector<Point3> points_;
		std::vector<Vec3> normals_;
		std::vector<int> uv_offsets_;
		std::vector<Uv> uv_values_;

	protected:
		bool has_orco_;
		bool has_uv_;
		bool is_smooth_;
		bool normals_exported_;
};

class TriangleObjectInstance final: public TriangleObject
{
		friend class TriangleInstance;
		friend class Scene;
		friend class YafaRayScene;

	public:
		TriangleObjectInstance(TriangleObject *base, Matrix4 obj_2_world);

	private:
		/*! the number of primitives the object holds. Primitive is an element
			that by definition can perform ray-triangle intersection */
		virtual int numPrimitives() const override { return triangles_.size(); }
		virtual int getPrimitives(const Triangle **prims) const override;

		virtual void finish() override;

		virtual Vec3 getVertexNormal(int index) const override
		{
			return Vec3(obj_to_world_ * m_base_->normals_[index]);
		}

		virtual Point3 getVertex(int index) const override
		{
			return obj_to_world_ * m_base_->points_[index];
		}

		std::vector<TriangleInstance> triangles_;
		Matrix4 obj_to_world_;
		TriangleObject *m_base_;
};

END_YAFARAY


#endif //YAFARAY_OBJECT_GEOM_MESH_H

