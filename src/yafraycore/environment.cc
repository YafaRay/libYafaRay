/****************************************************************************
 *
 *      environment.cc: Yafray environment for plugin loading and
 *      object instatiation
 *      This is part of the libYafaRay package
 *      Copyright (C) 2005  Alejandro Conty Estévez, Mathias Wein
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
 *
 */

#include <yafray_config.h>
#include <core_api/environment.h>
#include <core_api/scene.h>
#include <core_api/session.h>
#include <core_api/params.h>
#include <core_api/dynamic_library.h>
#include <core_api/output.h>

#ifdef _WIN32
#include <io.h>
#include <windows.h>
#endif

#include <core_api/light.h>
#include <core_api/material.h>
#include <core_api/integrator.h>
#include <core_api/texture.h>
#include <core_api/background.h>
#include <core_api/camera.h>
#include <core_api/shader.h>
#include <core_api/imagefilm.h>
#include <core_api/imagehandler.h>
#include <core_api/object3d.h>
#include <core_api/volume.h>
#include <yafraycore/std_primitives.h>
#include <core_api/file.h>
#include <string>
#include <sstream>


BEGIN_YAFRAY
#define ENV_TAG << "Environment: "
#define Y_INFO_ENV Y_INFO ENV_TAG
#define Y_VERBOSE_ENV Y_VERBOSE ENV_TAG
#define Y_ERROR_ENV Y_ERROR ENV_TAG
#define Y_WARN_ENV Y_WARNING ENV_TAG

#define WARN_EXIST Y_WARN_ENV << "Sorry, " << pname << " \"" << name << "\" already exists!" << YENDL

#define ERR_NO_TYPE Y_ERROR_ENV << pname << " type not specified for \"" << name << "\" node!" << YENDL
#define ERR_UNK_TYPE(t) Y_ERROR_ENV << "Don't know how to create " << pname << " of type '" << t << "'!" << YENDL
#define ERR_ON_CREATE(t) Y_ERROR_ENV << "No " << pname << " was constructed by plugin '" << t << "'!" << YENDL

#define INFO_SUCCESS(name, t) Y_INFO_ENV << "Added " << pname << " '"<< name << "' (" << t << ")!" << YENDL
#define INFO_SUCCESS_DISABLED(name, t) Y_INFO_ENV << "Added " << pname << " '"<< name << "' (" << t << ")! [DISABLED]" << YENDL

#define SUCCESS_REG(t, name) Y_INFO_ENV << "Registered " << t << " type '" << name << "'" << YENDL

#define INFO_VERBOSE_SUCCESS(name, t) Y_VERBOSE_ENV << "Added " << pname << " '"<< name << "' (" << t << ")!" << YENDL
#define INFO_VERBOSE_SUCCESS_DISABLED(name, t) Y_VERBOSE_ENV << "Added " << pname << " '"<< name << "' (" << t << ")! [DISABLED]" << YENDL

#define SUCCESS_VERBOSE_REG(t, name) Y_VERBOSE_ENV << "Registered " << t << " type '" << name << "'" << YENDL

RenderEnvironment::RenderEnvironment()
{
	std::string compiler = YAFARAY_BUILD_COMPILER;
	if(!YAFARAY_BUILD_PLATFORM.empty()) compiler = YAFARAY_BUILD_PLATFORM + "-" + YAFARAY_BUILD_COMPILER;

	Y_INFO << PACKAGE << " (" << YAFARAY_BUILD_VERSION << ")" << " " << YAFARAY_BUILD_OS << " " << YAFARAY_BUILD_ARCHITECTURE << " (" << compiler << ")" << YENDL;
	object_factory_["sphere"] = sphereFactory__;
	output_2_ = nullptr;
	session__.setDifferentialRaysEnabled(false);	//By default, disable ray differential calculations. Only if at least one texture uses them, then enable differentials.

#ifndef HAVE_OPENCV
	Y_WARNING << PACKAGE << " built without OpenCV support. The following functionality will not work: image output denoise, background IBL blur, object/face edge render passes, toon render pass." << YENDL;
#endif
}

template <class T>
void freeMap__(std::map< std::string, T * > &map)
{
	for(auto i = map.begin(); i != map.end(); ++i) delete i->second;
}

