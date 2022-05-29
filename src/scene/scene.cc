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
#include "camera/camera.h"
#include "common/logger.h"
#include "common/param.h"
#include "common/sysinfo.h"
#include "common/version_build_info.h"
#include "format/format.h"
#include "geometry/matrix4.h"
#include "geometry/object/object.h"
#include "geometry/object/object_instance.h"
#include "geometry/primitive/primitive.h"
#include "geometry/uv.h"
#include "image/image_manipulation.h"
#include "image/image_output.h"
#include "integrator/integrator.h"
#include "light/light.h"
#include "material/material.h"
#include "render/imagefilm.h"
#include "render/progress_bar.h"
#include "render/render_view.h"
#include "texture/texture.h"
#include "volume/volume.h"
#include <limits>
#include <memory>

BEGIN_YAFARAY

void Scene::logWarnExist(Logger &logger, const std::string &pname, const std::string &name)
{
	logger.logWarning("Scene: Sorry, ", pname, " \"", name, "\" already exists!");
}

void Scene::logErrNoType(Logger &logger, const std::string &pname, const std::string &name, const std::string &type)
{
	logger.logError("Scene: ", pname, " type '", type, "' not specified for \"", name, "\" node!");
}

void Scene::logErrOnCreate(Logger &logger, const std::string &pname, const std::string &name, const std::string &t)
{
	logger.logError("Scene: ", "No ", pname, " could be constructed '", t, "'!");
}

void Scene::logInfoVerboseSuccess(Logger &logger, const std::string &pname, const std::string &name, const std::string &t)
{
	logger.logVerbose("Scene: ", "Added ", pname, " '", name, "' (", t, ")!");
}

void Scene::logInfoVerboseSuccessDisabled(Logger &logger, const std::string &pname, const std::string &name, const std::string &t)
{
	logger.logVerbose("Scene: ", "Added ", pname, " '", name, "' (", t, ")! [DISABLED]");
}

Scene::Scene(Logger &logger) : scene_bound_(std::make_unique<Bound>()), logger_(logger)
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
	//Note: keep the std::string or the parameter will be created incorrectly as a bool. This should improve with the new C++17 string literals, but for now with C++11 this should be done.
	param_map["type"] = std::string("shinydiffusemat");
	const std::unique_ptr<const Material> *material = createMaterial("YafaRay_Default_Material", param_map, nodes_params);
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
		nthreads_ = sys_info::getNumSystemThreads();
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
		nthreads_photons_ = sys_info::getNumSystemThreads();
		if(logger_.isVerbose()) logger_.logVerbose("Number of Threads supported for Photon Mapping: [", nthreads_photons_, "].");
	}
	else
	{
		if(logger_.isVerbose()) logger_.logVerbose("Automatic Detection of Threads for Photon Mapping: Inactive.");
	}

	logger_.logParams("Using for Photon Mapping [", nthreads_photons_, "] Threads.");
}

void Scene::setBackground(const Background *bg)
{
	background_ = bg;
}

void Scene::setSurfIntegrator(SurfaceIntegrator *s)
{
	surf_integrator_ = s;
	creation_state_.changes_ |= CreationState::Flags::COther;
}

void Scene::setVolIntegrator(VolumeIntegrator *v)
{
	vol_integrator_ = v;
	creation_state_.changes_ |= CreationState::Flags::COther;
}

const Background *Scene::getBackground() const
{
	return background_;
}

Bound Scene::getSceneBound() const
{
	return *scene_bound_;
}

