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
#include "scene/scene_yafaray.h"
#include "common/session.h"
#include "common/logging.h"
#include "common/sysinfo.h"
#include "object_geom/triangle.h"
#include "accelerator/accelerator_kdtree.h"
#include "common/param.h"
#include "light/light.h"
#include "material/material.h"
#include "integrator/integrator.h"
#include "texture/texture.h"
#include "background/background.h"
#include "camera/camera.h"
#include "shader/shader_node.h"
#include "common/imagefilm.h"
#include "imagehandler/imagehandler.h"
#include "volume/volume.h"
#include "output/output.h"

#include <iostream>
#include <limits>

BEGIN_YAFARAY

#define Y_INFO_SCENE Y_INFO << "Scene: "
#define Y_VERBOSE_SCENE Y_VERBOSE << "Scene: "
#define Y_ERROR_SCENE Y_ERROR << "Scene: "
#define Y_WARN_SCENE Y_WARNING << "Scene: "

#define WARN_EXIST Y_WARN_SCENE << "Sorry, " << pname << " \"" << name << "\" already exists!" << YENDL

#define ERR_NO_TYPE Y_ERROR_SCENE << pname << " type not specified for \"" << name << "\" node!" << YENDL
#define ERR_ON_CREATE(t) Y_ERROR_SCENE << "No " << pname << " could be constructed '" << t << "'!" << YENDL

#define INFO_VERBOSE_SUCCESS(name, t) Y_VERBOSE_SCENE << "Added " << pname << " '"<< name << "' (" << t << ")!" << YENDL
#define INFO_VERBOSE_SUCCESS_DISABLED(name, t) Y_VERBOSE_SCENE << "Added " << pname << " '"<< name << "' (" << t << ")! [DISABLED]" << YENDL

Scene *Scene::factory(ParamMap &params)
{
	std::string type;
	params.getParam("type", type);
	if(type == "yafaray") return YafaRayScene::factory(params);
	else return nullptr;
}

Scene::Scene()
{
	state_.changes_ = CAll;
	state_.stack_.push_front(Ready);
	state_.next_free_id_ = std::numeric_limits<int>::max();
	state_.cur_obj_ = nullptr;

	std::string compiler = YAFARAY_BUILD_COMPILER;
	if(!YAFARAY_BUILD_PLATFORM.empty()) compiler = YAFARAY_BUILD_PLATFORM + "-" + YAFARAY_BUILD_COMPILER;

	Y_INFO << "LibYafaRay (" << YAFARAY_BUILD_VERSION << ")" << " " << YAFARAY_BUILD_OS << " " << YAFARAY_BUILD_ARCHITECTURE << " (" << compiler << ")" << YENDL;
	output_2_ = nullptr;
	session__.setDifferentialRaysEnabled(false);	//By default, disable ray differential calculations. Only if at least one texture uses them, then enable differentials.

#ifndef HAVE_OPENCV
	Y_WARNING << "libYafaRay built without OpenCV support. The following functionality will not work: image output denoise, background IBL blur, object/face edge render passes, toon render pass." << YENDL;
#endif
}

Scene::~Scene()
{
	clearAll();
}

void Scene::abort()
{
	sig_mutex_.lock();
	signals_ |= Y_SIG_ABORT;
	sig_mutex_.unlock();
}

int Scene::getSignals() const
{
	int sig;
	sig_mutex_.lock();
	sig = signals_;
	sig_mutex_.unlock();
	return sig;
}

bool Scene::startGeometry()
{
	if(state_.stack_.front() != Ready) return false;
	state_.stack_.push_front(Geometry);
	return true;
}

bool Scene::endGeometry()
{
	if(state_.stack_.front() != Geometry) return false;
	// in case objects share arrays, so they all need to be updated
	// after each object change, uncomment the below block again:
	// don't forget to update the mesh object iterators!
	/*	for(auto i=meshes.begin();
			 i!=meshes.end(); ++i)
		{
			objData_t &dat = (*i).second;
			dat.obj->setContext(dat.points.begin(), dat.normals.begin() );
		}*/
	state_.stack_.pop_front();
	return true;
}