RenderEnvironment::~RenderEnvironment()
{
	freeMap__(lights_);
	freeMap__(textures_);
	freeMap__(materials_);
	freeMap__(objects_);
	freeMap__(cameras_);
	freeMap__(backgrounds_);
	freeMap__(integrators_);
	freeMap__(volumes_);
	freeMap__(volumeregions_);
}

void RenderEnvironment::clearAll()
{
	freeMap__(lights_);
	freeMap__(textures_);
	freeMap__(materials_);
	freeMap__(objects_);
	freeMap__(cameras_);
	freeMap__(backgrounds_);
	freeMap__(integrators_);
	freeMap__(volumes_);
	freeMap__(volumeregions_);
	freeMap__(imagehandlers_);

	lights_.clear();
	textures_.clear();
	materials_.clear();
	objects_.clear();
	cameras_.clear();
	backgrounds_.clear();
	integrators_.clear();
	volumes_.clear();
	volumeregions_.clear();
	imagehandlers_.clear();
}

void RenderEnvironment::loadPlugins(const std::string &path)
{
	typedef void (Reg_t)(RenderEnvironment &);
	Y_INFO_ENV << "Loading plugins ..." << YENDL;
	std::vector<std::string> plugins = File::listFiles(path);

	for(auto i = plugins.begin(); i != plugins.end(); ++i)
	{
		DynamicLoadedLibrary plug((path + "//" + *i).c_str());
		if(!plug.isOpen()) continue;
		Reg_t *register_plugin;
		register_plugin = (Reg_t *)plug.getSymbol("registerPlugin__");
		if(register_plugin == nullptr) continue;
		register_plugin(*this);
		plugin_handlers_.push_back(plug);
	}
}

bool RenderEnvironment::getPluginPath(std::string &path)
{
	// Steps to find the plugins path.

	// First check if the plugins path has been manually set and if it exists:
	if(!path.empty())
	{
		if(File::exists(path, false))
		{
			Y_VERBOSE_ENV << "Plugins path found: '" << path << "'" << YENDL;
			return true;
		}
		else
		{
			Y_VERBOSE_ENV << "Plugins path NOT found in '" << path << "'" << YENDL;
		}
	}

	// If the previous check does not work, check if the plugins path is in a subfolder of the currently executed file. This only works if the executable is executed with the full path, as this will not search for the executable in the search paths.
	path = session__.getPathYafaRayXml() + "/yafaray-plugins/";
	if(File::exists(path, false))
	{
		Y_VERBOSE_ENV << "Plugins path found: '" << path << "'" << YENDL;
		return true;
	}
	else
	{
		Y_VERBOSE_ENV << "Plugins path NOT found in '" << path << "'" << YENDL;
	}

	// If the previous check does not work, check if the plugins path is in a "lib" subfolder of the parent of the currently executed file. This only works if the executable is executed with the full path, as this will not search for the executable in the search paths.
	path = session__.getPathYafaRayXml() + "/../lib/yafaray-plugins/";
	if(File::exists(path, false))
	{
		Y_VERBOSE_ENV << "Plugins path found: '" << path << "'" << YENDL;
		return true;
	}
	else
	{
		Y_VERBOSE_ENV << "Plugins path NOT found in '" << path << "'" << YENDL;
	}

	// If the previous checks do not work, check if the plugins path is in the plugins search directory defined in CMake during the building process
	path = std::string(YAFARAY_BUILD_SEARCH_PLUGIN_DIR);
	if(File::exists(path, false))
	{
		Y_VERBOSE_ENV << "Plugins path found: '" << path << "'" << YENDL;
		return true;
	}
	else
	{
		Y_VERBOSE_ENV << "Plugins path NOT found in '" << path << "'" << YENDL;
	}

	return false;
}


Material *RenderEnvironment::getMaterial(const std::string &name) const
{
	auto i = materials_.find(name);
	if(i != materials_.end()) return i->second;
	else return nullptr;
}

Texture *RenderEnvironment::getTexture(const std::string &name) const
{
	auto i = textures_.find(name);
	if(i != textures_.end()) return i->second;
	else return nullptr;
}

Camera *RenderEnvironment::getCamera(const std::string &name) const
{
	auto i = cameras_.find(name);
	if(i != cameras_.end()) return i->second;
	else return nullptr;
}

