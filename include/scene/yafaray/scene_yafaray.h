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

class Accelerator;
class Primitive;

class YafaRayScene final : public Scene
{
	public:
		static std::unique_ptr<Scene> factory(Logger &logger, ParamMap &params);

	private:
		YafaRayScene(Logger &logger);
		YafaRayScene(const YafaRayScene &s) = delete;
		virtual ~YafaRayScene() override;
		virtual int  addVertex(const Point3 &p) override;
		virtual int  addVertex(const Point3 &p, const Point3 &orco) override;
		virtual void addNormal(const Vec3 &n) override;
		virtual bool addFace(const std::vector<int> &vert_indices, const std::vector<int> &uv_indices = {}) override;
		virtual int  addUv(float u, float v) override;
		virtual bool smoothNormals(const std::string &name, float angle) override;
		virtual Object *createObject(const std::string &name, ParamMap &params) override;
		virtual bool endObject() override;
		virtual bool addInstance(const std::string &base_object_name, const Matrix4 &obj_to_world) override;
		virtual bool updateObjects() override;
		virtual Object *getObject(const std::string &name) const override;
		void clearObjects();
		virtual const Accelerator *getAccelerator() const override { return accelerator_.get(); }

		Object *current_object_ = nullptr;
		std::unique_ptr<Accelerator> accelerator_;
		std::map<std::string, std::unique_ptr<Object>> objects_;
};

END_YAFARAY

#endif // YAFARAY_SCENE_YAFARAY_H