void Scene::setNumThreads(int threads)
{
	nthreads_ = threads;

	if(nthreads_ == -1) //Automatic detection of number of threads supported by this system, taken from Blender. (DT)
	{
		Y_VERBOSE << "Automatic Detection of Threads: Active." << YENDL;
		const SysInfo sys_info;
		nthreads_ = sys_info.getNumSystemThreads();
		Y_VERBOSE << "Number of Threads supported: [" << nthreads_ << "]." << YENDL;
	}
	else
	{
		Y_VERBOSE << "Automatic Detection of Threads: Inactive." << YENDL;
	}

	Y_PARAMS << "Using [" << nthreads_ << "] Threads." << YENDL;

	std::stringstream set;
	set << "CPU threads=" << nthreads_ << std::endl;

	logger__.appendRenderSettings(set.str());
}

void Scene::setNumThreadsPhotons(int threads_photons)
{
	nthreads_photons_ = threads_photons;

	if(nthreads_photons_ == -1) //Automatic detection of number of threads supported by this system, taken from Blender. (DT)
	{
		Y_VERBOSE << "Automatic Detection of Threads for Photon Mapping: Active." << YENDL;
		const SysInfo sys_info;
		nthreads_photons_ = sys_info.getNumSystemThreads();
		Y_VERBOSE << "Number of Threads supported for Photon Mapping: [" << nthreads_photons_ << "]." << YENDL;
	}
	else
	{
		Y_VERBOSE << "Automatic Detection of Threads for Photon Mapping: Inactive." << YENDL;
	}

	Y_PARAMS << "Using for Photon Mapping [" << nthreads_photons_ << "] Threads." << YENDL;
}

void Scene::setCamera(Camera *cam)
{
	camera_ = cam;
}

void Scene::setImageFilm(ImageFilm *film)
{
	image_film_ = film;
}

void Scene::setBackground(Background *bg)
{
	background_ = bg;
}

void Scene::setSurfIntegrator(SurfaceIntegrator *s)
{
	surf_integrator_ = s;
	surf_integrator_->setScene(this);
	state_.changes_ |= COther;
}

void Scene::setVolIntegrator(VolumeIntegrator *v)
{
	vol_integrator_ = v;
	vol_integrator_->setScene(this);
	state_.changes_ |= COther;
}

Background *Scene::getBackground() const
{
	return background_;
}

Bound Scene::getSceneBound() const
{
	return scene_bound_;
}

bool Scene::render()
{
	sig_mutex_.lock();
	signals_ = 0;
	sig_mutex_.unlock();

	bool success = false;

	const std::map<std::string, Camera *> *camera_table = getCameraTable();

	if(camera_table->size() == 0)
	{
		Y_ERROR << "No cameras/views found, exiting." << YENDL;
		return false;
	}

	for(auto cam_table_entry = camera_table->begin(); cam_table_entry != camera_table->end(); ++cam_table_entry)
	{
		int num_view = distance(camera_table->begin(), cam_table_entry);
		Camera *cam = cam_table_entry->second;
		setCamera(cam);
		if(!update()) return false;

		success = surf_integrator_->render(num_view, image_film_);

		surf_integrator_->cleanup();
		image_film_->flush(num_view);
	}

	return success;
}

ObjId_t Scene::getNextFreeId()
{
	return --state_.next_free_id_;
}

void Scene::clearNonGeometry()
{
	freeMap(lights_);
	freeMap(textures_);
	freeMap(materials_);
	freeMap(cameras_);
	freeMap(backgrounds_);
	freeMap(integrators_);
	freeMap(volume_handlers_);
	freeMap(volume_regions_);
	freeMap(imagehandlers_);

	lights_.clear();
	textures_.clear();
	materials_.clear();
	cameras_.clear();
	backgrounds_.clear();
	integrators_.clear();
	volume_handlers_.clear();
	volume_regions_.clear();
	imagehandlers_.clear();
}

void Scene::clearAll()
{
	clearGeometry();
	clearNonGeometry();
}

Material *Scene::getMaterial(const std::string &name) const
{
	auto i = materials_.find(name);
	if(i != materials_.end()) return i->second;
	else return nullptr;
}

Texture *Scene::getTexture(const std::string &name) const
{
	auto i = textures_.find(name);
	if(i != textures_.end()) return i->second;
	else return nullptr;
}

Camera *Scene::getCamera(const std::string &name) const
{
	auto i = cameras_.find(name);
	if(i != cameras_.end()) return i->second;
	else return nullptr;
}