Background *RenderEnvironment::getBackground(const std::string &name) const
{
	auto i = backgrounds_.find(name);
	if(i != backgrounds_.end()) return i->second;
	else return nullptr;
}

Integrator *RenderEnvironment::getIntegrator(const std::string &name) const
{
	auto i = integrators_.find(name);
	if(i != integrators_.end()) return i->second;
	else return nullptr;
}

ShaderNode *RenderEnvironment::getShaderNode(const std::string &name) const
{
	auto i = shaders_.find(name);
	if(i != shaders_.end()) return i->second;
	else return nullptr;
}

Light *RenderEnvironment::createLight(const std::string &name, ParamMap &params)
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
	Light *light;
	auto i = light_factory_.find(type);
	if(i != light_factory_.end()) light = i->second(params, *this);
	else
	{
		ERR_UNK_TYPE(type); return nullptr;
	}
	if(light)
	{
		lights_[name] = light;

		if(light->lightEnabled()) INFO_VERBOSE_SUCCESS(name, type);
		else INFO_VERBOSE_SUCCESS_DISABLED(name, type);

		return light;
	}
	ERR_ON_CREATE(type);
	return nullptr;
}

Texture *RenderEnvironment::createTexture(const std::string &name, ParamMap &params)
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
	Texture *texture;
	auto i = texture_factory_.find(type);
	if(i != texture_factory_.end()) texture = i->second(params, *this);
	else
	{
		ERR_UNK_TYPE(type); return nullptr;
	}
	if(texture)
	{
		textures_[name] = texture;
		INFO_VERBOSE_SUCCESS(name, type);
		return texture;
	}
	ERR_ON_CREATE(type);
	return nullptr;
}

ShaderNode *RenderEnvironment::createShaderNode(const std::string &name, ParamMap &params)
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
	ShaderNode *shader;
	auto i = shader_factory_.find(type);
	if(i != shader_factory_.end()) shader = i->second(params, *this);
	else
	{
		ERR_UNK_TYPE(type); return nullptr;
	}
	if(shader)
	{
		shaders_[name] = shader;
		INFO_VERBOSE_SUCCESS(name, type);
		return shader;
	}
	ERR_ON_CREATE(type);
	return nullptr;
}

Material *RenderEnvironment::createMaterial(const std::string &name, ParamMap &params, std::list<ParamMap> &eparams)
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
	Material *material;
	auto i = material_factory_.find(type);
	if(i != material_factory_.end()) material = i->second(params, eparams, *this);
	else
	{
		ERR_UNK_TYPE(type); return nullptr;
	}
	if(material)
	{
		materials_[name] = material;
		INFO_VERBOSE_SUCCESS(name, type);
		return material;
	}
	ERR_ON_CREATE(type);
	return nullptr;
}

Background *RenderEnvironment::createBackground(const std::string &name, ParamMap &params)
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
	Background *background;
	auto i = background_factory_.find(type);
	if(i != background_factory_.end()) background = i->second(params, *this);
	else
	{
		ERR_UNK_TYPE(type); return nullptr;
	}
	if(background)
	{
		backgrounds_[name] = background;
		INFO_VERBOSE_SUCCESS(name, type);
		return background;
	}
	ERR_ON_CREATE(type);
	return nullptr;
}

ImageHandler *RenderEnvironment::createImageHandler(const std::string &name, ParamMap &params, bool add_to_table)
{
	std::string pname = "ImageHandler";
	std::stringstream newname;
	int sufix_count = 0;

	newname << name;

	if(add_to_table)
	{
		while(true)
		{
			if(imagehandlers_.find(newname.str()) != imagehandlers_.end())
			{
				newname.seekg(0, std::ios::beg);
				newname << name << ".";
				newname.width(3);
				newname.fill('0');
				newname.flags(std::ios::right);
				newname << sufix_count;
				sufix_count++;
			}
			else break;
		}
	}

	std::string type;

	if(! params.getParam("type", type))
	{
		ERR_NO_TYPE; return nullptr;
	}

	ImageHandler *ih = nullptr;
	auto i = imagehandler_factory_.find(type);

	if(i != imagehandler_factory_.end())
	{
		ih = i->second(params, *this);
	}
	else
	{
		ERR_UNK_TYPE(type); return nullptr;
	}

	if(ih)
	{
		if(add_to_table) imagehandlers_[newname.str()] = ih;

		INFO_VERBOSE_SUCCESS(newname.str(), type);

		return ih;
	}

	ERR_ON_CREATE(type);

	return nullptr;
}

