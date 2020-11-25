/****************************************************************************
 *      This is part of the libYafaRay package
 *      Copyright (C) 2006  Mathias Wein
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

#include "scene/yafaray/scene_yafaray.h"
#include "yafaray_config.h"
#include "common/logger.h"
#include "accelerator/accelerator_kdtree.h"
#include "common/param.h"
#include "light/light.h"
#include "material/material.h"
#include "integrator/integrator.h"
#include "texture/texture.h"
#include "background/background.h"
#include "camera/camera.h"
#include "shader/shader_node.h"
#include "render/imagefilm.h"
#include "volume/volume.h"
#include "output/output.h"
#include "render/render_data.h"
#include "scene/yafaray/object_mesh.h"
#include "geometry/object_instance.h"
#include "geometry/surface.h"
#include "geometry/uv.h"
#include "geometry/matrix4.h"
#include "scene/yafaray/primitive_face.h"

BEGIN_YAFARAY

#define Y_VERBOSE_SCENE Y_VERBOSE << "Scene (YafaRay): "
#define Y_ERROR_SCENE Y_ERROR << "Scene (YafaRay): "
#define Y_WARN_SCENE Y_WARNING << "Scene (YafaRay): "

#define WARN_EXIST Y_WARN_SCENE << "Sorry, " << pname << " \"" << name << "\" already exists!" << YENDL

#define ERR_NO_TYPE Y_ERROR_SCENE << pname << " type not specified for \"" << name << "\" node!" << YENDL
#define ERR_ON_CREATE(t) Y_ERROR_SCENE << "No " << pname << " could be constructed '" << t << "'!" << YENDL

#define INFO_VERBOSE_SUCCESS(name, t) Y_VERBOSE_SCENE << "Added " << pname << " '"<< name << "' (" << t << ")!" << YENDL
#define INFO_VERBOSE_SUCCESS_DISABLED(name, t) Y_VERBOSE_SCENE << "Added " << pname << " '"<< name << "' (" << t << ")! [DISABLED]" << YENDL


Scene *YafaRayScene::factory(ParamMap &params)
{
	std::string scene_mode;
	params.getParam("mode", scene_mode);
	Scene *scene = new YafaRayScene();
	return scene;
}

YafaRayScene::YafaRayScene()
{
	current_object_ = nullptr;
}

YafaRayScene::~YafaRayScene()
{
	clearObjects();
}

void YafaRayScene::clearObjects()
{
	if(accelerator_) { delete accelerator_; accelerator_ = nullptr; }
	freeMap(objects_);
	objects_.clear();
}

bool YafaRayScene::endObject()
{
	Y_DEBUG PRTEXT(YafaRayScene::endObject) PREND;
	if(creation_state_.stack_.front() != CreationState::Object) return false;
	const bool result = current_object_->calculateObject(creation_state_.current_material_);
	creation_state_.stack_.pop_front();
	return result;
}

bool YafaRayScene::smoothNormals(const std::string &name, float angle)
{
	Y_DEBUG PRTEXT(YafaRayScene::startObject) PR(name) PR(angle) PREND;
	if(creation_state_.stack_.front() != CreationState::Geometry) return false;
	Object *object;
	if(!name.empty())
	{
		auto it = objects_.find(name);
		if(it == objects_.end()) return false;
		object = it->second;
	}
	else
	{
		object = current_object_;
		if(!object) return false;
	}
	MeshObject *mesh_object = MeshObject::getMeshFromObject(object);
	if(!mesh_object) return false;
	else if(mesh_object->hasNormalsExported() && mesh_object->numNormals() == mesh_object->numVertices())
	{
		mesh_object->setSmooth(true);
		return true;
	}
	else return mesh_object->smoothNormals(angle);
}

int YafaRayScene::addVertex(const Point3 &p)
{
	//Y_DEBUG PRTEXT(YafaRayScene::addVertex) PR(p) PREND;
	if(creation_state_.stack_.front() != CreationState::Object) return -1;
	MeshObject *mesh_object = MeshObject::getMeshFromObject(current_object_);
	if(!mesh_object) return -1;
	else
	{
		mesh_object->addPoint(p);
		return mesh_object->lastVertexId();
	}
/*FIXME BsTriangle handling? if(geometry_creation_state_.cur_obj_->type_ == mtrim__)
	{
		geometry_creation_state_.cur_obj_->mobj_->addPoint(p);
		geometry_creation_state_.cur_obj_->last_vert_id_ = geometry_creation_state_.cur_obj_->mobj_->getPoints().size() - 1;
		return geometry_creation_state_.cur_obj_->mobj_->convertToBezierControlPoints();
	}*/
}