Background *Scene::getBackground(const std::string &name) const
{
	auto i = backgrounds_.find(name);
	if(i != backgrounds_.end()) return i->second;
	else return nullptr;
}

Integrator *Scene::getIntegrator(const std::string &name) const
{
	auto i = integrators_.find(name);
	if(i != integrators_.end()) return i->second;
	else return nullptr;
}

ShaderNode *Scene::getShaderNode(const std::string &name) const
{
	auto i = shaders_.find(name);
	if(i != shaders_.end()) return i->second;
	else return nullptr;
}

Light *Scene::createLight(const std::string &name, ParamMap &params)
{
	std::string pname = "Light";
	if(lights_.find(name) != lights_.end())
	{
		WARN_EXIST; return nullptr;
	}
	std::string type;
	if(! params.getParam("type", type))
	{
		ERR_NO_TYPE; return nullptr;
	}
	Light *light = Light::factory(params, *this);
	if(light)
	{
		lights_[name] = light;
		if(light->lightEnabled()) INFO_VERBOSE_SUCCESS(name, type);
		else INFO_VERBOSE_SUCCESS_DISABLED(name, type);
		state_.changes_ |= CLight;
		return light;
	}
	ERR_ON_CREATE(type);
	return nullptr;
}

Texture *Scene::createTexture(const std::string &name, ParamMap &params)
{
	std::string pname = "Texture";
	if(textures_.find(name) != textures_.end())
	{
		WARN_EXIST; return nullptr;
	}
	std::string type;
	if(! params.getParam("type", type))
	{
		ERR_NO_TYPE; return nullptr;
	}
	Texture *texture = Texture::factory(params, *this);
	if(texture)
	{
		textures_[name] = texture;
		INFO_VERBOSE_SUCCESS(name, type);
		return texture;
	}
	ERR_ON_CREATE(type);
	return nullptr;
}

ShaderNode *Scene::createShaderNode(const std::string &name, ParamMap &params)
{
	std::string pname = "ShaderNode";
	if(shaders_.find(name) != shaders_.end())
	{
		WARN_EXIST; return nullptr;
	}
	std::string type;
	if(! params.getParam("type", type))
	{
		ERR_NO_TYPE; return nullptr;
	}
	ShaderNode *shader = ShaderNode::factory(params, *this);
	if(shader)
	{
		shaders_[name] = shader;
		INFO_VERBOSE_SUCCESS(name, type);
		return shader;
	}
	ERR_ON_CREATE(type);
	return nullptr;
}

Material *Scene::createMaterial(const std::string &name, ParamMap &params, std::list<ParamMap> &eparams)
{
	std::string pname = "Material";
	if(materials_.find(name) != materials_.end())
	{
		WARN_EXIST; return nullptr;
	}
	std::string type;
	if(! params.getParam("type", type))
	{
		ERR_NO_TYPE; return nullptr;
	}
	params["name"] = name;
	Material *material = Material::factory(params, eparams, *this);
	if(material)
	{
		materials_[name] = material;
		INFO_VERBOSE_SUCCESS(name, type);
		return material;
	}
	ERR_ON_CREATE(type);
	return nullptr;
}

Background *Scene::createBackground(const std::string &name, ParamMap &params)
{
	std::string pname = "Background";
	if(backgrounds_.find(name) != backgrounds_.end())
	{
		WARN_EXIST; return nullptr;
	}
	std::string type;
	if(! params.getParam("type", type))
	{
		ERR_NO_TYPE; return nullptr;
	}
	Background *background = Background::factory(params, *this);
	if(background)
	{
		backgrounds_[name] = background;
		INFO_VERBOSE_SUCCESS(name, type);
		return background;
	}
	ERR_ON_CREATE(type);
	return nullptr;
}

ImageHandler *Scene::createImageHandler(const std::string &name, ParamMap &params)
{
	const std::string pname = "ImageHandler";
	std::string type;
	if(! params.getParam("type", type))
	{
		ERR_NO_TYPE; return nullptr;
	}
	ImageHandler *ih = ImageHandler::factory(params, *this);
	if(ih)
	{
		INFO_VERBOSE_SUCCESS(name, type);
		return ih;
	}

	ERR_ON_CREATE(type);
	return nullptr;
}