Object3D *RenderEnvironment::createObject(const std::string &name, ParamMap &params)
{
	std::string pname = "Object";
	if(objects_.find(name) != objects_.end())
	{
		WARN_EXIST; return nullptr;
	}
	std::string type;
	if(! params.getParam("type", type))
	{
		ERR_NO_TYPE; return nullptr;
	}
	Object3D *object;
	auto i = object_factory_.find(type);
	if(i != object_factory_.end()) object = i->second(params, *this);
	else
	{
		ERR_UNK_TYPE(type); return nullptr;
	}
	if(object)
	{
		objects_[name] = object;
		INFO_VERBOSE_SUCCESS(name, type);
		return object;
	}
	ERR_ON_CREATE(type);
	return nullptr;
}

Camera *RenderEnvironment::createCamera(const std::string &name, ParamMap &params)
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
	Camera *camera;
	auto i = camera_factory_.find(type);
	if(i != camera_factory_.end()) camera = i->second(params, *this);
	else
	{
		ERR_UNK_TYPE(type); return nullptr;
	}
	if(camera)
	{
		cameras_[name] = camera;
		INFO_VERBOSE_SUCCESS(name, type);
		int view_number = render_passes_.view_names_.size();
		camera->setCameraName(name);
		render_passes_.view_names_.push_back(camera->getViewName());

		Y_INFO << "Environment: View number=" << view_number << ", view name: '" << render_passes_.view_names_[view_number] << "', camera name: '" << camera->getCameraName() << "'" << YENDL;

		return camera;
	}
	ERR_ON_CREATE(type);
	return nullptr;
}

Integrator *RenderEnvironment::createIntegrator(const std::string &name, ParamMap &params)
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
	Integrator *integrator;
	auto i = integrator_factory_.find(type);
	if(i != integrator_factory_.end()) integrator = i->second(params, *this);
	else
	{
		ERR_UNK_TYPE(type); return nullptr;
	}
	if(integrator)
	{
		integrators_[name] = integrator;
		INFO_VERBOSE_SUCCESS(name, type);
		if(type == "bidirectional") Y_WARNING << "The Bidirectional integrator is UNSTABLE at the moment and needs to be improved. It might give unexpected and perhaps even incorrect render results. Use at your own risk." << YENDL;
		return integrator;
	}
	ERR_ON_CREATE(type);
	return nullptr;
}

void RenderEnvironment::setupRenderPasses(const ParamMap &params)
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

	//Adding the render passes and associating them to the internal YafaRay pass defined in the Blender Exporter "pass_xxx" parameters.
	for(auto it = render_passes_.ext_pass_map_int_string_.begin(); it != render_passes_.ext_pass_map_int_string_.end(); ++it)
	{
		internal_pass = "";
		external_pass = it->second;
		params.getParam("pass_" + external_pass, internal_pass);
		if(internal_pass != "disabled" && internal_pass != "") render_passes_.extPassAdd(external_pass, internal_pass);
	}

	//Generate any necessary auxiliar render passes
	render_passes_.auxPassesGenerate();

	render_passes_.setPassMaskObjIndex((float) pass_mask_obj_index);
	render_passes_.setPassMaskMatIndex((float) pass_mask_mat_index);
	render_passes_.setPassMaskInvert(pass_mask_invert);
	render_passes_.setPassMaskOnly(pass_mask_only);

	render_passes_.object_edge_thickness_ = object_edge_thickness;
	render_passes_.object_edge_threshold_ = object_edge_threshold;
	render_passes_.object_edge_smoothness_ = object_edge_smoothness;
	render_passes_.toon_pre_smooth_ = toon_pre_smooth;
	render_passes_.toon_quantization_ = toon_quantization;
	render_passes_.toon_post_smooth_ = toon_post_smooth;
	render_passes_.faces_edge_thickness_ = faces_edge_thickness;
	render_passes_.faces_edge_threshold_ = faces_edge_threshold;
	render_passes_.faces_edge_smoothness_ = faces_edge_smoothness;

	render_passes_.toon_edge_color_[0] = toon_edge_color.r_;
	render_passes_.toon_edge_color_[1] = toon_edge_color.g_;
	render_passes_.toon_edge_color_[2] = toon_edge_color.b_;
}