int YafaRayScene::addVertex(const Point3 &p, const Point3 &orco)
{
	if(creation_state_.stack_.front() != CreationState::Object) return -1;
	MeshObject *mesh_object = MeshObject::getMeshFromObject(current_object_);
	if(!mesh_object) return -1;
	else
	{
		mesh_object->addPoint(p);
		mesh_object->addOrcoPoint(orco);
	}
	return mesh_object->lastVertexId();

	//	FIXME BsTriangle handling? if(object_creation_state_.cur_obj_->type_ == mtrim__) return addVertex(p);
}

void YafaRayScene::addNormal(const Vec3 &n)
{
	if(creation_state_.stack_.front() != CreationState::Object) return;
	MeshObject *mesh_object = MeshObject::getMeshFromObject(current_object_);
	if(!mesh_object) return;
	mesh_object->addNormal(n);
}

bool YafaRayScene::addFace(const std::vector<int> &vert_indices, const std::vector<int> &uv_indices)
{
	if(creation_state_.stack_.front() != CreationState::Object) return false;
	MeshObject *mesh_object = MeshObject::getMeshFromObject(current_object_);
	if(!mesh_object) return false;
	mesh_object->addFace(vert_indices, uv_indices, creation_state_.current_material_);
	return true;
}

int YafaRayScene::addUv(float u, float v)
{
	if(creation_state_.stack_.front() != CreationState::Object) return false;
	MeshObject *mesh_object = MeshObject::getMeshFromObject(current_object_);
	if(!mesh_object) return -1;
	return mesh_object->addUvValue({u, v});
}

Object *YafaRayScene::createObject(const std::string &name, ParamMap &params)
{
	std::string pname = "Object";
	if(objects_.find(name) != objects_.end())
	{
		WARN_EXIST;
		return nullptr;
	}
	std::string type;
	if(!params.getParam("type", type))
	{
		ERR_NO_TYPE;
		return nullptr;
	}
	Object *object = Object::factory(params, *this);
	if(object)
	{
		object->setName(name);
		objects_[name] = object;
		INFO_VERBOSE_SUCCESS(name, type);
		creation_state_.stack_.push_front(CreationState::Object);
		creation_state_.changes_ |= CreationState::Flags::CGeom;
		current_object_ = object;
		return object;
	}
	ERR_ON_CREATE(type);
	return nullptr;
}

Object *YafaRayScene::getObject(const std::string &name) const
{
	auto oi = objects_.find(name);
	if(oi != objects_.end()) return oi->second;
	else return nullptr;
}

bool YafaRayScene::updateObjects()
{
	if(accelerator_) { delete accelerator_; accelerator_ = nullptr; }
	int nprims = 0;
	for(const auto &o : objects_)
	{
		if(o.second->getVisibility() == Visibility::Invisible) continue;
		if(o.second->isBaseObject()) continue;
		nprims += o.second->numPrimitives();
	}
	if(nprims > 0)
	{
		std::vector<const Primitive *> primitives;
		for(const auto &o : objects_)
		{
			if(o.second->getVisibility() == Visibility::Invisible) continue;
			if(o.second->isBaseObject()) continue;
			const auto prims = o.second->getPrimitives();
			primitives.insert(primitives.end(), prims.begin(), prims.end());
		}
		ParamMap params;
		params["type"] = std::string("kdtree"); //Do not remove the std::string(), entering directly a string literal can be confused with bool until C++17 new string literals
		params["num_primitives"] = nprims;
		params["depth"] = -1;
		params["leaf_size"] = 1;
		params["cost_ratio"] = 0.8f;
		params["empty_bonus"] = 0.33f;

		accelerator_ = Accelerator<Primitive>::factory(primitives, params);
		scene_bound_ = accelerator_->getBound();
		Y_VERBOSE << "Scene: New scene bound is:" << YENDL <<
				  "(" << scene_bound_.a_.x_ << ", " << scene_bound_.a_.y_ << ", " << scene_bound_.a_.z_ << "), (" <<
				  scene_bound_.g_.x_ << ", " << scene_bound_.g_.y_ << ", " << scene_bound_.g_.z_ << ")" << YENDL;

		if(shadow_bias_auto_) shadow_bias_ = shadow_bias__;
		if(ray_min_dist_auto_) ray_min_dist_ = min_raydist__;

		Y_INFO << "Scene: total scene dimensions: X=" << scene_bound_.longX() << ", Y=" << scene_bound_.longY() << ", Z=" << scene_bound_.longZ() << ", volume=" << scene_bound_.vol() << ", Shadow Bias=" << shadow_bias_ << (shadow_bias_auto_ ? " (auto)" : "") << ", Ray Min Dist=" << ray_min_dist_ << (ray_min_dist_auto_ ? " (auto)" : "") << YENDL;

	}
	else Y_ERROR << "Scene: Scene is empty..." << YENDL;
	return true;
}

