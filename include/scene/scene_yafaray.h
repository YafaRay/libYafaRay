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

#ifndef YAFARAY_SCENE_YAFARAY_H
#define YAFARAY_SCENE_YAFARAY_H

#include "scene/scene.h"

BEGIN_YAFARAY

class MeshObject;
class Triangle;
template <typename T> class Accelerator;
class Primitive;

struct ObjData
{
	TriangleObject *obj_;
	MeshObject *mobj_;
	int type_;
	size_t last_vert_id_;
};

class YafaRayScene final : public Scene
{
	public:
		static Scene *factory(ParamMap &params);

	private:
		struct GeometryCreationState
		{
			ObjData *cur_obj_;
			Triangle *cur_tri_;
			bool orco_;
			float smooth_angle_;
		};
		YafaRayScene();
		YafaRayScene(const YafaRayScene &s) = delete;
		virtual ~YafaRayScene() override;
		virtual bool startTriMesh(const std::string &name, int vertices, int triangles, bool has_orco, bool has_uv = false, int type = 0, int obj_pass_index = 0) override;
		virtual bool endTriMesh() override;
		virtual bool startCurveMesh(const std::string &name, int vertices, int obj_pass_index = 0) override;
		virtual bool endCurveMesh(const Material *mat, float strand_start, float strand_end, float strand_shape) override;
		virtual int  addVertex(const Point3 &p) override;
		virtual int  addVertex(const Point3 &p, const Point3 &orco) override;
		virtual void addNormal(const Vec3 &n) override;
		virtual bool addTriangle(int a, int b, int c, const Material *mat) override;
		virtual bool addTriangle(int a, int b, int c, int uv_a, int uv_b, int uv_c, const Material *mat) override;
		virtual int  addUv(float u, float v) override;
		virtual bool smoothMesh(const std::string &name, float angle) override;
		virtual ObjectGeometric *createObject(const std::string &name, ParamMap &params) override;
		virtual bool addInstance(const std::string &base_object_name, const Matrix4 &obj_to_world) override;
		virtual bool updateGeometry() override;
		virtual void clearGeometry() override;

		virtual bool intersect(const Ray &ray, SurfacePoint &sp) const override;
		virtual bool intersect(const DiffRay &ray, SurfacePoint &sp) const override;
		virtual bool isShadowed(RenderData &render_data, const Ray &ray, float &obj_index, float &mat_index) const override;
		virtual bool isShadowed(RenderData &render_data, const Ray &ray, int max_depth, Rgb &filt, float &obj_index, float &mat_index) const override;
		virtual TriangleObject *getMesh(const std::string &name) const override;
		virtual ObjectGeometric *getObject(const std::string &name) const override;

		GeometryCreationState geometry_creation_state_;
		Accelerator<Triangle> *tree_ = nullptr; //!< kdTree for triangle-only mode
		Accelerator<Primitive> *vtree_ = nullptr; //!< kdTree for universal mode
		std::map<std::string, ObjectGeometric *> objects_;
		std::map<std::string, ObjData> meshes_;
};

END_YAFARAY

#endif // YAFARAY_SCENE_YAFARAY_H