ImageFilm *RenderEnvironment::createImageFilm(const ParamMap &params, ColorOutput &output)
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
	AutoSaveIntervalType images_autosave_interval_type = AutoSaveIntervalType::None;
	int images_autosave_interval_passes = 1;
	double images_autosave_interval_seconds = 300.0;
	std::string film_save_load_string = "none";
	FilmFileSaveLoad film_save_load = FilmFileSaveLoad::None;
	std::string film_autosave_interval_type_string = "none";
	AutoSaveIntervalType film_autosave_interval_type = AutoSaveIntervalType::None;
	int film_autosave_interval_passes = 1;
	double film_autosave_interval_seconds = 300.0;

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
	params.getParam("images_autosave_interval_passes", images_autosave_interval_passes);
	params.getParam("images_autosave_interval_seconds", images_autosave_interval_seconds);
	params.getParam("film_save_load", film_save_load_string);
	params.getParam("film_autosave_interval_type", film_autosave_interval_type_string);
	params.getParam("film_autosave_interval_passes", film_autosave_interval_passes);
	params.getParam("film_autosave_interval_seconds", film_autosave_interval_seconds);

	Y_DEBUG << "Images autosave: " << images_autosave_interval_type_string << ", " << images_autosave_interval_passes << ", " << images_autosave_interval_seconds << YENDL;

	Y_DEBUG << "ImageFilm autosave: " << film_save_load_string << ", " << film_autosave_interval_type_string << ", " << film_autosave_interval_passes << ", " << film_autosave_interval_seconds << YENDL;

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

	if(images_autosave_interval_type_string == "pass-interval") images_autosave_interval_type = AutoSaveIntervalType::Pass;
	else if(images_autosave_interval_type_string == "time-interval") images_autosave_interval_type = AutoSaveIntervalType::Time;
	else images_autosave_interval_type = AutoSaveIntervalType::None;

	if(film_save_load_string == "load-save") film_save_load = FilmFileSaveLoad::LoadAndSave;
	else if(film_save_load_string == "save") film_save_load = FilmFileSaveLoad::Save;
	else film_save_load = FilmFileSaveLoad::None;

	if(film_autosave_interval_type_string == "pass-interval") film_autosave_interval_type = AutoSaveIntervalType::Pass;
	else if(film_autosave_interval_type_string == "time-interval") film_autosave_interval_type = AutoSaveIntervalType::Time;
	else film_autosave_interval_type = AutoSaveIntervalType::None;

	output.initTilesPasses(cameras_.size(), render_passes_.extPassesSize());

	ImageFilm::FilterType type = ImageFilm::FilterType::Box;
	if(name == "mitchell") type = ImageFilm::FilterType::Mitchell;
	else if(name == "gauss") type = ImageFilm::FilterType::Gauss;
	else if(name == "lanczos") type = ImageFilm::FilterType::Lanczos;
	else if(name != "box") Y_WARN_ENV << "No AA filter defined defaulting to Box!" << YENDL;

	ImageSplitter::TilesOrderType tilesOrder = ImageSplitter::CentreRandom;
	if(tiles_order == "linear") tilesOrder = ImageSplitter::Linear;
	else if(tiles_order == "random") tilesOrder = ImageSplitter::Random;
	else if(tiles_order != "centre") Y_VERBOSE_ENV << "Defaulting to Centre tiles order." << YENDL; // this is info imho not a warning

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

	film->setImagesAutoSaveIntervalType(images_autosave_interval_type);
	film->setImagesAutoSaveIntervalSeconds(images_autosave_interval_seconds);
	film->setImagesAutoSaveIntervalPasses(images_autosave_interval_passes);

	film->setFilmFileSaveLoad(film_save_load);
	film->setFilmAutoSaveIntervalType(film_autosave_interval_type);
	film->setFilmAutoSaveIntervalSeconds(film_autosave_interval_seconds);
	film->setFilmAutoSaveIntervalPasses(film_autosave_interval_passes);

	if(images_autosave_interval_type == AutoSaveIntervalType::Pass) Y_INFO_ENV << "AutoSave partially rendered image every " << images_autosave_interval_passes << " passes" << YENDL;

	if(images_autosave_interval_type == AutoSaveIntervalType::Time) Y_INFO_ENV << "AutoSave partially rendered image every " << images_autosave_interval_seconds << " seconds" << YENDL;

	if(film_save_load != FilmFileSaveLoad::Save) Y_INFO_ENV << "Enabling imageFilm file saving feature" << YENDL;
	if(film_save_load == FilmFileSaveLoad::LoadAndSave) Y_INFO_ENV << "Enabling imageFilm Loading feature. It will load and combine the ImageFilm files from the currently selected image output folder before start rendering, autodetecting each film format (binary/text) automatically. If they don't match exactly the scene, bad results could happen. Use WITH CARE!" << YENDL;

	if(film_autosave_interval_type == AutoSaveIntervalType::Pass) Y_INFO_ENV << "AutoSave internal imageFilm every " << film_autosave_interval_passes << " passes" << YENDL;

	if(film_autosave_interval_type == AutoSaveIntervalType::Time) Y_INFO_ENV << "AutoSave internal imageFilm image every " << film_autosave_interval_seconds << " seconds" << YENDL;

	return film;
}