bool YafaRayScene::intersect(const Ray &ray, SurfacePoint &sp) const
{
	float dis, z;
	IntersectData data;
	if(ray.tmax_ < 0) dis = std::numeric_limits<float>::infinity();
	else dis = ray.tmax_;
	// intersect with tree:
	if(!accelerator_) return false;
	const Primitive *hitprim = nullptr;
	if(!accelerator_->intersect(ray, dis, &hitprim, z, data)) { return false; }
	Point3 h = ray.from_ + z * ray.dir_;
	hitprim->getSurface(sp, h, data, nullptr);
	sp.origin_ = hitprim;
	sp.data_ = data;
	sp.ray_ = nullptr;
	ray.tmax_ = z;
	return true;
}

bool YafaRayScene::intersect(const DiffRay &ray, SurfacePoint &sp) const
{
	float dis, z;
	IntersectData data;
	if(ray.tmax_ < 0) dis = std::numeric_limits<float>::infinity();
	else dis = ray.tmax_;
	// intersect with tree:
	if(!accelerator_) return false;
	const Primitive *hitprim = nullptr;
	if(!accelerator_->intersect(ray, dis, &hitprim, z, data)) { return false; }
	Point3 h = ray.from_ + z * ray.dir_;
	hitprim->getSurface(sp, h, data, nullptr);
	sp.origin_ = hitprim;
	sp.data_ = data;
	sp.ray_ = &ray;
	ray.tmax_ = z;
	return true;
}

bool YafaRayScene::isShadowed(RenderData &render_data, const Ray &ray, float &obj_index, float &mat_index) const
{

	Ray sray(ray);
	sray.from_ += sray.dir_ * sray.tmin_;
	sray.time_ = render_data.time_;
	float dis;
	if(ray.tmax_ < 0) dis = std::numeric_limits<float>::infinity();
	else  dis = sray.tmax_ - 2 * sray.tmin_;
	const Primitive *hitt = nullptr;
	if(!accelerator_) return false;
	bool shadowed = accelerator_->intersectS(sray, dis, &hitt, shadow_bias_);
	if(hitt)
	{
		if(hitt->getObject()) obj_index = hitt->getObject()->getAbsObjectIndex();	//Object index of the object casting the shadow
		if(hitt->getMaterial()) mat_index = hitt->getMaterial()->getAbsMaterialIndex();	//Material index of the object casting the shadow
	}
	return shadowed;
}

bool YafaRayScene::isShadowed(RenderData &render_data, const Ray &ray, int max_depth, Rgb &filt, float &obj_index, float &mat_index) const
{
	Ray sray(ray);
	sray.from_ += sray.dir_ * sray.tmin_;
	float dis;
	if(ray.tmax_ < 0) dis = std::numeric_limits<float>::infinity();
	else  dis = sray.tmax_ - 2 * sray.tmin_;
	filt = Rgb(1.0);
	void *odat = render_data.arena_;
	alignas (16) unsigned char userdata[Integrator::getUserDataSize()];
	render_data.arena_ = static_cast<void *>(userdata);
	bool isect = false;
	const Primitive *hitt = nullptr;
	if(accelerator_)
	{
		isect = accelerator_->intersectTs(render_data, sray, max_depth, dis, &hitt, filt, shadow_bias_);
		if(hitt)
		{
			if(hitt->getObject()) obj_index = hitt->getObject()->getAbsObjectIndex();	//Object index of the object casting the shadow
			if(hitt->getMaterial()) mat_index = hitt->getMaterial()->getAbsMaterialIndex();	//Material index of the object casting the shadow
		}
	}
	render_data.arena_ = odat;
	return isect;
}


bool YafaRayScene::addInstance(const std::string &base_object_name, const Matrix4 &obj_to_world)
{
	//Y_DEBUG PRTEXT(YafaRayScene::addInstance) PR(base_object_name) PREND;
	//Y_DEBUG PRPREC(6) PR(obj_to_world) PREND;
	const Object *base_object = objects_.find(base_object_name)->second;
	if(objects_.find(base_object_name) == objects_.end())
	{
		Y_ERROR << "Base mesh for instance doesn't exist " << base_object_name << YENDL;
		return false;
	}
	int id = getNextFreeId();
	if(id > 0)
	{
		const std::string instance_name = base_object_name + "-" + std::to_string(id);
		Y_DEBUG << "  " PRTEXT(Instance:) PR(instance_name) PR(base_object_name) PREND;
		objects_[instance_name] = new ObjectInstance(base_object, obj_to_world);
		return true;
	}
	else return false;
}

END_YAFARAY