bool Scene::render()
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

	//if(creation_state_.changes_ != CreationState::Flags::CNone) //FIXME: handle better subsequent scene renders differently if previous render already complete
	{
		if(creation_state_.changes_ & CreationState::Flags::CGeom) updateObjects();
		for(auto &[light_name, light] : getLights()) light->init(*this);
		for(auto &[output_name, output] : outputs_)
		{
			output->init(image_film_->getWidth(), image_film_->getHeight(), image_film_->getExportedImageLayers(), &render_views_);
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
	backgrounds_.clear();
	integrators_.clear();
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

template <typename T>
std::shared_ptr<T> Scene::findMapItem(const std::string &name, const std::map<std::string, std::shared_ptr<T>> &map)
{
	auto i = map.find(name);
	if(i != map.end()) return i->second;
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
	return Scene::findMapItem<const Camera>(name, cameras_);
}

Light *Scene::getLight(const std::string &name) const
{
	return Scene::findMapItem<Light>(name, lights_);
}

const Background * Scene::getBackground(const std::string &name) const
{
	return Scene::findMapItem<const Background>(name, backgrounds_);
}

Integrator *Scene::getIntegrator(const std::string &name) const
{
	return Scene::findMapItem<Integrator>(name, integrators_);
}

ImageOutput *Scene::getOutput(const std::string &name) const
{
	return Scene::findMapItem<ImageOutput>(name, outputs_);
}

std::shared_ptr<Image> Scene::getImage(const std::string &name) const
{
	return Scene::findMapItem<Image>(name, images_);
}

bool Scene::removeOutput(const std::string &name)
{
	ImageOutput *output = getOutput(name);
	if(!output) return false;
	outputs_.erase(name);
	return true;
}

Light *Scene::createLight(const std::string &name, const ParamMap &params)
{
	std::string pname = "Light";
	if(lights_.find(name) != lights_.end())
	{
		logWarnExist(logger_, pname, name); return nullptr;
	}
	std::string type;
	if(! params.getParam("type", type))
	{
		logErrNoType(logger_, pname, name, type); return nullptr;
	}
	auto light = std::unique_ptr<Light>(Light::factory(logger_, *this, name, params));
	if(light)
	{
		lights_[name] = std::move(light);
		if(logger_.isVerbose() && lights_[name]->lightEnabled()) logInfoVerboseSuccess(logger_, pname, name, type);
		else if(logger_.isVerbose()) logInfoVerboseSuccessDisabled(logger_, pname, name, type);
		creation_state_.changes_ |= CreationState::Flags::CLight;
		return lights_[name].get();
	}
	logErrOnCreate(logger_, pname, name, type);
	return nullptr;
}

std::unique_ptr<const Material> * Scene::createMaterial(const std::string &name, const ParamMap &params, const std::list<ParamMap> &nodes_params)
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
	std::string type;
	if(!params.getParam("type", type))
	{
		logErrNoType(logger_, pname, name, type); return nullptr;
	}
	auto material = std::unique_ptr<const Material>(Material::factory(logger_, *this, name, params, nodes_params));
	if(material)
	{
		creation_state_.changes_ |= CreationState::Flags::CMaterial;
		++material_index_auto_;
		*(materials_[name]) = std::move(material);
		if(logger_.isVerbose()) logInfoVerboseSuccess(logger_, pname, name, type);
		return materials_[name].get();
	}
	logErrOnCreate(logger_, pname, name, type);
	return nullptr;
}

template <typename T>
T *Scene::createMapItem(Logger &logger, const std::string &name, const std::string &class_name, const ParamMap &params, std::map<std::string, std::unique_ptr<T>> &map, Scene *scene, bool check_type_exists)
{
	if(map.find(name) != map.end())
	{
		logWarnExist(logger, class_name, name); return nullptr;
	}
	std::string type;
	if(! params.getParam("type", type))
	{
		if(check_type_exists)
		{
			logErrNoType(logger, class_name, name, type);
			return nullptr;
		}
	}
	std::unique_ptr<T> item(T::factory(logger, *scene, name, params));
	if(item)
	{
		map[name] = std::move(item);
		if(logger.isVerbose()) logInfoVerboseSuccess(logger, class_name, name, type);
		return map[name].get();
	}
	if(type != "none") logErrOnCreate(logger, class_name, name, type);
	return nullptr;
}

template <typename T>
std::shared_ptr<T> Scene::createMapItem(Logger &logger, const std::string &name, const std::string &class_name, const ParamMap &params, std::map<std::string, std::shared_ptr<T>> &map, Scene *scene, bool check_type_exists)
{
	if(map.find(name) != map.end())
	{
		logWarnExist(logger, class_name, name); return nullptr;
	}
	std::string type;
	if(! params.getParam("type", type))
	{
		if(check_type_exists)
		{
			logErrNoType(logger, class_name, name, type);
			return nullptr;
		}
	}
	std::shared_ptr<T> item(T::factory(logger, *scene, name, params));
	if(item)
	{
		map[name] = std::move(item);
		if(logger.isVerbose()) logInfoVerboseSuccess(logger, class_name, name, type);
		return map[name];
	}
	logErrOnCreate(logger, class_name, name, type);
	return nullptr;
}

ImageOutput *Scene::createOutput(const std::string &name, const ParamMap &params)
{
	std::string class_name = "ColorOutput";
	if(outputs_.find(name) != outputs_.end())
	{
		logWarnExist(logger_, class_name, name); return nullptr;
	}
	std::unique_ptr<ImageOutput> item(ImageOutput::factory(logger_, *this, name, params));
	if(item)
	{
		outputs_[name] = std::move(item);
		if(logger_.isVerbose()) logInfoVerboseSuccess(logger_, class_name, name, "");
		return outputs_[name].get();
	}
	logErrOnCreate(logger_, class_name, name, "");
	return nullptr;
}

Texture *Scene::createTexture(const std::string &name, const ParamMap &params)
{
	return createMapItem<Texture>(logger_, name, "Texture", params, textures_, this);
}

const Background * Scene::createBackground(const std::string &name, const ParamMap &params)
{
	return createMapItem<const Background>(logger_, name, "Background", params, backgrounds_, this);
}

const Camera *Scene::createCamera(const std::string &name, const ParamMap &params)
{
	return createMapItem<const Camera>(logger_, name, "Camera", params, cameras_, this);
}

Integrator *Scene::createIntegrator(const std::string &name, const ParamMap &params)
{
	return createMapItem<Integrator>(logger_, name, "Integrator", params, integrators_, this);
}

VolumeRegion *Scene::createVolumeRegion(const std::string &name, const ParamMap &params)
{
	return createMapItem<VolumeRegion>(logger_, name, "VolumeRegion", params, volume_regions_, this);
}

RenderView *Scene::createRenderView(const std::string &name, const ParamMap &params)
{
	return createMapItem<RenderView>(logger_, name, "RenderView", params, render_views_, this, false);
}

std::shared_ptr<Image> Scene::createImage(const std::string &name, const ParamMap &params)
{
	return createMapItem<Image>(logger_, name, "Image", params, images_, this);
}

/*! setup the scene for rendering (set camera, background, integrator, create image film,
	set antialiasing etc.)
	attention: since this function creates an image film and asigns it to the scene,
	you need to delete it before deleting the scene!
*/
bool Scene::setupSceneRenderParams(Scene &scene, const ParamMap &params)
{
	if(logger_.isDebug())
	{
		logger_.logDebug("**Scene::setupSceneRenderParams");
		params.logContents(logger_);
	}
	std::string name;
	std::string aa_dark_detection_type_string = "none";
	AaNoiseParams aa_noise_params;
	int nthreads = -1, nthreads_photons = -1;
	bool adv_auto_shadow_bias_enabled = true;
	float adv_shadow_bias_value = shadow_bias_global;
	bool adv_auto_min_raydist_enabled = true;
	float adv_min_raydist_value = min_raydist_global;
	int adv_base_sampling_offset = 0;
	int adv_computer_node = 0;
	bool background_resampling = true;  //If false, the background will not be resampled in subsequent adaptative AA passes

	if(!params.getParam("integrator_name", name))
	{
		logger_.logError("Scene: ", "Specify an Integrator!!");
		return false;
	}
	Integrator *integrator = getIntegrator(name);
	if(!integrator)
	{
		logger_.logError("Scene: ", "Specify an _existing_ Integrator!!");
		return false;
	}
	if(integrator->getType() != Integrator::Surface)
	{
		logger_.logError("Scene: ", "Integrator '", name, "' is not a surface integrator!");
		return false;
	}

	Integrator *volume_integrator = nullptr;
	if(params.getParam("volintegrator_name", name))
	{
		volume_integrator = getIntegrator(name);
		if(volume_integrator && volume_integrator->getType() != Integrator::Volume)
		{
			logger_.logError("Scene: ", "Integrator '", name, "' is not a volume integrator!");
			return false;
		}
	}

	const Background *background = nullptr;
	if(params.getParam("background_name", name))
	{
		background = getBackground(name);
		if(!background) logger_.logError("Scene: ", "please specify an _existing_ Background!!");
	}

	params.getParam("AA_passes", aa_noise_params.passes_);
	params.getParam("AA_minsamples", aa_noise_params.samples_);
	aa_noise_params.inc_samples_ = aa_noise_params.samples_;
	params.getParam("AA_inc_samples", aa_noise_params.inc_samples_);
	params.getParam("AA_threshold", aa_noise_params.threshold_);
	params.getParam("AA_resampled_floor", aa_noise_params.resampled_floor_);
	params.getParam("AA_sample_multiplier_factor", aa_noise_params.sample_multiplier_factor_);
	params.getParam("AA_light_sample_multiplier_factor", aa_noise_params.light_sample_multiplier_factor_);
	params.getParam("AA_indirect_sample_multiplier_factor", aa_noise_params.indirect_sample_multiplier_factor_);
	params.getParam("AA_detect_color_noise", aa_noise_params.detect_color_noise_);
	params.getParam("AA_dark_detection_type", aa_dark_detection_type_string);
	params.getParam("AA_dark_threshold_factor", aa_noise_params.dark_threshold_factor_);
	params.getParam("AA_variance_edge_size", aa_noise_params.variance_edge_size_);
	params.getParam("AA_variance_pixels", aa_noise_params.variance_pixels_);
	params.getParam("AA_clamp_samples", aa_noise_params.clamp_samples_);
	params.getParam("AA_clamp_indirect", aa_noise_params.clamp_indirect_);
	params.getParam("threads", nthreads); // number of threads, -1 = auto detection
	params.getParam("background_resampling", background_resampling);

	nthreads_photons = nthreads;	//if no "threads_photons" parameter exists, make "nthreads_photons" equal to render threads

	params.getParam("threads_photons", nthreads_photons); // number of threads for photon mapping, -1 = auto detection
	params.getParam("adv_auto_shadow_bias_enabled", adv_auto_shadow_bias_enabled);
	params.getParam("adv_shadow_bias_value", adv_shadow_bias_value);
	params.getParam("adv_auto_min_raydist_enabled", adv_auto_min_raydist_enabled);
	params.getParam("adv_min_raydist_value", adv_min_raydist_value);
	params.getParam("adv_base_sampling_offset", adv_base_sampling_offset); //Base sampling offset, in case of multi-computer rendering each should have a different offset so they don't "repeat" the same samples (user configurable)
	params.getParam("adv_computer_node", adv_computer_node); //Computer node in multi-computer render environments/render farms
	params.getParam("scene_accelerator", scene_accelerator_); //Computer node in multi-computer render environments/render farms

	defineBasicLayers();
	defineDependentLayers();
	setMaskParams(params);
	setEdgeToonParams(params);

	image_film_ = std::unique_ptr<ImageFilm>(ImageFilm::factory(logger_, params, this));

	params.getParam("filter_type", name); // AA filter type
	std::stringstream aa_settings;
	aa_settings << "AA Settings (" << ((!name.empty()) ? name : "box") << "): Tile size=" << image_film_->getTileSize();
	render_control_.setAaNoiseInfo(aa_settings.str());

	if(aa_dark_detection_type_string == "linear") aa_noise_params.dark_detection_type_ = AaNoiseParams::DarkDetectionType::Linear;
	else if(aa_dark_detection_type_string == "curve") aa_noise_params.dark_detection_type_ = AaNoiseParams::DarkDetectionType::Curve;
	else aa_noise_params.dark_detection_type_ = AaNoiseParams::DarkDetectionType::None;

	scene.setSurfIntegrator(static_cast<SurfaceIntegrator *>(integrator));
	scene.setVolIntegrator(static_cast<VolumeIntegrator *>(volume_integrator));
	scene.setAntialiasing(aa_noise_params);
	scene.setNumThreads(nthreads);
	scene.setNumThreadsPhotons(nthreads_photons);
	if(background) scene.setBackground(background);
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

bool Scene::setupSceneProgressBar(Scene &scene, const std::shared_ptr<ProgressBar> &progress_bar)
{
	if(logger_.isDebug())
	{
		logger_.logDebug("**Scene::setupSceneProgressBar");
	}

	if(progress_bar)
	{
		if(image_film_) image_film_->setProgressBar(progress_bar);
		else logger_.logError("Scene: image film does not exist, cannot set progress bar");

		if(getSurfIntegrator()) getSurfIntegrator()->setProgressBar(progress_bar);
		else logger_.logError("Scene: integrator does not exist, cannot set progress bar");
	}
	return true;
}

void Scene::defineLayer(const ParamMap &params)
{
	if(logger_.isDebug())
	{
		logger_.logDebug("**Scene::defineLayer");
		params.logContents(logger_);
	}
	std::string layer_type_name, image_type_name, exported_image_name, exported_image_type_name;
	params.getParam("type", layer_type_name);
	params.getParam("image_type", image_type_name);
	params.getParam("exported_image_name", exported_image_name);
	params.getParam("exported_image_type", exported_image_type_name);
	defineLayer(layer_type_name, image_type_name, exported_image_type_name, exported_image_name);
}

void Scene::defineLayer(const std::string &layer_type_name, const std::string &image_type_name, const std::string &exported_image_type_name, const std::string &exported_image_name)
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
				if(!layers_.isDefined(LayerDef::NormalSmooth)) defineLayer(LayerDef::NormalSmooth, Image::Type::ColorAlpha);
				if(!layers_.isDefined(LayerDef::ZDepthNorm)) defineLayer(LayerDef::ZDepthNorm, Image::Type::GrayAlpha);
				break;

			case LayerDef::Toon:
				if(!layers_.isDefined(LayerDef::DebugObjectsEdges)) defineLayer(LayerDef::DebugObjectsEdges, Image::Type::ColorAlpha);
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
	mask_params.obj_index_ = static_cast<float>(mask_obj_index);
	mask_params.mat_index_ = static_cast<float>(mask_mat_index);
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

bool Scene::smoothVerticesNormals(const std::string &name, float angle)
{
	if(logger_.isDebug()) logger_.logDebug("Scene::smoothVerticesNormals) PR(name) PR(angle");
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

	if(object->hasVerticesNormals(0) && object->numVerticesNormals(0) == object->numVertices(0))
	{
		object->setSmooth(true);
		return true;
	}
	else return object->smoothVerticesNormals(logger_, angle);
}

int Scene::addVertex(const Point3 &p, size_t time_step)
{
	//if(logger_.isDebug()) logger.logDebug("Scene::addVertex) PR(p");
	if(creation_state_.stack_.front() != CreationState::Object) return -1;
	current_object_->addPoint(p, time_step);
	return current_object_->lastVertexId(time_step);
}

int Scene::addVertex(const Point3 &p, const Point3 &orco, size_t time_step)
{
	if(creation_state_.stack_.front() != CreationState::Object) return -1;
	current_object_->addPoint(p, time_step);
	current_object_->addOrcoPoint(orco, time_step);
	return current_object_->lastVertexId(time_step);
}

void Scene::addVertexNormal(const Vec3 &n, size_t time_step)
{
	if(creation_state_.stack_.front() != CreationState::Object) return;
	current_object_->addVertexNormal(n, time_step);
}

bool Scene::addFace(const std::vector<int> &vert_indices, const std::vector<int> &uv_indices)
{
	if(creation_state_.stack_.front() != CreationState::Object) return false;
	current_object_->addFace(vert_indices, uv_indices, creation_state_.current_material_);
	return true;
}

int Scene::addUv(float u, float v)
{
	if(creation_state_.stack_.front() != CreationState::Object) return false;
	return current_object_->addUvValue({u, v});
}

Object *Scene::createObject(const std::string &name, const ParamMap &params)
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
	std::unique_ptr<Object> object(Object::factory(logger_, *this, name, params));
	if(object)
	{
		creation_state_.changes_ |= CreationState::Flags::CGeom;
		object->setName(name);
		++object_index_auto_;
		objects_[name] = std::move(object);
		if(logger_.isVerbose()) logInfoVerboseSuccess(logger_, pname, name, type);
		creation_state_.stack_.push_front(CreationState::Object);
		current_object_ = objects_[name].get();
		return objects_[name].get();
	}
	logErrOnCreate(logger_, pname, name, type);
	return nullptr;
}

Object *Scene::getObject(const std::string &name) const
{
	auto oi = objects_.find(name);
	if(oi != objects_.end()) return oi->second.get();
	else return nullptr;
}

size_t Scene::createInstance()
{
	instances_.emplace_back(std::make_unique<ObjectInstance>());
	return instances_.size() - 1;
}

bool Scene::addInstanceObject(size_t instance_id, const std::string &base_object_name)
{
	const Object *object = getObject(base_object_name);
	if(!object) return false;
	else
	{
		instances_[instance_id]->addPrimitives(object->getPrimitives());
		return true;
	}
}

bool Scene::addInstanceOfInstance(size_t instance_id, size_t base_instance_id)
{
	const Object *object = instances_[base_instance_id].get();
	if(!object) return false;
	else
	{
		instances_[instance_id]->addPrimitives(object->getPrimitives());
		return true;
	}
}

bool Scene::addInstanceMatrix(size_t instance_id, const Matrix4 &obj_to_world, float time)
{
	instances_[instance_id]->addObjToWorldMatrix(obj_to_world, time);
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
		if(object->getVisibility() == Visibility::Invisible) continue;
		if(object->isBaseObject()) continue;
		const auto prims = object->getPrimitives();
		primitives.insert(primitives.end(), prims.begin(), prims.end());
	}
	for(const auto &instance : instances_)
	{
		//if(object->getVisibility() == Visibility::Invisible) continue;
		const auto prims = instance->getPrimitives();
		primitives.insert(primitives.end(), prims.begin(), prims.end());
	}
	if(primitives.empty())
	{
		logger_.logWarning("Scene: Scene is empty...");
	}
	ParamMap params;
	params["type"] = scene_accelerator_;
	params["num_primitives"] = static_cast<int>(primitives.size());
	params["accelerator_threads"] = getNumThreads();

	accelerator_ = std::unique_ptr<const Accelerator>(Accelerator::factory(logger_, primitives, params));
	*scene_bound_ = accelerator_->getBound();
	if(logger_.isVerbose()) logger_.logVerbose("Scene: New scene bound is: ", "(", scene_bound_->a_.x(), ", ", scene_bound_->a_.y(), ", ", scene_bound_->a_.z(), "), (", scene_bound_->g_.x(), ", ", scene_bound_->g_.y(), ", ", scene_bound_->g_.z(), ")");

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

	if(shadow_bias_auto_) shadow_bias_ = shadow_bias_global;
	if(ray_min_dist_auto_) ray_min_dist_ = min_raydist_global;

	logger_.logInfo("Scene: total scene dimensions: X=", scene_bound_->longX(), ", y=", scene_bound_->longY(), ", z=", scene_bound_->longZ(), ", volume=", scene_bound_->vol(), ", Shadow Bias=", shadow_bias_, (shadow_bias_auto_ ? " (auto)" : ""), ", Ray Min Dist=", ray_min_dist_, (ray_min_dist_auto_ ? " (auto)" : ""));
	return true;
}

END_YAFARAY