VolumeHandler *RenderEnvironment::createVolumeH(const std::string &name, const ParamMap &params)
{
	std::string pname = "VolumeHandler";
	if(volumes_.find(name) != volumes_.end())
	{
		WARN_EXIST; return nullptr;
	}
	std::string type;
	if(! params.getParam("type", type))
	{
		ERR_NO_TYPE; return nullptr;
	}
	VolumeHandler *volume;
	auto i = volume_factory_.find(type);
	if(i != volume_factory_.end()) volume = i->second(params, *this);
	else
	{
		ERR_UNK_TYPE(type); return nullptr;
	}
	if(volume)
	{
		volumes_[name] = volume;
		INFO_VERBOSE_SUCCESS(name, type);
		return volume;
	}
	ERR_ON_CREATE(type);
	return nullptr;
}

VolumeRegion *RenderEnvironment::createVolumeRegion(const std::string &name, ParamMap &params)
{
	std::string pname = "VolumeRegion";
	if(volumeregions_.find(name) != volumeregions_.end())
	{
		WARN_EXIST; return nullptr;
	}
	std::string type;
	if(! params.getParam("type", type))
	{
		ERR_NO_TYPE; return nullptr;
	}
	VolumeRegion *volumeregion;
	auto i = volumeregion_factory_.find(type);
	if(i != volumeregion_factory_.end()) volumeregion = i->second(params, *this);
	else
	{
		ERR_UNK_TYPE(type); return nullptr;
	}
	if(volumeregion)
	{
		volumeregions_[name] = volumeregion;
		INFO_VERBOSE_SUCCESS(name, type);
		return volumeregion;
	}
	ERR_ON_CREATE(type);
	return nullptr;
}