Camera *Scene::createCamera(const std::string &name, ParamMap &params)
{
	std::string pname = "Camera";
	if(cameras_.find(name) != cameras_.end())
	{
		WARN_EXIST; return nullptr;
	}
	std::string type;
	if(! params.getParam("type", type))
	{
		ERR_NO_TYPE; return nullptr;
	}
	Camera *camera = Camera::factory(params, *this);
	if(camera)
	{
		cameras_[name] = camera;
		INFO_VERBOSE_SUCCESS(name, type);
		int view_number = passes_settings_.view_names_.size();
		camera->setCameraName(name);
		passes_settings_.view_names_.push_back(camera->getViewName());

		Y_INFO << "Environment: View number=" << view_number << ", view name: '" << passes_settings_.view_names_[view_number] << "', camera name: '" << camera->getCameraName() << "'" << YENDL;

		return camera;
	}
	ERR_ON_CREATE(type);
	return nullptr;
}

Integrator *Scene::createIntegrator(const std::string &name, ParamMap &params)
{
	std::string pname = "Integrator";
	if(integrators_.find(name) != integrators_.end())
	{
		WARN_EXIST; return nullptr;
	}
	std::string type;
	if(! params.getParam("type", type))
	{
		ERR_NO_TYPE; return nullptr;
	}
	Integrator *integrator = Integrator::factory(params, *this);
	if(integrator)
	{
		integrators_[name] = integrator;
		INFO_VERBOSE_SUCCESS(name, type);
		return integrator;
	}
	ERR_ON_CREATE(type);
	return nullptr;
}

void Scene::createRenderPass(const std::string &ext_pass_name, const std::string &int_pass_name, const ImageType &image_type)
{
	passes_settings_.extPassAdd(ext_pass_name, int_pass_name, image_type);
}

void Scene::setupRenderPasses(const ParamMap &params)
{
	std::string external_pass, internal_pass;
	int pass_mask_obj_index = 0, pass_mask_mat_index = 0;
	bool pass_mask_invert = false;
	bool pass_mask_only = false;

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

	params.getParam("pass_mask_obj_index", pass_mask_obj_index);
	params.getParam("pass_mask_mat_index", pass_mask_mat_index);
	params.getParam("pass_mask_invert", pass_mask_invert);
	params.getParam("pass_mask_only", pass_mask_only);

	params.getParam("toonEdgeColor", toon_edge_color);
	params.getParam("objectEdgeThickness", object_edge_thickness);
	params.getParam("objectEdgeThreshold", object_edge_threshold);
	params.getParam("objectEdgeSmoothness", object_edge_smoothness);
	params.getParam("toonPreSmooth", toon_pre_smooth);
	params.getParam("toonQuantization", toon_quantization);
	params.getParam("toonPostSmooth", toon_post_smooth);
	params.getParam("facesEdgeThickness", faces_edge_thickness);
	params.getParam("facesEdgeThreshold", faces_edge_threshold);
	params.getParam("facesEdgeSmoothness", faces_edge_smoothness);

	PassMaskParams mask_params;
	mask_params.obj_index_ = (float) pass_mask_obj_index;
	mask_params.mat_index_ = (float) pass_mask_mat_index;
	mask_params.invert_ = pass_mask_invert;
	mask_params.only_ = pass_mask_only;
	passes_settings_.setPassMaskParams(mask_params);

	PassEdgeToonParams edge_params;
	edge_params.thickness_ = object_edge_thickness;
	edge_params.threshold_ = object_edge_threshold;
	edge_params.smoothness_ = object_edge_smoothness;
	edge_params.toon_color_[0] = toon_edge_color.r_;
	edge_params.toon_color_[1] = toon_edge_color.g_;
	edge_params.toon_color_[2] = toon_edge_color.b_;
	edge_params.toon_pre_smooth_ = toon_pre_smooth;
	edge_params.toon_quantization_ = toon_quantization;
	edge_params.toon_post_smooth_ = toon_post_smooth;
	edge_params.face_thickness_ = faces_edge_thickness;
	edge_params.face_threshold_ = faces_edge_threshold;
	edge_params.face_smoothness_ = faces_edge_smoothness;
	passes_settings_.setPassEdgeToonParams(edge_params);

	//Generate any necessary auxiliar render passes
	passes_settings_.auxPassesGenerate();
}

