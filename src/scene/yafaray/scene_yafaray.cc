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
//#include "yafaray_config.h"
#include "common/logger.h"
#include "accelerator/accelerator.h"
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
#include "image/image_output.h"
#include "scene/yafaray/object_mesh.h"
#include "geometry/object_instance.h"
#include "geometry/uv.h"
#include "geometry/matrix4.h"

BEGIN_YAFARAY

std::unique_ptr<Scene> YafaRayScene::factory(Logger &logger, ParamMap &params)
{
	std::string scene_mode;
	params.getParam("mode", scene_mode);
	auto scene = std::unique_ptr<Scene>(new YafaRayScene(logger));
	return scene;
}

bool YafaRayScene::endObject()
{
	if(logger_.isDebug()) logger_.logDebug("YafaRayScene::endObject");
	if(creation_state_.stack_.front() != CreationState::Object) return false;
	const bool result = current_object_->calculateObject(creation_state_.current_material_);
	creation_state_.stack_.pop_front();
	return result;
}

bool YafaRayScene::smoothNormals(const std::string &name, float angle)
{
	if(logger_.isDebug()) logger_.logDebug("YafaRayScene::startObject) PR(name) PR(angle");
	if(creation_state_.stack_.front() != CreationState::Geometry) return false;
	Object *object;
	if(!name.empty())
	{
		auto it = objects_.find(name);
		if(it == objects_.end()) return false;
		object = it->second.get();
	}
	else
	{
		object = current_object_;
		if(!object) return false;
	}

	if(object->hasNormalsExported() && object->numNormals() == object->numVertices())
	{
		object->setSmooth(true);
		return true;
	}
	else return object->smoothNormals(logger_, angle);
}

int YafaRayScene::addVertex(const Point3 &p)
{
	//if(logger_.isDebug()) logger.logDebug("YafaRayScene::addVertex) PR(p");
	if(creation_state_.stack_.front() != CreationState::Object) return -1;
	current_object_->addPoint(p);
	return current_object_->lastVertexId();
/*FIXME BsTriangle handling? if(geometry_creation_state_.cur_obj_->type_ == mtrim_global)
	{
		geometry_creation_state_.cur_obj_->mobj_->addPoint(p);
		geometry_creation_state_.cur_obj_->last_vert_id_ = geometry_creation_state_.cur_obj_->mobj_->getPoints().size() - 1;
		return geometry_creation_state_.cur_obj_->mobj_->convertToBezierControlPoints();
	}*/
}

int YafaRayScene::addVertex(const Point3 &p, const Point3 &orco)
{
	if(creation_state_.stack_.front() != CreationState::Object) return -1;
	current_object_->addPoint(p);
	current_object_->addOrcoPoint(orco);
	return current_object_->lastVertexId();

	//	FIXME BsTriangle handling? if(object_creation_state_.cur_obj_->type_ == mtrim_global) return addVertex(p);
}

void YafaRayScene::addNormal(const Vec3 &n)
{
	if(creation_state_.stack_.front() != CreationState::Object) return;
	current_object_->addNormal(n);
}

bool YafaRayScene::addFace(const std::vector<int> &vert_indices, const std::vector<int> &uv_indices)
{
	if(creation_state_.stack_.front() != CreationState::Object) return false;
	current_object_->addFace(vert_indices, uv_indices, creation_state_.current_material_);
	return true;
}

int YafaRayScene::addUv(float u, float v)
{
	if(creation_state_.stack_.front() != CreationState::Object) return false;
	return current_object_->addUvValue({u, v});
}

Object *YafaRayScene::createObject(const std::string &name, ParamMap &params)
{
	std::string pname = "Object";
	if(objects_.find(name) != objects_.end())
	{
		logWarnExist(logger_, pname, name);
		return nullptr;
	}
	std::string type;
	if(!params.getParam("type", type))
	{
		logErrNoType(logger_, pname, name, type);
		return nullptr;
	}
	std::unique_ptr<Object> object = Object::factory(logger_, params, *this);
	if(object)
	{
		object->setName(name);
		objects_[name] = std::move(object);
		if(logger_.isVerbose()) logInfoVerboseSuccess(logger_, pname, name, type);
		creation_state_.stack_.push_front(CreationState::Object);
		creation_state_.changes_ |= CreationState::Flags::CGeom;
		current_object_ = objects_[name].get();
		return objects_[name].get();
	}
	logErrOnCreate(logger_, pname, name, type);
	return nullptr;
}

Object *YafaRayScene::getObject(const std::string &name) const
{
	auto oi = objects_.find(name);
	if(oi != objects_.end()) return oi->second.get();
	else return nullptr;
}

bool YafaRayScene::updateObjects()
{
	std::vector<const Primitive *> primitives;
	for(const auto &o : objects_)
	{
		if(o.second->getVisibility() == Visibility::Invisible) continue;
		if(o.second->isBaseObject()) continue;
		const auto prims = o.second->getPrimitives();
		primitives.insert(primitives.end(), prims.begin(), prims.end());
	}
	if(primitives.empty())
	{
		logger_.logError("Scene: Scene is empty...");
		return false;
	}
	ParamMap params;
	params["type"] = scene_accelerator_;
	params["num_primitives"] = static_cast<int>(primitives.size());
	params["depth"] = -1;
	params["leaf_size"] = 1;
	params["cost_ratio"] = 0.8f;
	params["empty_bonus"] = 0.33f;
	params["accelerator_threads"] = getNumThreads();

	accelerator_ = Accelerator::factory(logger_, primitives, params);
	scene_bound_ = accelerator_->getBound();
	if(logger_.isVerbose()) logger_.logVerbose("Scene: New scene bound is: ", "(", scene_bound_.a_.x_, ", ", scene_bound_.a_.y_, ", ", scene_bound_.a_.z_, "), (", scene_bound_.g_.x_, ", ", scene_bound_.g_.y_, ", ", scene_bound_.g_.z_, ")");

	if(shadow_bias_auto_) shadow_bias_ = shadow_bias_global;
	if(ray_min_dist_auto_) ray_min_dist_ = min_raydist_global;

	logger_.logInfo("Scene: total scene dimensions: X=", scene_bound_.longX(), ", Y=", scene_bound_.longY(), ", Z=", scene_bound_.longZ(), ", volume=", scene_bound_.vol(), ", Shadow Bias=", shadow_bias_, (shadow_bias_auto_ ? " (auto)" : ""), ", Ray Min Dist=", ray_min_dist_, (ray_min_dist_auto_ ? " (auto)" : ""));
	return true;
}

bool YafaRayScene::addInstance(const std::string &base_object_name, const Matrix4 &obj_to_world)
{
	//if(logger_.isDebug()) logger.logDebug("YafaRayScene::addInstance) PR(base_object_name");
	//if(logger_.isDebug()) Y_DEBUG PRPREC(6) PR(obj_to_world");
	const Object *base_object = objects_.find(base_object_name)->second.get();
	if(objects_.find(base_object_name) == objects_.end())
	{
		logger_.logError("Base mesh for instance doesn't exist ", base_object_name);
		return false;
	}
	int id = getNextFreeId();
	if(id > 0)
	{
		const std::string instance_name = base_object_name + "-" + std::to_string(id);
		if(logger_.isDebug())logger_.logDebug("  Instance: ", instance_name, " base_object_name=", base_object_name);
		objects_[instance_name] = std::unique_ptr<Object>(new ObjectInstance(*base_object, obj_to_world));
		return true;
	}
	else return false;
}

END_YAFARAY
