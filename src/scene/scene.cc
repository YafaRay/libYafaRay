/****************************************************************************
 *      scene.cc: scene_t controls the rendering of a scene
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

#include "scene/scene.h"
#include "accelerator/accelerator.h"
#include "background/background.h"
#include "common/logger.h"
#include "param/param.h"
#include "geometry/matrix.h"
#include "geometry/object/object.h"
#include "geometry/instance.h"
#include "geometry/primitive/primitive.h"
#include "geometry/primitive/primitive_instance.h"
#include "geometry/uv.h"
#include "image/image.h"
#include "light/light.h"
#include "material/material.h"
#include "texture/texture.h"
#include "param/param_result.h"
#include "volume/region/volume_region.h"
#include "render/render_control.h"
#include <memory>

namespace yafaray {

Scene::Scene(Logger &logger, const std::string &name) : name_{name}, scene_bound_{std::make_unique<Bound<float>>()}, logger_{logger}
{
	createDefaultMaterial();
}

//This is just to avoid compilation error "error: invalid application of ‘sizeof’ to incomplete type ‘yafaray::Accelerator’" because the destructor needs to know the type of any shared_ptr or unique_ptr objects
Scene::~Scene() = default;

void Scene::createDefaultMaterial()
{
	ParamMap param_map;
	std::list<ParamMap> nodes_params;
	//Note: keep the std::string or the parameter will be created incorrectly as a bool
	param_map["type"] = std::string("shinydiffusemat");
	material_id_default_ = createMaterial("YafaRay_Default_Material", std::move(param_map), std::move(nodes_params)).first;
}

const Background *Scene::getBackground() const
{
	return background_.get();
}

Bound<float> Scene::getSceneBound() const
{
	return *scene_bound_;
}

std::pair<size_t, ResultFlags> Scene::getMaterial(const std::string &name) const
{
	const auto [material_id, material_result]{materials_.findIdFromName(name)};
	if(material_result.notOk()) return {material_id_default_, material_result};
	else return {material_id, material_result};
}

std::tuple<const Light *, size_t, ResultFlags> Scene::getLight(const std::string &name) const
{
	return lights_.getByName(name);
}

std::pair<const Light *, ResultFlags> Scene::getLight(size_t light_id) const
{
	return lights_.getById(light_id);
}

std::tuple<const Texture *, size_t, ResultFlags> Scene::getTexture(const std::string &name) const
{
	return textures_.getByName(name);
}

std::tuple<Image *, size_t, ResultFlags> Scene::getImage(const std::string &name) const
{
	return images_.getByName(name);
}

std::pair<size_t, ParamResult> Scene::createLight(const std::string &name, const ParamMap &param_map)
{
	auto result{Items<Light>::createItem<Scene>(logger_, lights_, name, param_map, *this)};
	return result;
}

std::pair<size_t, ParamResult> Scene::createMaterial(const std::string &name, const ParamMap &param_map, const std::list<ParamMap> &param_map_list_nodes)
{
	auto [material, param_result]{Material::factory(logger_, *this, name, param_map, param_map_list_nodes)};
	if(param_result.hasError()) return {material_id_default_, ParamResult{YAFARAY_RESULT_ERROR_WHILE_CREATING}};
	if(logger_.isVerbose())
	{
		logger_.logVerbose(getClassName(), " '", this->getName(), "': Added ", material->getClassName(), " '", name, "' (", material->type().print(), ")!");
	}
	auto [material_id, result_flags]{materials_.add(name, std::move(material))};
	if(result_flags == YAFARAY_RESULT_WARNING_OVERWRITTEN) logger_.logDebug(getClassName(), " '", this->getName(), "': ", material->getClassName(), " \"", name, "\" already exists, replacing.");
	param_result.flags_ |= result_flags;
	return {material_id, param_result};
}


std::pair<size_t, ParamResult> Scene::createTexture(const std::string &name, const ParamMap &param_map)
{
	auto result{Items<Texture>::createItem<Scene>(logger_, textures_, name, param_map, *this)};
	return result;
}

ParamResult Scene::defineBackground(const ParamMap &param_map)
{
	if(background_)
	{
		for(const auto &[ibl_light_name, ibl_light_param_map] : background_->getRequestedIblLights()) disableLight(ibl_light_name);
	}
	auto [background, background_result]{Background::factory(logger_, "background", param_map, getTextures())};
	if(!background)
	{
		logger_.logError(getClassName(), " '", this->getName(), "': background could not be created!");
		return background_result;
	}
	else if(logger_.isVerbose())
	{
		logger_.logVerbose(getClassName(), " '", this->getName(), "': Added ", background->getClassName(), " '", "", "' (", background->type().print(), ")!");
	}
	//logger_.logParams(result.first->getAsParamMap(true).print()); //TEST CODE ONLY, REMOVE!!
	background_ = std::move(background);
	if(background_->usesIblBlur()) textures_.getById(background->getTextureId()).first->useMipMaps();
	for(const auto &[ibl_light_name, ibl_light_param_map] : background_->getRequestedIblLights())
	{
		createLight(ibl_light_name, ibl_light_param_map);
	}
	return background_result;
}

std::pair<size_t, ParamResult> Scene::createVolumeRegion(const std::string &name, const ParamMap &param_map)
{
	auto result{Items<VolumeRegion>::createItem<Scene>(logger_, volume_regions_, name, param_map, *this)};
	return result;
}

std::pair<size_t, ParamResult> Scene::createImage(const std::string &name, const ParamMap &param_map)
{
	auto result{Items<Image>::createItem<Scene>(logger_, images_, name, param_map, *this)};
	return result;
}

bool Scene::initObject(size_t object_id, size_t material_id)
{
	if(logger_.isDebug()) logger_.logDebug(getClassName(), " '", getName(), "'::initObject");
	auto[object, object_result]{objects_.getById(object_id)};
	const bool result{object->calculateObject(material_id)};
	return result;
}

bool Scene::smoothVerticesNormals(size_t object_id, float angle)
{
	auto [object, object_result]{objects_.getById(object_id)};
	if(object_result == YAFARAY_RESULT_ERROR_NOT_FOUND)
	{
		logger_.logWarning(getClassName(), " '", getName(), "'::smoothVerticesNormals: object id '", object_id, "' not found, skipping...");
		return false;
	}
	if(object->hasVerticesNormals(0) && object->numVerticesNormals(0) == object->numVertices(0))
	{
		object->setSmooth(true);
		return true;
	}
	else return object->smoothVerticesNormals(logger_, angle);
}

int Scene::addVertex(size_t object_id, Point3f &&p, unsigned char time_step)
{
	auto[object, object_result]{objects_.getById(object_id)};
	object->addPoint(std::move(p), time_step);
	return object->lastVertexId(time_step);
}

int Scene::addVertex(size_t object_id, Point3f &&p, Point3f &&orco, unsigned char time_step)
{
	auto[object, object_result]{objects_.getById(object_id)};
	object->addPoint(std::move(p), time_step);
	object->addOrcoPoint(std::move(orco), time_step);
	return object->lastVertexId(time_step);
}

void Scene::addVertexNormal(size_t object_id, Vec3f &&n, unsigned char time_step)
{
	auto[object, object_result]{objects_.getById(object_id)};
	object->addVertexNormal(std::move(n), time_step);
}

bool Scene::addFace(size_t object_id, const FaceIndices<int> &face_indices, size_t material_id)
{
	auto[object, object_result]{objects_.getById(object_id)};
	object->addFace(face_indices, material_id);
	return true;
}

int Scene::addUv(size_t object_id, Uv<float> &&uv)
{
	auto[object, object_result]{objects_.getById(object_id)};
	return object->addUvValue(std::move(uv));
}

std::pair<size_t, ParamResult> Scene::createObject(const std::string &name, const ParamMap &param_map)
{
	auto result{Items<Object>::createItem<Scene>(logger_, objects_, name, param_map, *this)};
	return result;
}

std::tuple<const Object *, size_t, ResultFlags> Scene::getObject(const std::string &name) const
{
	return objects_.getByName(name);
}

std::pair<const Object *, ResultFlags> Scene::getObject(size_t object_id) const
{
	return objects_.getById(object_id);
}

size_t Scene::createInstance()
{
	instances_.emplace_back(std::make_unique<Instance>());
	return instances_.size() - 1;
}

bool Scene::addInstanceObject(size_t instance_id, size_t object_id)
{
	if(instance_id >= instances_.size()) return false;
	const auto [object, object_result]{objects_.getById(object_id)};
	if(!object || object_result == YAFARAY_RESULT_ERROR_NOT_FOUND) return false;
	else
	{
		instances_[instance_id]->addObject(object_id);
		return true;
	}
}

bool Scene::addInstanceOfInstance(size_t instance_id, size_t base_instance_id)
{
	if(instance_id >= instances_.size() || base_instance_id >= instances_.size()) return false;
	instances_[instance_id]->addInstance(base_instance_id);
	return true;
}

bool Scene::addInstanceMatrix(size_t instance_id, Matrix4f &&obj_to_world, float time)
{
	if(instance_id >= instances_.size()) return false;
	instances_[instance_id]->addObjToWorldMatrix(std::move(obj_to_world), time);
	return true;
}

yafaray_SceneModifiedFlags Scene::checkAndClearSceneModifiedFlags()
{
	int scene_modified_flags{YAFARAY_SCENE_MODIFIED_NOTHING};
	if(objects_.modified()) //FIXME: INSTANCES MODIFIED????
	{
		scene_modified_flags = scene_modified_flags | YAFARAY_SCENE_MODIFIED_OBJECTS;
		objects_.clearModifiedList();
	}
	if(lights_.modified())
	{
		scene_modified_flags = scene_modified_flags | YAFARAY_SCENE_MODIFIED_LIGHTS;
		lights_.clearModifiedList();
	}
	if(materials_.modified())
	{
		scene_modified_flags = scene_modified_flags | YAFARAY_SCENE_MODIFIED_MATERIALS;
		materials_.clearModifiedList();
	}
	if(textures_.modified())
	{
		scene_modified_flags = scene_modified_flags | YAFARAY_SCENE_MODIFIED_TEXTURES;
		textures_.clearModifiedList();
	}
	if(volume_regions_.modified())
	{
		scene_modified_flags = scene_modified_flags | YAFARAY_SCENE_MODIFIED_VOLUME_REGIONS;
		volume_regions_.clearModifiedList();
	}
	if(images_.modified())
	{
		scene_modified_flags = scene_modified_flags | YAFARAY_SCENE_MODIFIED_IMAGES;
		images_.clearModifiedList();
	}
	if(accelerator_param_map_modified_)
	{
		scene_modified_flags = scene_modified_flags | YAFARAY_SCENE_MODIFIED_SCENE_ACCELERATOR_PARAMS;
		accelerator_param_map_modified_ = false;
	}
	return static_cast<yafaray_SceneModifiedFlags>(scene_modified_flags);
}

bool Scene::preprocess(const RenderControl &render_control, yafaray_SceneModifiedFlags scene_modified_flags)
{
	if(render_control.canceled() || render_control.finished()) return false;
	//if(!accelerator_) scene_modified_flags = static_cast<yafaray_SceneModifiedFlags>(YAFARAY_SCENE_MODIFIED_LIGHTS | YAFARAY_SCENE_MODIFIED_IMAGES | YAFARAY_SCENE_MODIFIED_TEXTURES | YAFARAY_SCENE_MODIFIED_MATERIALS | YAFARAY_SCENE_MODIFIED_OBJECTS | YAFARAY_SCENE_MODIFIED_VOLUME_REGIONS);
	if((scene_modified_flags & YAFARAY_SCENE_MODIFIED_OBJECTS) || (scene_modified_flags & YAFARAY_SCENE_MODIFIED_SCENE_ACCELERATOR_PARAMS))
	{
		std::vector<const Primitive *> primitives;
		for(const auto &[object, object_name, object_enabled]: objects_)
		{
			if(!object || !object_enabled || object->getVisibility() == Visibility::None || object->isBaseObject()) continue;
			const auto object_primitives{object->getPrimitives()};
			primitives.insert(primitives.end(), object_primitives.begin(), object_primitives.end());
		}
		for(size_t instance_id = 0; instance_id < instances_.size(); ++instance_id)
		{
			//if(object->getVisibility() == Visibility::Invisible) continue; //FIXME
			//if(object->isBaseObject()) continue; //FIXME
			auto instance{instances_[instance_id].get()};
			if(!instance) continue;
			const bool instance_primitives_result{instance->updatePrimitives(*this)};
			if(!instance_primitives_result)
			{
				logger_.logWarning(getClassName(), " '", getName(), "': Instance id=", instance_id, " could not update primitives, maybe recursion problem...");
				continue;
			}
			const auto instance_primitives{instance->getPrimitives()};
			primitives.insert(primitives.end(), instance_primitives.begin(), instance_primitives.end());
		}
		if(primitives.empty())
		{
			logger_.logWarning(getClassName(), " '", getName(), "': Scene is empty...");
		}

		auto [accelerator, accelerator_result]{Accelerator::factory(logger_, &render_control, primitives, accelerator_param_map_)};
		if(logger_.isVerbose() && accelerator)
		{
			logger_.logVerbose(getClassName(), " '", getName(), "': Added ", accelerator->getClassName(), " (", accelerator->type().print(), ")!");
		}
		accelerator_ = std::move(accelerator);
		*scene_bound_ = accelerator_->getBound();
		if(logger_.isVerbose()) logger_.logVerbose(getClassName(), " '", getName(), "': New scene bound is: ", "(", scene_bound_->a_[Axis::X], ", ", scene_bound_->a_[Axis::Y], ", ", scene_bound_->a_[Axis::Z], "), (", scene_bound_->g_[Axis::X], ", ", scene_bound_->g_[Axis::Y], ", ", scene_bound_->g_[Axis::Z], ")");

		object_index_highest_ = 1;
		for(const auto &[object, object_name, object_enabled]: objects_)
		{
			if(object_index_highest_ < object->getPassIndex()) object_index_highest_ = object->getPassIndex();
			object->setLight(math::invalid<size_t>);
		}
	}

	if(scene_modified_flags & YAFARAY_SCENE_MODIFIED_MATERIALS || scene_modified_flags & YAFARAY_SCENE_MODIFIED_TEXTURES || scene_modified_flags & YAFARAY_SCENE_MODIFIED_IMAGES)
	{
		for(auto &texture: textures_)
		{
			texture.item_->updateMipMaps();
		}
		material_index_highest_ = 1;
		for(size_t material_id = 0; material_id < materials_.size(); ++material_id)
		{
			const int material_pass_index{materials_.getById(material_id).first->getPassIndex()};
			if(material_index_highest_ < material_pass_index) material_index_highest_ = material_pass_index;
		}

		logger_.logInfo(getClassName(), " '", getName(), "': total scene dimensions: X=", scene_bound_->length(Axis::X), ", y=", scene_bound_->length(Axis::Y), ", z=", scene_bound_->length(Axis::Z), ", volume=", scene_bound_->vol());

		mipmap_interpolation_required_ = false;
		for(size_t texture_id = 0; texture_id < textures_.size(); ++texture_id)
		{
			const InterpolationType texture_interpolation_type{textures_.getById(texture_id).first->getInterpolationType()};
			if(texture_interpolation_type == InterpolationType::Trilinear || texture_interpolation_type == InterpolationType::Ewa)
			{
				if(logger_.isVerbose()) logger_.logVerbose(getClassName(), " '", getName(), "': At least one texture using mipmaps interpolation, ray differentials will be enabled.");
				mipmap_interpolation_required_ = true;
				break;
			}
		}
	}
	if(scene_modified_flags & YAFARAY_SCENE_MODIFIED_LIGHTS || scene_modified_flags & YAFARAY_SCENE_MODIFIED_OBJECTS || scene_modified_flags & YAFARAY_SCENE_MODIFIED_TEXTURES || scene_modified_flags & YAFARAY_SCENE_MODIFIED_IMAGES)
	{
		for(auto &[light, light_name, light_enabled]: lights_)
		{
			if(light && light_enabled)
			{
				const size_t object_id{light->init(*this)};
				if(object_id != math::invalid<size_t>) objects_.getById(object_id).first->setLight(light->getId());
			}
		}
	}
	return true;
}

std::pair<const Instance *, ResultFlags> Scene::getInstance(size_t instance_id) const
{
	if(instance_id >= instances_.size()) return {nullptr, YAFARAY_RESULT_ERROR_NOT_FOUND};
	else return {instances_[instance_id].get(), YAFARAY_RESULT_OK};
}

bool Scene::disableLight(const std::string &name)
{
	const auto result{lights_.disable(name)};
	return result.isOk();
}

std::pair<Rgba, bool> Scene::getImageColor(size_t image_id, const Point2i &point) const
{
	const auto [image, image_result]{images_.getById(image_id)};
	if(image_result.notOk()) return {};
	if(point[Axis::X] < 0 || point[Axis::X] >= image->getWidth() || point[Axis::Y] < 0 || point[Axis::Y] >= image->getHeight()) return {};
	return {image->getColor(point), true};
}

bool Scene::setImageColor(size_t image_id, const Point2i &point, const Rgba &col)
{
	auto [image, image_result]{images_.getById(image_id)};
	if(image_result.notOk()) return {};
	if(point[Axis::X] < 0 || point[Axis::X] >= image->getWidth() || point[Axis::Y] < 0 || point[Axis::Y] >= image->getHeight()) return {};
	image->setColor(point, col);
	return true;
}

std::pair<Size2i, bool> Scene::getImageSize(size_t image_id) const
{
	auto [image, image_result]{images_.getById(image_id)};
	if(image_result.notOk()) return {};
	return {image->getSize(), true};
}

void Scene::setAcceleratorParamMap(const ParamMap &param_map)
{
	if(accelerator_param_map_ != param_map)
	{
		accelerator_param_map_ = param_map;
		accelerator_param_map_modified_ = true;
	}
}

std::string Scene::exportToString(yafaray_ContainerExportType container_export_type, bool only_export_non_default_parameters) const
{
	std::stringstream ss;
	ss << getClassName() << " '" << getName() << "'" << std::endl;
	return ss.str();
}

} //namespace yafaray