ImageFilm *Scene::createImageFilm(const ParamMap &params, ColorOutput &output)
{
	std::string name;
	std::string tiles_order;
	int width = 320, height = 240, xstart = 0, ystart = 0;
	std::string color_space_string = "Raw_Manual_Gamma";
	ColorSpace color_space = RawManualGamma;
	std::string color_space_string_2 = "Raw_Manual_Gamma";
	ColorSpace color_space_2 = RawManualGamma;
	float filt_sz = 1.5, gamma = 1.f, gamma_2 = 1.f;
	bool show_sampled_pixels = false;
	int tile_size = 32;
	bool premult = false;
	bool premult_2 = false;
	std::string images_autosave_interval_type_string = "none";
	ImageFilm::AutoSaveParams images_autosave_params;
	std::string film_save_load_string = "none";
	ImageFilm::FilmSaveLoad film_save_load = ImageFilm::FilmSaveLoad::None;
	std::string film_autosave_interval_type_string = "none";
	ImageFilm::AutoSaveParams film_autosave_params;

	params.getParam("color_space", color_space_string);
	params.getParam("gamma", gamma);
	params.getParam("color_space2", color_space_string_2);
	params.getParam("gamma2", gamma_2);
	params.getParam("AA_pixelwidth", filt_sz);
	params.getParam("width", width); // width of rendered image
	params.getParam("height", height); // height of rendered image
	params.getParam("xstart", xstart); // x-offset (for cropped rendering)
	params.getParam("ystart", ystart); // y-offset (for cropped rendering)
	params.getParam("filter_type", name); // AA filter type
	params.getParam("show_sam_pix", show_sampled_pixels); // Show pixels marked to be resampled on adaptative sampling
	params.getParam("tile_size", tile_size); // Size of the render buckets or tiles
	params.getParam("tiles_order", tiles_order); // Order of the render buckets or tiles
	params.getParam("premult", premult); // Premultipy Alpha channel for better alpha antialiasing against bg
	params.getParam("premult2", premult_2); // Premultipy Alpha channel for better alpha antialiasing against bg, for the optional secondary output
	params.getParam("images_autosave_interval_type", images_autosave_interval_type_string);
	params.getParam("images_autosave_interval_passes", images_autosave_params.interval_passes_);
	params.getParam("images_autosave_interval_seconds", images_autosave_params.interval_seconds_);
	params.getParam("film_save_load", film_save_load_string);
	params.getParam("film_autosave_interval_type", film_autosave_interval_type_string);
	params.getParam("film_autosave_interval_passes", film_autosave_params.interval_passes_);
	params.getParam("film_autosave_interval_seconds", film_autosave_params.interval_seconds_);

	Y_DEBUG << "Images autosave: " << images_autosave_interval_type_string << ", " << images_autosave_params.interval_passes_ << ", " << images_autosave_params.interval_seconds_ << YENDL;

	Y_DEBUG << "ImageFilm autosave: " << film_save_load_string << ", " << film_autosave_interval_type_string << ", " << film_autosave_params.interval_passes_ << ", " << film_autosave_params.interval_seconds_ << YENDL;

	if(color_space_string == "sRGB") color_space = Srgb;
	else if(color_space_string == "XYZ") color_space = XyzD65;
	else if(color_space_string == "LinearRGB") color_space = LinearRgb;
	else if(color_space_string == "Raw_Manual_Gamma") color_space = RawManualGamma;
	else color_space = Srgb;

	if(color_space_string_2 == "sRGB") color_space_2 = Srgb;
	else if(color_space_string_2 == "XYZ") color_space_2 = XyzD65;
	else if(color_space_string_2 == "LinearRGB") color_space_2 = LinearRgb;
	else if(color_space_string_2 == "Raw_Manual_Gamma") color_space_2 = RawManualGamma;
	else color_space_2 = Srgb;

	if(images_autosave_interval_type_string == "pass-interval") images_autosave_params.interval_type_ = ImageFilm::ImageFilm::AutoSaveParams::IntervalType::Pass;
	else if(images_autosave_interval_type_string == "time-interval") images_autosave_params.interval_type_ = ImageFilm::AutoSaveParams::IntervalType::Time;
	else images_autosave_params.interval_type_ = ImageFilm::AutoSaveParams::IntervalType::None;

	if(film_save_load_string == "load-save") film_save_load = ImageFilm::FilmSaveLoad::LoadAndSave;
	else if(film_save_load_string == "save") film_save_load = ImageFilm::FilmSaveLoad::Save;
	else film_save_load = ImageFilm::FilmSaveLoad::None;

	if(film_autosave_interval_type_string == "pass-interval") film_autosave_params.interval_type_ = ImageFilm::AutoSaveParams::IntervalType::Pass;
	else if(film_autosave_interval_type_string == "time-interval") film_autosave_params.interval_type_ = ImageFilm::AutoSaveParams::IntervalType::Time;
	else film_autosave_params.interval_type_ = ImageFilm::AutoSaveParams::IntervalType::None;

	output.initTilesPasses(cameras_.size(), passes_settings_.extPassesSettings().size());

	ImageFilm::FilterType type = ImageFilm::FilterType::Box;
	if(name == "mitchell") type = ImageFilm::FilterType::Mitchell;
	else if(name == "gauss") type = ImageFilm::FilterType::Gauss;
	else if(name == "lanczos") type = ImageFilm::FilterType::Lanczos;
	else if(name != "box") Y_WARN_SCENE << "No AA filter defined defaulting to Box!" << YENDL;

	ImageSplitter::TilesOrderType tilesOrder = ImageSplitter::CentreRandom;
	if(tiles_order == "linear") tilesOrder = ImageSplitter::Linear;
	else if(tiles_order == "random") tilesOrder = ImageSplitter::Random;
	else if(tiles_order != "centre") Y_VERBOSE_SCENE << "Defaulting to Centre tiles order." << YENDL; // this is info imho not a warning

	ImageFilm *film = new ImageFilm(width, height, xstart, ystart, output, filt_sz, type, this, show_sampled_pixels, tile_size, tilesOrder, premult);

	if(color_space == RawManualGamma)
	{
		if(gamma > 0 && std::fabs(1.f - gamma) > 0.001) film->setColorSpace(color_space, gamma);
		else film->setColorSpace(LinearRgb, 1.f); //If the gamma is too close to 1.f, or negative, ignore gamma and do a pure linear RGB processing without gamma.
	}
	else film->setColorSpace(color_space, gamma);

	if(color_space_2 == RawManualGamma)
	{
		if(gamma_2 > 0 && std::fabs(1.f - gamma_2) > 0.001) film->setColorSpace2(color_space_2, gamma_2);
		else film->setColorSpace2(LinearRgb, 1.f); //If the gamma is too close to 1.f, or negative, ignore gamma and do a pure linear RGB processing without gamma.
	}
	else film->setColorSpace2(color_space_2, gamma_2);

	film->setPremult2(premult_2);

	film->setImagesAutoSaveParams(images_autosave_params);
	film->setFilmAutoSaveParams(film_autosave_params);
	film->setFilmSaveLoad(film_save_load);

	if(images_autosave_params.interval_type_ == ImageFilm::AutoSaveParams::IntervalType::Pass) Y_INFO_SCENE << "AutoSave partially rendered image every " << images_autosave_params.interval_passes_ << " passes" << YENDL;

	if(images_autosave_params.interval_type_ == ImageFilm::AutoSaveParams::IntervalType::Time) Y_INFO_SCENE << "AutoSave partially rendered image every " << images_autosave_params.interval_seconds_ << " seconds" << YENDL;

	if(film_save_load != ImageFilm::FilmSaveLoad::Save) Y_INFO_SCENE << "Enabling imageFilm file saving feature" << YENDL;
	if(film_save_load == ImageFilm::FilmSaveLoad::LoadAndSave) Y_INFO_SCENE << "Enabling imageFilm Loading feature. It will load and combine the ImageFilm files from the currently selected image output folder before start rendering, autodetecting each film format (binary/text) automatically. If they don't match exactly the scene, bad results could happen. Use WITH CARE!" << YENDL;

	if(film_autosave_params.interval_type_ == ImageFilm::AutoSaveParams::IntervalType::Pass) Y_INFO_SCENE << "AutoSave internal imageFilm every " << film_autosave_params.interval_passes_ << " passes" << YENDL;

	if(film_autosave_params.interval_type_ == ImageFilm::AutoSaveParams::IntervalType::Time) Y_INFO_SCENE << "AutoSave internal imageFilm image every " << film_autosave_params.interval_seconds_ << " seconds" << YENDL;

	return film;
}