void RenderEnvironment::setupLoggingAndBadge(const ParamMap &params)
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
bool RenderEnvironment::setupScene(Scene &scene, const ParamMap &params, ColorOutput &output, ProgressBar *pb)
{
	std::string name;
	int aa_passes = 1, aa_samples = 1, aa_inc_samples = 1, nthreads = -1, nthreads_photons = -1;
	double aa_threshold = 0.05;
	float aa_resampled_floor = 0.f;
	float aa_sample_multiplier_factor = 1.f;
	float aa_light_sample_multiplier_factor = 1.f;
	float aa_indirect_sample_multiplier_factor = 1.f;
	bool aa_detect_color_noise = false;
	std::string aa_dark_detection_type_string = "none";
	DarkDetectionType aa_dark_detection_type = DarkDetectionType::None;
	float aa_dark_threshold_factor = 0.f;
	int aa_variance_edge_size = 10;
	int aa_variance_pixels = 0;
	float aa_clamp_samples = 0.f;
	float aa_clamp_indirect = 0.f;

	bool adv_auto_shadow_bias_enabled = true;
	float adv_shadow_bias_value = YAF_SHADOW_BIAS;
	bool adv_auto_min_raydist_enabled = true;
	float adv_min_raydist_value = MIN_RAYDIST;
	int adv_base_sampling_offset = 0;
	int adv_computer_node = 0;

	bool background_resampling = true;  //If false, the background will not be resampled in subsequent adaptative AA passes

	if(! params.getParam("camera_name", name))
	{
		Y_ERROR_ENV << "Specify a Camera!!" << YENDL;
		return false;
	}

	if(!params.getParam("integrator_name", name))
	{
		Y_ERROR_ENV << "Specify an Integrator!!" << YENDL;
		return false;
	}

	Integrator *inte = this->getIntegrator(name);

	if(!inte)
	{
		Y_ERROR_ENV << "Specify an _existing_ Integrator!!" << YENDL;
		return false;
	}

	if(inte->integratorType() != Integrator::Surface)
	{
		Y_ERROR_ENV << "Integrator is no surface integrator!" << YENDL;
		return false;
	}

	if(! params.getParam("volintegrator_name", name))
	{
		Y_ERROR_ENV << "Specify a Volume Integrator!" << YENDL;
		return false;
	}

	Integrator *vol_inte = this->getIntegrator(name);

	Background *backg = nullptr;
	if(params.getParam("background_name", name))
	{
		backg = this->getBackground(name);
		if(!backg) Y_ERROR_ENV << "please specify an _existing_ Background!!" << YENDL;
	}

	params.getParam("AA_passes", aa_passes);
	params.getParam("AA_minsamples", aa_samples);
	aa_inc_samples = aa_samples;
	params.getParam("AA_inc_samples", aa_inc_samples);
	params.getParam("AA_threshold", aa_threshold);
	params.getParam("AA_resampled_floor", aa_resampled_floor);
	params.getParam("AA_sample_multiplier_factor", aa_sample_multiplier_factor);
	params.getParam("AA_light_sample_multiplier_factor", aa_light_sample_multiplier_factor);
	params.getParam("AA_indirect_sample_multiplier_factor", aa_indirect_sample_multiplier_factor);
	params.getParam("AA_detect_color_noise", aa_detect_color_noise);
	params.getParam("AA_dark_detection_type", aa_dark_detection_type_string);
	params.getParam("AA_dark_threshold_factor", aa_dark_threshold_factor);
	params.getParam("AA_variance_edge_size", aa_variance_edge_size);
	params.getParam("AA_variance_pixels", aa_variance_pixels);
	params.getParam("AA_clamp_samples", aa_clamp_samples);
	params.getParam("AA_clamp_indirect", aa_clamp_indirect);
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

	if(aa_dark_detection_type_string == "linear") aa_dark_detection_type = DarkDetectionType::Linear;
	else if(aa_dark_detection_type_string == "curve") aa_dark_detection_type = DarkDetectionType::Curve;
	else aa_dark_detection_type = DarkDetectionType::None;

	//setup scene and render.
	scene.setImageFilm(film);
	scene.setSurfIntegrator((SurfaceIntegrator *)inte);
	scene.setVolIntegrator((VolumeIntegrator *)vol_inte);
	scene.setAntialiasing(aa_samples, aa_passes, aa_inc_samples, aa_threshold, aa_resampled_floor, aa_sample_multiplier_factor, aa_light_sample_multiplier_factor, aa_indirect_sample_multiplier_factor, aa_detect_color_noise, aa_dark_detection_type, aa_dark_threshold_factor, aa_variance_edge_size, aa_variance_pixels, aa_clamp_samples, aa_clamp_indirect);
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

	return true;
}

void RenderEnvironment::registerFactory(const std::string &name, LightFactory_t *f)
{
	light_factory_[name] = f;
	SUCCESS_VERBOSE_REG("Light", name);
}

void RenderEnvironment::registerFactory(const std::string &name, MaterialFactory_t *f)
{
	material_factory_[name] = f;
	SUCCESS_VERBOSE_REG("Material", name);
}

void RenderEnvironment::registerFactory(const std::string &name, TextureFactory_t *f)
{
	texture_factory_[name] = f;
	SUCCESS_VERBOSE_REG("Texture", name);
}

void RenderEnvironment::registerFactory(const std::string &name, ShaderFactory_t *f)
{
	shader_factory_[name] = f;
	SUCCESS_VERBOSE_REG("ShaderNode", name);
}

