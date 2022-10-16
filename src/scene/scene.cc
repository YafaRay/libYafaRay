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
#include "render/progress_bar.h"
#include "accelerator/accelerator.h"
#include "background/background.h"
#include "camera/camera.h"
#include "common/logger.h"
#include "param/param.h"
#include "common/sysinfo.h"
#include "common/version_build_info.h"
#include "format/format.h"
#include "geometry/matrix.h"
#include "geometry/object/object_base.h"
#include "geometry/object/object_instance.h"
#include "geometry/primitive/primitive.h"
#include "geometry/uv.h"
#include "image/image_manipulation.h"
#include "image/image_output.h"
#include "integrator/surface/integrator_surface.h"
#include "integrator/volume/integrator_volume.h"
#include "light/light.h"
#include "material/material.h"
#include "render/imagefilm.h"
#include "render/render_view.h"
#include "texture/texture.h"
#include "param/param_error.h"
#include "volume/region/volume_region.h"
#include <limits>
#include <memory>

namespace yafaray {

void Scene::logWarnExist(Logger &logger, const std::string &pname, const std::string &name)
{
	logger.logWarning("Scene: Sorry, ", pname, " \"", name, "\" already exists!");
}

void Scene::logInfoVerboseSuccess(Logger &logger, const std::string &pname, const std::string &name, const std::string &t)
{
	logger.logVerbose("Scene: ", "Added ", pname, " '", name, "' (", t, ")!");
}

void Scene::logInfoVerboseSuccessDisabled(Logger &logger, const std::string &pname, const std::string &name, const std::string &t)
{
	logger.logVerbose("Scene: ", "Added ", pname, " '", name, "' (", t, ")! [DISABLED]");
}

Scene::Scene(Logger &logger) : scene_bound_(std::make_unique<Bound<float>>()), logger_(logger)
{
	creation_state_.changes_ = CreationState::Flags::CAll;
	creation_state_.stack_.push_front(CreationState::Ready);
	creation_state_.next_free_id_ = std::numeric_limits<int>::max();
	logger_.logInfo("LibYafaRay (", buildinfo::getVersionString(), buildinfo::getBuildTypeSuffix(), ")", " ", buildinfo::getBuildOs(), " ", buildinfo::getBuildArchitectureBits(), "bit (", buildinfo::getBuildCompiler(), ")");
	logger_.logDebug("LibYafaRay build details:");
	if(logger_.isDebug())
	{
		const std::vector<std::string> build_details = buildinfo::getAllBuildDetails();
		for(const auto &build_detail : build_details)
		{
			logger_.logDebug(build_detail);
		}
	}
	render_control_.setDifferentialRaysEnabled(false);	//By default, disable ray differential calculations. Only if at least one texture uses them, then enable differentials.
	createDefaultMaterial();

	image_manipulation::logWarningsMissingLibraries(logger_);
}

//This is just to avoid compilation error "error: invalid application of ‘sizeof’ to incomplete type ‘yafaray::Accelerator’" because the destructor needs to know the type of any shared_ptr or unique_ptr objects
Scene::~Scene() = default;

void Scene::createDefaultMaterial()
{
	ParamMap param_map;
	std::list<ParamMap> nodes_params;
	//Note: keep the std::string or the parameter will be created incorrectly as a bool
	param_map["type"] = std::string("shinydiffusemat");
	const std::unique_ptr<const Material> *material = createMaterial("YafaRay_Default_Material", std::move(param_map), std::move(nodes_params)).first;
	setCurrentMaterial(material);
}

void Scene::setCurrentMaterial(const std::unique_ptr<const Material> *material)
{
	if(material) creation_state_.current_material_ = material;
	else creation_state_.current_material_ = getMaterial("YafaRay_Default_Material");
}

bool Scene::startObjects()
{
	if(creation_state_.stack_.front() != CreationState::Ready) return false;
	creation_state_.stack_.push_front(CreationState::Geometry);
	return true;
}

bool Scene::endObjects()
{
	if(creation_state_.stack_.front() != CreationState::Geometry) return false;
	// in case objects share arrays, so they all need to be updated
	// after each object change, uncomment the below block again:
	// don't forget to update the mesh object iterators!
	/*	for(auto i=meshes.begin();
			 i!=meshes.end(); ++i)
		{
			objData_t &dat = (*i).second;
			dat.obj->setContext(dat.points.begin(), dat.normals.begin() );
		}*/
	creation_state_.stack_.pop_front();
	return true;
}

void Scene::setNumThreads(int threads)
{
	nthreads_ = threads;

	if(nthreads_ == -1) //Automatic detection of number of threads supported by this system, taken from Blender. (DT)
	{
		if(logger_.isVerbose()) logger_.logVerbose("Automatic Detection of Threads: Active.");
		nthreads_ = sysinfo::getNumSystemThreads();
		if(logger_.isVerbose()) logger_.logVerbose("Number of Threads supported: [", nthreads_, "].");
	}
	else
	{
		if(logger_.isVerbose()) logger_.logVerbose("Automatic Detection of Threads: Inactive.");
	}

	logger_.logParams("Using [", nthreads_, "] Threads.");

	std::stringstream set;
	set << "CPU threads=" << nthreads_ << std::endl;

	render_control_.setRenderInfo(set.str());
}

void Scene::setNumThreadsPhotons(int threads_photons)
{
	nthreads_photons_ = threads_photons;

	if(nthreads_photons_ == -1) //Automatic detection of number of threads supported by this system, taken from Blender. (DT)
	{
		if(logger_.isVerbose()) logger_.logVerbose("Automatic Detection of Threads for Photon Mapping: Active.");
		nthreads_photons_ = sysinfo::getNumSystemThreads();
		if(logger_.isVerbose()) logger_.logVerbose("Number of Threads supported for Photon Mapping: [", nthreads_photons_, "].");
	}
	else
	{
		if(logger_.isVerbose()) logger_.logVerbose("Automatic Detection of Threads for Photon Mapping: Inactive.");
	}

	logger_.logParams("Using for Photon Mapping [", nthreads_photons_, "] Threads.");
}

const Background *Scene::getBackground() const
{
	return background_.get();
}

Bound<float> Scene::getSceneBound() const
{
	return *scene_bound_;
}

bool Scene::render(std::unique_ptr<ProgressBar> progress_bar)
{
	if(!image_film_)
	{
		logger_.logError("Scene: No ImageFilm present, bailing out...");
		return false;
	}
	if(!surf_integrator_)
	{
		logger_.logError("Scene: No surface integrator, bailing out...");
		return false;
	}

	render_control_.setProgressBar(std::move(progress_bar));

	//if(creation_state_.changes_ != CreationState::Flags::CNone) //FIXME: handle better subsequent scene renders differently if previous render already complete
	{
		if(creation_state_.changes_ & CreationState::Flags::CGeom) updateObjects();
		for(auto &[light_name, light] : getLights()) light->init(*this);
		for(auto &[output_name, output] : outputs_)
		{
			output->init(image_film_->getSize(), image_film_->getExportedImageLayers(), &render_views_);
		}

		for(auto &[render_view_name, render_view] : render_views_)
		{
			for(auto &[output_name, output] : outputs_) output->setRenderView(render_view.get());
			std::stringstream inte_settings;
			bool success = render_view->init(logger_, *this);
			if(!success)
			{
				logger_.logWarning("Scene: No cameras or lights found at RenderView ", render_view_name, "', skipping this RenderView...");
				continue;
			}

			FastRandom fast_random;
			success = surf_integrator_->preprocess(fast_random, image_film_.get(), render_view.get(), *this);
			if(vol_integrator_) success = success && vol_integrator_->preprocess(fast_random, image_film_.get(), render_view.get(), *this);

			if(!success)
			{
				logger_.logError("Scene: Preprocessing process failed, exiting...");
				return false;
			}
			render_control_.setStarted();
			success = surf_integrator_->render(fast_random, object_index_highest_, material_index_highest_);
			if(!success)
			{
				logger_.logError("Scene: Rendering process failed, exiting...");
				return false;
			}
			render_control_.setRenderInfo(surf_integrator_->getRenderInfo());
			render_control_.setAaNoiseInfo(surf_integrator_->getAaNoiseInfo());
			surf_integrator_->cleanup();
			image_film_->flush(render_view.get(), render_control_, getEdgeToonParams());
			render_control_.setFinished();
			image_film_->cleanup();
		}
	}
	creation_state_.changes_ = CreationState::Flags::CNone;
	return true;
}

ObjId_t Scene::getNextFreeId()
{
	return --creation_state_.next_free_id_;
}

void Scene::clearNonObjects()
{
	lights_.clear();
	textures_.clear();
	materials_.clear();
	cameras_.clear();
	volume_regions_.clear();
	outputs_.clear();
	render_views_.clear();
	clearLayers();
}

void Scene::clearAll()
{
	clearNonObjects();
}

void Scene::clearOutputs()
{
	outputs_.clear();
}

void Scene::clearLayers()
{
	layers_.clear();
}

template <typename T>
T *Scene::findMapItem(const std::string &name, const std::map<std::string, std::unique_ptr<T>> &map)
{
	auto i = map.find(name);
	if(i != map.end()) return i->second.get();
	else return nullptr;
}

const std::unique_ptr<const Material> * Scene::getMaterial(const std::string &name) const
{
	return Scene::findMapItem<std::unique_ptr<const Material>>(name, materials_);
}

Texture *Scene::getTexture(const std::string &name) const
{
	return Scene::findMapItem<Texture>(name, textures_);
}

const Camera * Scene::getCamera(const std::string &name) const
{
	return Scene::findMapItem<Camera>(name, cameras_);
}

Light *Scene::getLight(const std::string &name) const
{
	return Scene::findMapItem<Light>(name, lights_);
}

ImageOutput *Scene::getOutput(const std::string &name) const
{
	return Scene::findMapItem<ImageOutput>(name, outputs_);
}

Image *Scene::getImage(const std::string &name) const
{
	return Scene::findMapItem<Image>(name, images_);
}

bool Scene::removeOutput(std::string &&name)
{
	ImageOutput *output = getOutput(name);
	if(!output) return false;
	outputs_.erase(name);
	return true;
}

std::map<std::string, Light *> Scene::getLights() const
{
	std::map<std::string, Light *> result;
	for(const auto &light : lights_) result[light.first] = light.second.get();
	if(background_)
	{
		for(const auto light : background_->getLights()) result["background::" + light->getName()] = light;
	}
	return result;
}

std::pair<size_t, ParamError> Scene::createLight(std::string &&name, ParamMap &&params)
{
	if(lights_.find(name) != lights_.end())
	{
		logWarnExist(logger_, Light::getClassName(), name); return {0, {ParamError::Flags::ErrorAlreadyExists}};
	}
	auto [light, param_error] {Light::factory(logger_, *this, name, params)};
	if(light)
	{
		if(logger_.isVerbose())
		{
			if(light->lightEnabled()) logInfoVerboseSuccess(logger_, Light::getClassName(), name, light->type().print());
			else logInfoVerboseSuccessDisabled(logger_, Light::getClassName(), name, light->type().print());
		}
		creation_state_.changes_ |= CreationState::Flags::CLight;
		lights_[name] = std::move(light);
		return {lights_.size() - 1, param_error}; //FIXME: this is just a placeholder for now for future LightID, although this will not work while we still use std::map for lights_
	}
	return {0, ParamError{ParamError::Flags::ErrorWhileCreating}};
}

std::pair<std::unique_ptr<const Material> *, ParamError> Scene::createMaterial(std::string &&name, ParamMap &&params, std::list<ParamMap> &&nodes_params)
{
	std::string pname = "Material";
	if(materials_.find(name) != materials_.end())
	{
		logger_.logDebug("Scene: ", pname, " \"", name, "\" already exists, replacing.");
	}
	else
	{
		materials_[name] = std::make_unique<std::unique_ptr<const Material>>();
	}
	auto [material, param_error]{Material::factory(logger_, *this, name, params, nodes_params)};
	if(material)
	{
		creation_state_.changes_ |= CreationState::Flags::CMaterial;
		++material_index_auto_;
		if(logger_.isVerbose()) logInfoVerboseSuccess(logger_, pname, name, material->type().print());
		*(materials_[name]) = std::move(material);
		return {materials_[name].get(), param_error};
	}
	return {nullptr, ParamError{ParamError::Flags::ErrorWhileCreating}};
}

template <typename T>
std::pair<T *, ParamError> Scene::createMapItem(Logger &logger, std::string &&name, ParamMap &&params, std::map<std::string, std::unique_ptr<T>> &map, const Scene *scene)
{
	if(map.find(name) != map.end())
	{
		logWarnExist(logger, T::getClassName(), name); return {nullptr, {ParamError::Flags::ErrorAlreadyExists}};
	}
	std::unique_ptr<T> item(T::factory(logger, *scene, name, params).first);
	if(item)
	{
		map[name] = std::move(item);
		std::string type;
		params.getParam("type", type);
		if(logger.isVerbose()) logInfoVerboseSuccess(logger, T::getClassName(), name, type);
		return {map[name].get(), {}};
	}
	return {nullptr, ParamError{ParamError::Flags::ErrorWhileCreating}};
}

template <typename T>
std::pair<size_t, ParamError> Scene::createMapItemItemId(Logger &logger, std::string &&name, ParamMap &&params, std::map<std::string, std::unique_ptr<T>> &map, const Scene *scene)
{
	if(map.find(name) != map.end())
	{
		logWarnExist(logger, T::getClassName(), name); return {0, {ParamError::Flags::ErrorAlreadyExists}};
	}
	auto [item, param_error]{T::factory(logger, *scene, name, params)};
	if(item)
	{
		if(logger.isVerbose()) logInfoVerboseSuccess(logger, T::getClassName(), name, item->type().print());
		map[name] = std::move(item);
		return {map.size() - 1, param_error}; //FIXME: this is just a placeholder for now for future ItemID, although this will not work while we still use std::map for items
	}
	return {0, ParamError{ParamError::Flags::ErrorWhileCreating}};
}

template <typename T>
std::pair<T *, ParamError> Scene::createMapItemPointer(Logger &logger, std::string &&name, ParamMap &&params, std::map<std::string, std::unique_ptr<T>> &map, const Scene *scene)
{
	if(map.find(name) != map.end())
	{
		logWarnExist(logger, T::getClassName(), name); return {0, {ParamError::Flags::ErrorAlreadyExists}};
	}
	auto [item, param_error]{T::factory(logger, *scene, name, params)};
	if(item)
	{
		if(logger.isVerbose()) logInfoVerboseSuccess(logger, T::getClassName(), name, item->getType().print());
		map[name] = std::move(item);
		return {map.at(name).get(), param_error}; //FIXME: this is just a placeholder for now for future ItemID, although this will not work while we still use std::map for items
	}
	return {nullptr, ParamError{ParamError::Flags::ErrorWhileCreating}};
}

std::pair<ImageOutput *, ParamError> Scene::createOutput(std::string &&name, ParamMap &&params)
{
	std::string class_name = "ColorOutput";
	if(outputs_.find(name) != outputs_.end())
	{
		logWarnExist(logger_, class_name, name); return {nullptr, {ParamError::Flags::ErrorAlreadyExists}};
	}
	auto [output, param_error]{ImageOutput::factory(logger_, *this, name, params)};
	if(output)
	{
		if(logger_.isVerbose()) logInfoVerboseSuccess(logger_, class_name, name, "");
		outputs_[name] = std::move(output);
		return {outputs_[name].get(), param_error};
	}
	return {nullptr, ParamError{ParamError::Flags::ErrorWhileCreating}};
}

std::pair<size_t, ParamError> Scene::createTexture(std::string &&name, ParamMap &&params)
{
	auto result{createMapItem<Texture>(logger_, std::move(name), std::move(params), textures_, this)};
	InterpolationType texture_interpolation_type = result.first->getInterpolationType();
	if(!render_control_.getDifferentialRaysEnabled() && (texture_interpolation_type == InterpolationType::Trilinear || texture_interpolation_type == InterpolationType::Ewa))
	{
		if(logger_.isVerbose()) logger_.logVerbose("At least one texture using mipmaps interpolation, enabling ray differentials.");
		render_control_.setDifferentialRaysEnabled(true);	//If there is at least one texture using mipmaps, then enable differential rays in the rendering process.
	}
	return {textures_.size(), result.second}; //FIXME: this is just a placeholder for now for future ItemID, although this will not work while we still use std::map for items
}

std::pair<size_t, ParamError> Scene::createCamera(std::string &&name, ParamMap &&params)
{
	return createMapItemItemId<Camera>(logger_, std::move(name), std::move(params), cameras_, this);
}

ParamError Scene::defineBackground(ParamMap &&params)
{
	auto factory{Background::factory(logger_, *this, "background", params)};
	if(logger_.isVerbose() && factory.first) logInfoVerboseSuccess(logger_, factory.first->getClassName(), "", factory.first->type().print());
	//logger_.logParams(result.first->getAsParamMap(true).print()); //TEST CODE ONLY, REMOVE!!
	background_ = std::move(factory.first);
	return factory.second;
}

ParamError Scene::defineSurfaceIntegrator(ParamMap &&params)
{
	auto factory{SurfaceIntegrator::factory(logger_, render_control_, *this, params)};
	if(logger_.isVerbose() && factory.first) logInfoVerboseSuccess(logger_, factory.first->getClassName(), "", factory.first->type().print());
	//logger_.logParams(result.first->getAsParamMap(true).print()); //TEST CODE ONLY, REMOVE!!
	surf_integrator_ = std::move(factory.first);
	return factory.second;
}

ParamError Scene::defineVolumeIntegrator(ParamMap &&params)
{
	auto factory{VolumeIntegrator::factory(logger_, *this, params)};
	if(logger_.isVerbose() && factory.first) logInfoVerboseSuccess(logger_, factory.first->getClassName(), "", factory.first->type().print());
	//logger_.logParams(result.first->getAsParamMap(true).print()); //TEST CODE ONLY, REMOVE!!
	vol_integrator_ = std::move(factory.first);
	return factory.second;
}

std::pair<size_t, ParamError> Scene::createVolumeRegion(std::string &&name, ParamMap &&params)
{
	return createMapItemItemId<VolumeRegion>(logger_, std::move(name), std::move(params), volume_regions_, this);
}

std::pair<RenderView *, ParamError> Scene::createRenderView(std::string &&name, ParamMap &&params)
{
	return createMapItem<RenderView>(logger_, std::move(name), std::move(params), render_views_, this);
}

std::pair<Image *, ParamError> Scene::createImage(std::string &&name, ParamMap &&params)
{
	return createMapItemPointer<Image>(logger_, std::move(name), std::move(params), images_, this);
}

/*! setup the scene for rendering (set camera, background, integrator, create image film,
	set antialiasing etc.)
	attention: since this function creates an image film and asigns it to the scene,
	you need to delete it before deleting the scene!
*/
bool Scene::setupSceneRenderParams(Scene &scene, ParamMap &&param_map)
{
	if(logger_.isDebug()) logger_.logDebug("**Scene::setupSceneRenderParams 'raw' ParamMap\n" + param_map.logContents());
	std::string name;
	std::string aa_dark_detection_type_string = "none";
	AaNoiseParams aa_noise_params;
	int nthreads = -1, nthreads_photons = -1;
	bool adv_auto_shadow_bias_enabled = true;
	float adv_shadow_bias_value = Accelerator::shadowBias();
	bool adv_auto_min_raydist_enabled = true;
	float adv_min_raydist_value = Accelerator::minRayDist();
	int adv_base_sampling_offset = 0;
	int adv_computer_node = 0;
	bool background_resampling = true;  //If false, the background will not be resampled in subsequent adaptative AA passes

	param_map.getParam("AA_passes", aa_noise_params.passes_);
	param_map.getParam("AA_minsamples", aa_noise_params.samples_);
	aa_noise_params.inc_samples_ = aa_noise_params.samples_;
	param_map.getParam("AA_inc_samples", aa_noise_params.inc_samples_);
	param_map.getParam("AA_threshold", aa_noise_params.threshold_);
	param_map.getParam("AA_resampled_floor", aa_noise_params.resampled_floor_);
	param_map.getParam("AA_sample_multiplier_factor", aa_noise_params.sample_multiplier_factor_);
	param_map.getParam("AA_light_sample_multiplier_factor", aa_noise_params.light_sample_multiplier_factor_);
	param_map.getParam("AA_indirect_sample_multiplier_factor", aa_noise_params.indirect_sample_multiplier_factor_);
	param_map.getParam("AA_detect_color_noise", aa_noise_params.detect_color_noise_);
	param_map.getParam("AA_dark_detection_type", aa_dark_detection_type_string);
	param_map.getParam("AA_dark_threshold_factor", aa_noise_params.dark_threshold_factor_);
	param_map.getParam("AA_variance_edge_size", aa_noise_params.variance_edge_size_);
	param_map.getParam("AA_variance_pixels", aa_noise_params.variance_pixels_);
	param_map.getParam("AA_clamp_samples", aa_noise_params.clamp_samples_);
	param_map.getParam("AA_clamp_indirect", aa_noise_params.clamp_indirect_);
	param_map.getParam("threads", nthreads); // number of threads, -1 = auto detection
	param_map.getParam("background_resampling", background_resampling);

	nthreads_photons = nthreads;	//if no "threads_photons" parameter exists, make "nthreads_photons" equal to render threads

	param_map.getParam("threads_photons", nthreads_photons); // number of threads for photon mapping, -1 = auto detection
	param_map.getParam("adv_auto_shadow_bias_enabled", adv_auto_shadow_bias_enabled);
	param_map.getParam("adv_shadow_bias_value", adv_shadow_bias_value);
	param_map.getParam("adv_auto_min_raydist_enabled", adv_auto_min_raydist_enabled);
	param_map.getParam("adv_min_raydist_value", adv_min_raydist_value);
	param_map.getParam("adv_base_sampling_offset", adv_base_sampling_offset); //Base sampling offset, in case of multi-computer rendering each should have a different offset so they don't "repeat" the same samples (user configurable)
	param_map.getParam("adv_computer_node", adv_computer_node); //Computer node in multi-computer render environments/render farms
	param_map.getParam("scene_accelerator", scene_accelerator_); //Computer node in multi-computer render environments/render farms

	defineBasicLayers();
	defineDependentLayers();
	setMaskParams(param_map);
	setEdgeToonParams(param_map);

	image_film_ = std::unique_ptr<ImageFilm>(ImageFilm::factory(logger_, render_control_, param_map, this).first);

	param_map.getParam("filter_type", name); // AA filter type
	std::stringstream aa_settings;
	aa_settings << "AA Settings (" << ((!name.empty()) ? name : "box") << "): Tile size=" << image_film_->getTileSize();
	render_control_.setAaNoiseInfo(aa_settings.str());

	if(aa_dark_detection_type_string == "linear") aa_noise_params.dark_detection_type_ = AaNoiseParams::DarkDetectionType::Linear;
	else if(aa_dark_detection_type_string == "curve") aa_noise_params.dark_detection_type_ = AaNoiseParams::DarkDetectionType::Curve;
	else aa_noise_params.dark_detection_type_ = AaNoiseParams::DarkDetectionType::None;

	creation_state_.changes_ |= CreationState::Flags::COther;
	scene.setAntialiasing(std::move(aa_noise_params));
	scene.setNumThreads(nthreads);
	scene.setNumThreadsPhotons(nthreads_photons);
	scene.shadow_bias_auto_ = adv_auto_shadow_bias_enabled;
	scene.shadow_bias_ = adv_shadow_bias_value;
	scene.ray_min_dist_auto_ = adv_auto_min_raydist_enabled;
	scene.ray_min_dist_ = adv_min_raydist_value;
	if(logger_.isDebug())logger_.logDebug("adv_base_sampling_offset=", adv_base_sampling_offset);
	image_film_->setBaseSamplingOffset(adv_base_sampling_offset);
	image_film_->setComputerNode(adv_computer_node);
	image_film_->setBackgroundResampling(background_resampling);

	return true;
}

void Scene::defineLayer(ParamMap &&params)
{
	if(logger_.isDebug()) logger_.logDebug("**Scene::defineLayer 'raw' ParamMap\n" + params.logContents());
	std::string layer_type_name, image_type_name, exported_image_name, exported_image_type_name;
	params.getParam("type", layer_type_name);
	params.getParam("image_type", image_type_name);
	params.getParam("exported_image_name", exported_image_name);
	params.getParam("exported_image_type", exported_image_type_name);
	defineLayer(std::move(layer_type_name), std::move(image_type_name), std::move(exported_image_type_name), std::move(exported_image_name));
}

void Scene::defineLayer(std::string &&layer_type_name, std::string &&image_type_name, std::string &&exported_image_type_name, std::string &&exported_image_name)
{
	const LayerDef::Type layer_type = LayerDef::getType(layer_type_name);
	const Image::Type image_type = image_type_name.empty() ? LayerDef::getDefaultImageType(layer_type) : Image::getTypeFromName(image_type_name);
	const Image::Type exported_image_type = Image::getTypeFromName(exported_image_type_name);
	defineLayer(layer_type, image_type, exported_image_type, exported_image_name);
}

void Scene::defineLayer(LayerDef::Type layer_type, Image::Type image_type, Image::Type exported_image_type, const std::string &exported_image_name)
{
	if(layer_type == LayerDef::Disabled)
	{
		logger_.logWarning("Scene: cannot create layer '", LayerDef::getName(layer_type), "' of unknown or disabled layer type");
		return;
	}

	if(Layer *existing_layer = layers_.find(layer_type))
	{
		if(existing_layer->getType() == layer_type &&
				existing_layer->getImageType() == image_type &&
				existing_layer->getExportedImageType() == exported_image_type) return;

		if(logger_.isDebug())logger_.logDebug("Scene: had previously defined: ", existing_layer->print());
		if(image_type == Image::Type::None && existing_layer->getImageType() != Image::Type::None)
		{
			if(logger_.isDebug())logger_.logDebug("Scene: the layer '", LayerDef::getName(layer_type), "' had previously a defined internal image which cannot be removed.");
		}
		else existing_layer->setImageType(image_type);

		if(exported_image_type == Image::Type::None && existing_layer->getExportedImageType() != Image::Type::None)
		{
			if(logger_.isDebug())logger_.logDebug("Scene: the layer '", LayerDef::getName(layer_type), "' was previously an exported layer and cannot be changed into an internal layer now.");
		}
		else
		{
			existing_layer->setExportedImageType(exported_image_type);
			existing_layer->setExportedImageName(exported_image_name);
		}
		existing_layer->setType(layer_type);
		logger_.logInfo("Scene: layer redefined: " + existing_layer->print());
	}
	else
	{
		Layer new_layer(layer_type, image_type, exported_image_type, exported_image_name);
		layers_.set(layer_type, new_layer);
		logger_.logInfo("Scene: layer defined: ", new_layer.print());
	}
}

void Scene::defineBasicLayers()
{
	//by default we will have an external/internal Combined layer
	if(!layers_.isDefined(LayerDef::Combined)) defineLayer(LayerDef::Combined, Image::Type::ColorAlpha, Image::Type::ColorAlpha);

	//This auxiliary layer will always be needed for material-specific number of samples calculation
	if(!layers_.isDefined(LayerDef::DebugSamplingFactor)) defineLayer(LayerDef::DebugSamplingFactor, Image::Type::Gray);
}

void Scene::defineDependentLayers()
{
	for(const auto &[layer_def, layer] : layers_)
	{
		switch(layer_def)
		{
			case LayerDef::ZDepthNorm:
				if(!layers_.isDefined(LayerDef::Mist)) defineLayer(LayerDef::Mist);
				break;

			case LayerDef::Mist:
				if(!layers_.isDefined(LayerDef::ZDepthNorm)) defineLayer(LayerDef::ZDepthNorm);
				break;

			case LayerDef::ReflectAll:
				if(!layers_.isDefined(LayerDef::ReflectPerfect)) defineLayer(LayerDef::ReflectPerfect);
				if(!layers_.isDefined(LayerDef::Glossy)) defineLayer(LayerDef::Glossy);
				if(!layers_.isDefined(LayerDef::GlossyIndirect)) defineLayer(LayerDef::GlossyIndirect);
				break;

			case LayerDef::RefractAll:
				if(!layers_.isDefined(LayerDef::RefractPerfect)) defineLayer(LayerDef::RefractPerfect);
				if(!layers_.isDefined(LayerDef::Trans)) defineLayer(LayerDef::Trans);
				if(!layers_.isDefined(LayerDef::TransIndirect)) defineLayer(LayerDef::TransIndirect);
				break;

			case LayerDef::IndirectAll:
				if(!layers_.isDefined(LayerDef::Indirect)) defineLayer(LayerDef::Indirect);
				if(!layers_.isDefined(LayerDef::DiffuseIndirect)) defineLayer(LayerDef::DiffuseIndirect);
				break;

			case LayerDef::ObjIndexMaskAll:
				if(!layers_.isDefined(LayerDef::ObjIndexMask)) defineLayer(LayerDef::ObjIndexMask);
				if(!layers_.isDefined(LayerDef::ObjIndexMaskShadow)) defineLayer(LayerDef::ObjIndexMaskShadow);
				break;

			case LayerDef::MatIndexMaskAll:
				if(!layers_.isDefined(LayerDef::MatIndexMask)) defineLayer(LayerDef::MatIndexMask);
				if(!layers_.isDefined(LayerDef::MatIndexMaskShadow)) defineLayer(LayerDef::MatIndexMaskShadow);
				break;

			case LayerDef::DebugFacesEdges:
				if(!layers_.isDefined(LayerDef::NormalGeom)) defineLayer(LayerDef::NormalGeom, Image::Type::ColorAlpha);
				if(!layers_.isDefined(LayerDef::ZDepthNorm)) defineLayer(LayerDef::ZDepthNorm, Image::Type::GrayAlpha);
				break;

			case LayerDef::DebugObjectsEdges:
				if(!layers_.isDefined(LayerDef::Toon)) defineLayer(LayerDef::Toon, Image::Type::ColorAlpha);
				if(!layers_.isDefined(LayerDef::NormalSmooth)) defineLayer(LayerDef::NormalSmooth, Image::Type::ColorAlpha);
				if(!layers_.isDefined(LayerDef::ZDepthNorm)) defineLayer(LayerDef::ZDepthNorm, Image::Type::GrayAlpha);
				break;

			case LayerDef::Toon:
				if(!layers_.isDefined(LayerDef::DebugObjectsEdges)) defineLayer(LayerDef::DebugObjectsEdges, Image::Type::ColorAlpha);
				if(!layers_.isDefined(LayerDef::NormalSmooth)) defineLayer(LayerDef::NormalSmooth, Image::Type::ColorAlpha);
				if(!layers_.isDefined(LayerDef::ZDepthNorm)) defineLayer(LayerDef::ZDepthNorm, Image::Type::GrayAlpha);
				break;

			default:
				break;
		}
	}
}

void Scene::setMaskParams(const ParamMap &params)
{
	std::string external_pass, internal_pass;
	int mask_obj_index = 0, mask_mat_index = 0;
	bool mask_invert = false;
	bool mask_only = false;

	params.getParam("layer_mask_obj_index", mask_obj_index);
	params.getParam("layer_mask_mat_index", mask_mat_index);
	params.getParam("layer_mask_invert", mask_invert);
	params.getParam("layer_mask_only", mask_only);

	MaskParams mask_params;
	mask_params.obj_index_ = mask_obj_index;
	mask_params.mat_index_ = mask_mat_index;
	mask_params.invert_ = mask_invert;
	mask_params.only_ = mask_only;

	setMaskParams(mask_params);
}

void Scene::setEdgeToonParams(const ParamMap &params)
{
	Rgb toon_edge_color(0.f);
	int object_edge_thickness = 2;
	float object_edge_threshold = 0.3f;
	float object_edge_smoothness = 0.75f;
	float toon_pre_smooth = 3.f;
	float toon_quantization = 0.1f;
	float toon_post_smooth = 3.f;
	int faces_edge_thickness = 1;
	float faces_edge_threshold = 0.01f;
	float faces_edge_smoothness = 0.5f;

	params.getParam("layer_toon_edge_color", toon_edge_color);
	params.getParam("layer_object_edge_thickness", object_edge_thickness);
	params.getParam("layer_object_edge_threshold", object_edge_threshold);
	params.getParam("layer_object_edge_smoothness", object_edge_smoothness);
	params.getParam("layer_toon_pre_smooth", toon_pre_smooth);
	params.getParam("layer_toon_quantization", toon_quantization);
	params.getParam("layer_toon_post_smooth", toon_post_smooth);
	params.getParam("layer_faces_edge_thickness", faces_edge_thickness);
	params.getParam("layer_faces_edge_threshold", faces_edge_threshold);
	params.getParam("layer_faces_edge_smoothness", faces_edge_smoothness);

	EdgeToonParams edge_params;
	edge_params.thickness_ = object_edge_thickness;
	edge_params.threshold_ = object_edge_threshold;
	edge_params.smoothness_ = object_edge_smoothness;
	edge_params.toon_color_ = toon_edge_color;
	edge_params.toon_pre_smooth_ = toon_pre_smooth;
	edge_params.toon_quantization_ = toon_quantization;
	edge_params.toon_post_smooth_ = toon_post_smooth;
	edge_params.face_thickness_ = faces_edge_thickness;
	edge_params.face_threshold_ = faces_edge_threshold;
	edge_params.face_smoothness_ = faces_edge_smoothness;
	setEdgeToonParams(edge_params);
}

void Scene::setRenderNotifyViewCallback(yafaray_RenderNotifyViewCallback_t callback, void *callback_data)
{
	render_callbacks_.notify_view_ = callback;
	render_callbacks_.notify_view_data_ = callback_data;
}

void Scene::setRenderNotifyLayerCallback(yafaray_RenderNotifyLayerCallback_t callback, void *callback_data)
{
	render_callbacks_.notify_layer_ = callback;
	render_callbacks_.notify_layer_data_ = callback_data;
}

void Scene::setRenderPutPixelCallback(yafaray_RenderPutPixelCallback_t callback, void *callback_data)
{
	render_callbacks_.put_pixel_ = callback;
	render_callbacks_.put_pixel_data_ = callback_data;
}

void Scene::setRenderHighlightPixelCallback(yafaray_RenderHighlightPixelCallback_t callback, void *callback_data)
{
	render_callbacks_.highlight_pixel_ = callback;
	render_callbacks_.highlight_pixel_data_ = callback_data;
}

void Scene::setRenderFlushAreaCallback(yafaray_RenderFlushAreaCallback_t callback, void *callback_data)
{
	render_callbacks_.flush_area_ = callback;
	render_callbacks_.flush_area_data_ = callback_data;
}

void Scene::setRenderFlushCallback(yafaray_RenderFlushCallback_t callback, void *callback_data)
{
	render_callbacks_.flush_ = callback;
	render_callbacks_.flush_data_ = callback_data;
}

void Scene::setRenderHighlightAreaCallback(yafaray_RenderHighlightAreaCallback_t callback, void *callback_data)
{
	render_callbacks_.highlight_area_ = callback;
	render_callbacks_.highlight_area_data_ = callback_data;
}

bool Scene::endObject()
{
	if(logger_.isDebug()) logger_.logDebug("Scene::endObject");
	if(creation_state_.stack_.front() != CreationState::Object) return false;
	const bool result = current_object_->calculateObject(creation_state_.current_material_);
	creation_state_.stack_.pop_front();
	return result;
}

bool Scene::smoothVerticesNormals(std::string &&name, float angle)
{
	if(logger_.isDebug()) logger_.logDebug("Scene::smoothVerticesNormals) PR(name) PR(angle");
	//if(creation_state_.stack_.front() != CreationState::Geometry) return false;
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

	if(object->hasVerticesNormals(0) && object->numVerticesNormals(0) == object->numVertices(0))
	{
		object->setSmooth(true);
		return true;
	}
	else return object->smoothVerticesNormals(logger_, angle);
}

int Scene::addVertex(Point3f &&p, int time_step)
{
	//if(logger_.isDebug()) logger.logDebug("Scene::addVertex) PR(p");
	if(creation_state_.stack_.front() != CreationState::Object) return -1;
	current_object_->addPoint(std::move(p), time_step);
	return current_object_->lastVertexId(time_step);
}

int Scene::addVertex(Point3f &&p, Point3f &&orco, int time_step)
{
	if(creation_state_.stack_.front() != CreationState::Object) return -1;
	current_object_->addPoint(std::move(p), time_step);
	current_object_->addOrcoPoint(std::move(orco), time_step);
	return current_object_->lastVertexId(time_step);
}

void Scene::addVertexNormal(Vec3f &&n, int time_step)
{
	if(creation_state_.stack_.front() != CreationState::Object) return;
	current_object_->addVertexNormal(std::move(n), time_step);
}

bool Scene::addFace(std::vector<int> &&vert_indices, std::vector<int> &&uv_indices)
{
	if(creation_state_.stack_.front() != CreationState::Object) return false;
	current_object_->addFace(std::move(vert_indices), std::move(uv_indices), creation_state_.current_material_);
	return true;
}

int Scene::addUv(Uv<float> &&uv)
{
	if(creation_state_.stack_.front() != CreationState::Object) return false;
	return current_object_->addUvValue(std::move(uv));
}

std::pair<size_t, ParamError> Scene::createObject(std::string &&name, ParamMap &&params)
{	std::string pname = "Object";
	if(objects_.find(name) != objects_.end())
	{
		logWarnExist(logger_, Object::getClassName(), name);
		return {0, ParamError{ParamError::Flags::ErrorWhileCreating}};
	}
	auto [object, param_error]{ObjectBase::factory(logger_, *this, name, params)};
	if(object)
	{
		creation_state_.changes_ |= CreationState::Flags::CGeom;
		object->setName(name);
		++object_index_auto_;
		std::string type;
		params.getParam("type", type);
		if(logger_.isVerbose()) logInfoVerboseSuccess(logger_, pname, name, type);
		creation_state_.stack_.push_front(CreationState::Object);
		current_object_ = object.get();
		objects_[name] = std::move(object);
		return {objects_.size() - 1, param_error}; //FIXME: this is just a placeholder for now for future LightID, although this will not work while we still use std::map for objects
	}
	return {0, ParamError{ParamError::Flags::ErrorWhileCreating}};
}

Object *Scene::getObject(const std::string &name) const
{
	auto oi = objects_.find(name);
	if(oi != objects_.end()) return oi->second.get();
	else return nullptr;
}

int Scene::createInstance()
{
	instances_.emplace_back(std::make_unique<ObjectInstance>());
	return static_cast<int>(instances_.size()) - 1;
}

bool Scene::addInstanceObject(int instance_id, std::string &&base_object_name)
{
	const Object *object = getObject(base_object_name);
	if(!object) return false;
	else
	{
		instances_[instance_id]->addPrimitives(object->getPrimitives());
		return true;
	}
}

bool Scene::addInstanceOfInstance(int instance_id, size_t base_instance_id)
{
	const Object *object = instances_[base_instance_id].get();
	if(!object) return false;
	else
	{
		instances_[instance_id]->addPrimitives(object->getPrimitives());
		return true;
	}
}

bool Scene::addInstanceMatrix(int instance_id, Matrix4f &&obj_to_world, float time)
{
	instances_[instance_id]->addObjToWorldMatrix(std::move(obj_to_world), time);
	return true;
}

/*bool Scene::addInstance(const std::string &base_object_name, const Matrix4 &obj_to_world)
{
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
		std::vector<const Primitive *> primitives;
		for(const auto &[object_name, object] : objects_)
		{
			if(object->getVisibility() == Visibility::Invisible) continue;
			const auto prims = object->getPrimitives();
			primitives.insert(primitives.end(), prims.begin(), prims.end());
		}
		auto instances = std::make_unique<Instance>();
		instances->addPrimitives(primitives);
		instances->addObjToWorldMatrix(obj_to_world, 0); //FIXME set proper time for each time step!
		instances_[instance_name] = std::move(instances);
		return true;
	}
	else return false;
}*/

bool Scene::updateObjects()
{
	std::vector<const Primitive *> primitives;
	for(const auto &[object_name, object] : objects_)
	{
		if(object->getVisibility() == Visibility::None) continue;
		if(object->isBaseObject()) continue;
		const auto prims = object->getPrimitives();
		primitives.insert(primitives.end(), prims.begin(), prims.end());
	}
	for(const auto &instance : instances_)
	{
		//if(object->getVisibility() == Visibility::Invisible) continue; //FIXME
		//if(object->isBaseObject()) continue; //FIXME
		const auto prims = instance->getPrimitives();
		primitives.insert(primitives.end(), prims.begin(), prims.end());
	}
	if(primitives.empty())
	{
		logger_.logWarning("Scene: Scene is empty...");
	}
	ParamMap params;
	params["type"] = scene_accelerator_;
	params["accelerator_threads"] = getNumThreads();

	accelerator_ = Accelerator::factory(logger_, primitives, params).first;
	*scene_bound_ = accelerator_->getBound();
	if(logger_.isVerbose()) logger_.logVerbose("Scene: New scene bound is: ", "(", scene_bound_->a_[Axis::X], ", ", scene_bound_->a_[Axis::Y], ", ", scene_bound_->a_[Axis::Z], "), (", scene_bound_->g_[Axis::X], ", ", scene_bound_->g_[Axis::Y], ", ", scene_bound_->g_[Axis::Z], ")");

	object_index_highest_ = 1;
	for(const auto &[object_name, object] : objects_)
	{
		if(object_index_highest_ < object->getIndex()) object_index_highest_ = object->getIndex();
	}

	material_index_highest_ = 1;
	for(const auto &[material_name, material] : materials_)
	{
		if(material_index_highest_ < material->get()->getIndex()) material_index_highest_ = material->get()->getIndex();
	}

	if(shadow_bias_auto_) shadow_bias_ = Accelerator::shadowBias();
	if(ray_min_dist_auto_) ray_min_dist_ = Accelerator::minRayDist();

	logger_.logInfo("Scene: total scene dimensions: X=", scene_bound_->length(Axis::X), ", y=", scene_bound_->length(Axis::Y), ", z=", scene_bound_->length(Axis::Z), ", volume=", scene_bound_->vol(), ", Shadow Bias=", shadow_bias_, (shadow_bias_auto_ ? " (auto)" : ""), ", Ray Min Dist=", ray_min_dist_, (ray_min_dist_auto_ ? " (auto)" : ""));
	return true;
}

} //namespace yafaray