VolumeHandler *Scene::createVolumeH(const std::string &name, const ParamMap &params)
{
	std::string pname = "VolumeHandler";
	if(volume_handlers_.find(name) != volume_handlers_.end())
	{
		WARN_EXIST; return nullptr;
	}
	std::string type;
	if(! params.getParam("type", type))
	{
		ERR_NO_TYPE; return nullptr;
	}
	VolumeHandler *volume = VolumeHandler::factory(params, *this);
	if(volume)
	{
		volume_handlers_[name] = volume;
		INFO_VERBOSE_SUCCESS(name, type);
		return volume;
	}
	ERR_ON_CREATE(type);
	return nullptr;
}

VolumeRegion *Scene::createVolumeRegion(const std::string &name, ParamMap &params)
{
	std::string pname = "VolumeRegion";
	if(volume_regions_.find(name) != volume_regions_.end())
	{
		WARN_EXIST; return nullptr;
	}
	std::string type;
	if(! params.getParam("type", type))
	{
		ERR_NO_TYPE; return nullptr;
	}
	VolumeRegion *volumeregion = VolumeRegion::factory(params, *this);
	if(volumeregion)
	{
		volume_regions_[name] = volumeregion;
		INFO_VERBOSE_SUCCESS(name, type);
		return volumeregion;
	}
	ERR_ON_CREATE(type);
	return nullptr;
}