void RenderEnvironment::registerFactory(const std::string &name, ObjectFactory_t *f)
{
	object_factory_[name] = f;
	SUCCESS_VERBOSE_REG("Object", name);
}

void RenderEnvironment::registerFactory(const std::string &name, CameraFactory_t *f)
{
	camera_factory_[name] = f;
	SUCCESS_VERBOSE_REG("Camera", name);
}

void RenderEnvironment::registerFactory(const std::string &name, BackgroundFactory_t *f)
{
	background_factory_[name] = f;
	SUCCESS_VERBOSE_REG("Background", name);
}

void RenderEnvironment::registerFactory(const std::string &name, IntegratorFactory_t *f)
{
	integrator_factory_[name] = f;
	SUCCESS_VERBOSE_REG("Integrator", name);
}

void RenderEnvironment::registerFactory(const std::string &name, VolumeFactory_t *f)
{
	volume_factory_[name] = f;
	SUCCESS_VERBOSE_REG("VolumetricHandler", name);
}

void RenderEnvironment::registerFactory(const std::string &name, VolumeregionFactory_t *f)
{
	volumeregion_factory_[name] = f;
	SUCCESS_VERBOSE_REG("VolumeRegion", name);
}

void RenderEnvironment::registerImageHandler(const std::string &name, const std::string &valid_extensions, const std::string &full_name, ImagehandlerFactory_t *f)
{
	imagehandler_factory_[name] = f;
	imagehandlers_fullnames_[name] = full_name;
	imagehandlers_extensions_[name] = valid_extensions;
	SUCCESS_VERBOSE_REG("ImageHandler", name);
}

std::vector<std::string> RenderEnvironment::listImageHandlers()
{
	std::vector<std::string> ret;
	if(imagehandlers_fullnames_.size() > 0)
	{
		for(auto i = imagehandlers_fullnames_.begin(); i != imagehandlers_fullnames_.end(); ++i)
		{
			ret.push_back(i->first);
		}
	}
	else Y_ERROR_ENV << "There is no image handlers registrered" << YENDL;

	return ret;
}

std::vector<std::string> RenderEnvironment::listImageHandlersFullName()
{
	std::vector<std::string> ret;
	if(imagehandlers_fullnames_.size() > 0)
	{
		for(auto i = imagehandlers_fullnames_.begin(); i != imagehandlers_fullnames_.end(); ++i)
		{
			ret.push_back(i->second);
		}
	}
	else Y_ERROR_ENV << "There is no image handlers registrered" << YENDL;

	return ret;
}

std::string RenderEnvironment::getImageFormatFromFullName(const std::string &fullname)
{
	std::string ret;
	if(imagehandlers_fullnames_.size() > 0)
	{
		for(auto i = imagehandlers_fullnames_.begin(); i != imagehandlers_fullnames_.end(); ++i)
		{
			if(i->second == fullname) ret = i->first;
		}
	}
	else Y_ERROR_ENV << "There is no image handlers registrered" << YENDL;

	return ret;
}

std::string RenderEnvironment::getImageFormatFromExtension(const std::string &ext)
{
	std::string ret = "";

	if(ext == "" || ext == " ") return ret;

	if(imagehandlers_extensions_.size() > 0)
	{
		for(auto i = imagehandlers_extensions_.begin(); i != imagehandlers_extensions_.end(); ++i)
		{
			if(i->second.find(ext) != std::string::npos) ret = i->first;
		}
	}
	else Y_ERROR_ENV << "There is no image handlers registrered" << YENDL;

	return ret;
}

std::string RenderEnvironment::getImageFullNameFromFormat(const std::string &format)
{
	std::string ret;
	if(imagehandlers_fullnames_.size() > 0)
	{
		for(auto i = imagehandlers_fullnames_.begin(); i != imagehandlers_fullnames_.end(); ++i)
		{
			if(i->first == format) ret = i->second;
		}
	}
	else Y_ERROR_ENV << "There is no image handlers registrered" << YENDL;

	return ret;
}

RenderEnvironment::ShaderFactory_t *RenderEnvironment::getShaderNodeFactory(const std::string &name) const
{
	auto i = shader_factory_.find(name);
	if(i != shader_factory_.end()) return i->second;
	Y_ERROR_ENV << "There is no factory for '" << name << "'\n";
	return nullptr;
}

END_YAFRAY