void Scene::setupLoggingAndBadge(const ParamMap &params)
{
	bool logging_save_log = false;
	bool logging_save_html = false;
	bool logging_draw_render_settings = true;
	bool logging_draw_aa_noise_settings = true;
	std::string logging_params_badge_position;
	std::string logging_title;
	std::string logging_author;
	std::string logging_contact;
	std::string logging_comments;
	std::string logging_custom_icon;
	std::string logging_font_path;
	float logging_font_size_factor = 1.f;

	params.getParam("logging_paramsBadgePosition", logging_params_badge_position);
	params.getParam("logging_saveLog", logging_save_log);
	params.getParam("logging_saveHTML", logging_save_html);
	params.getParam("logging_drawRenderSettings", logging_draw_render_settings);
	params.getParam("logging_drawAANoiseSettings", logging_draw_aa_noise_settings);
	params.getParam("logging_author", logging_author);
	params.getParam("logging_title", logging_title);
	params.getParam("logging_contact", logging_contact);
	params.getParam("logging_comments", logging_comments);
	params.getParam("logging_customIcon", logging_custom_icon);
	params.getParam("logging_fontPath", logging_font_path);
	params.getParam("logging_fontSizeFactor", logging_font_size_factor);

	logger__.setSaveLog(logging_save_log);
	logger__.setSaveHtml(logging_save_html);
	logger__.setDrawRenderSettings(logging_draw_render_settings);
	logger__.setDrawAaNoiseSettings(logging_draw_aa_noise_settings);
	logger__.setParamsBadgePosition(logging_params_badge_position);
	logger__.setLoggingTitle(logging_title);
	logger__.setLoggingAuthor(logging_author);
	logger__.setLoggingContact(logging_contact);
	logger__.setLoggingComments(logging_comments);
	logger__.setLoggingCustomIcon(logging_custom_icon);
	logger__.setLoggingFontPath(logging_font_path);
	logger__.setLoggingFontSizeFactor(logging_font_size_factor);
}

/*! setup the scene for rendering (set camera, background, integrator, create image film,
	set antialiasing etc.)
	attention: since this function creates an image film and asigns it to the scene,
	you need to delete it before deleting the scene!
*/
bool Scene::setupScene(Scene &scene, const ParamMap &params, ColorOutput &output, ProgressBar *pb)
{
	std::string name;
	std::string aa_dark_detection_type_string = "none";
	AaNoiseParams aa_noise_params;
	int nthreads = -1, nthreads_photons = -1;
	bool adv_auto_shadow_bias_enabled = true;
	float adv_shadow_bias_value = YAF_SHADOW_BIAS;
	bool adv_auto_min_raydist_enabled = true;
	float adv_min_raydist_value = MIN_RAYDIST;
	int adv_base_sampling_offset = 0;
	int adv_computer_node = 0;

	bool background_resampling = true;  //If false, the background will not be resampled in subsequent adaptative AA passes

	if(! params.getParam("camera_name", name))
	{
		Y_ERROR_SCENE << "Specify a Camera!!" << YENDL;
		return false;
	}

	if(!params.getParam("integrator_name", name))
	{
		Y_ERROR_SCENE << "Specify an Integrator!!" << YENDL;
		return false;
	}

	Integrator *inte = this->getIntegrator(name);

	if(!inte)
	{
		Y_ERROR_SCENE << "Specify an _existing_ Integrator!!" << YENDL;
		return false;
	}

	if(inte->integratorType() != Integrator::Surface)
	{
		Y_ERROR_SCENE << "Integrator is no surface integrator!" << YENDL;
		return false;
	}

	if(! params.getParam("volintegrator_name", name))
	{
		Y_ERROR_SCENE << "Specify a Volume Integrator!" << YENDL;
		return false;
	}

	Integrator *vol_inte = this->getIntegrator(name);

	Background *backg = nullptr;
	if(params.getParam("background_name", name))
	{
		backg = this->getBackground(name);
		if(!backg) Y_ERROR_SCENE << "please specify an _existing_ Background!!" << YENDL;
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
	ImageFilm *film = createImageFilm(params, output);

	if(pb)
	{
		film->setProgressBar(pb);
		inte->setProgressBar(pb);
	}

	params.getParam("filter_type", name); // AA filter type

	std::stringstream aa_settings;
	aa_settings << "AA Settings (" << ((!name.empty()) ? name : "box") << "): Tile size=" << film->getTileSize();
	logger__.appendAaNoiseSettings(aa_settings.str());

	if(aa_dark_detection_type_string == "linear") aa_noise_params.dark_detection_type_ = AaNoiseParams::DarkDetectionType::Linear;
	else if(aa_dark_detection_type_string == "curve") aa_noise_params.dark_detection_type_ = AaNoiseParams::DarkDetectionType::Curve;
	else aa_noise_params.dark_detection_type_ = AaNoiseParams::DarkDetectionType::None;

	//setup scene and render.
	scene.setImageFilm(film);
	scene.setSurfIntegrator((SurfaceIntegrator *)inte);
	scene.setVolIntegrator((VolumeIntegrator *)vol_inte);
	scene.setAntialiasing(aa_noise_params);
	scene.setNumThreads(nthreads);
	scene.setNumThreadsPhotons(nthreads_photons);
	if(backg) scene.setBackground(backg);
	scene.shadow_bias_auto_ = adv_auto_shadow_bias_enabled;
	scene.shadow_bias_ = adv_shadow_bias_value;
	scene.ray_min_dist_auto_ = adv_auto_min_raydist_enabled;
	scene.ray_min_dist_ = adv_min_raydist_value;

	Y_DEBUG << "adv_base_sampling_offset=" << adv_base_sampling_offset << YENDL;
	film->setBaseSamplingOffset(adv_base_sampling_offset);
	film->setComputerNode(adv_computer_node);

	film->setBackgroundResampling(background_resampling);

	output.setPassesSettings(getPassesSettings());

	return true;
}

const std::vector<Light *> Scene::getLightsVisible() const
{
	std::vector<Light *> result;
	for(const auto &l : lights_)
	{
		if(l.second->lightEnabled() && !l.second->photonOnly()) result.push_back(l.second);
	}
	return result;
}

const std::vector<Light *> Scene::getLightsEmittingCausticPhotons() const
{
	std::vector<Light *> result;
	for(const auto &l : lights_)
	{
		if(l.second->lightEnabled() && l.second->shootsCausticP()) result.push_back(l.second);
	}
	return result;
}

const std::vector<Light *> Scene::getLightsEmittingDiffusePhotons() const
{
	std::vector<Light *> result;
	for(const auto &l : lights_)
	{
		if(l.second->lightEnabled() && l.second->shootsDiffuseP()) result.push_back(l.second);
	}
	return result;
}

bool Scene::passEnabled(const IntPassType &pass_type) const { return getPassesSettings()->intPassesSettings().enabled(pass_type); }

void Scene::setOutput2(ColorOutput *out_2) {
	output_2_ = out_2;
	output_2_->setPassesSettings(getPassesSettings());
}


END_YAFARAY
