/****************************************************************************
 *
 *      environment.cc: Yafray environment for plugin loading and
 *      object instatiation
 *      This is part of the yafray package
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

#include <core_api/environment.h>

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
#include <string>
#include <sstream>

__BEGIN_YAFRAY
#define ENV_TAG << "Environment: "
#define Y_INFO_ENV Y_INFO ENV_TAG
#define Y_VERBOSE_ENV Y_VERBOSE ENV_TAG
#define Y_ERROR_ENV Y_ERROR ENV_TAG
#define Y_WARN_ENV Y_WARNING ENV_TAG

#define WarnExist Y_WARN_ENV << "Sorry, " << pname << " \"" << name << "\" already exists!" << yendl

#define ErrNoType Y_ERROR_ENV << pname << " type not specified for \"" << name << "\" node!" << yendl
#define ErrUnkType(t) Y_ERROR_ENV << "Don't know how to create " << pname << " of type '" << t << "'!" << yendl
#define ErrOnCreate(t) Y_ERROR_ENV << "No " << pname << " was constructed by plugin '" << t << "'!" << yendl

#define InfoSuccess(name, t) Y_INFO_ENV << "Added " << pname << " '"<< name << "' (" << t << ")!" << yendl
#define InfoSuccessDisabled(name, t) Y_INFO_ENV << "Added " << pname << " '"<< name << "' (" << t << ")! [DISABLED]" << yendl

#define SuccessReg(t, name) Y_INFO_ENV << "Registered " << t << " type '" << name << "'" << yendl

#define InfoVerboseSuccess(name, t) Y_VERBOSE_ENV << "Added " << pname << " '"<< name << "' (" << t << ")!" << yendl
#define InfoVerboseSuccessDisabled(name, t) Y_VERBOSE_ENV << "Added " << pname << " '"<< name << "' (" << t << ")! [DISABLED]" << yendl

#define SuccessVerboseReg(t, name) Y_VERBOSE_ENV << "Registered " << t << " type '" << name << "'" << yendl

renderEnvironment_t::renderEnvironment_t()
{	
	Y_INFO << PACKAGE << " Core (" << session.getYafaRayCoreVersion() << ")" << " " << sysInfoGetOS() << sysInfoGetArchitecture() << sysInfoGetPlatform() << sysInfoGetCompiler() << yendl;
	object_factory["sphere"] = sphere_factory;
	output2 = nullptr;
}

template <class T>
void freeMap(std::map< std::string, T* > &map)
{
	for(auto i=map.begin(); i!=map.end(); ++i) delete i->second;
}

renderEnvironment_t::~renderEnvironment_t()
{
	freeMap(light_table);
	freeMap(texture_table);
	freeMap(material_table);
	freeMap(object_table);
	freeMap(camera_table);
	freeMap(background_table);
	freeMap(integrator_table);
	freeMap(volume_table);
	freeMap(volumeregion_table);
}

void renderEnvironment_t::clearAll()
{
	freeMap(light_table);
	freeMap(texture_table);
	freeMap(material_table);
	freeMap(object_table);
	freeMap(camera_table);
	freeMap(background_table);
	freeMap(integrator_table);
	freeMap(volume_table);
	freeMap(volumeregion_table);
	freeMap(imagehandler_table);

	light_table.clear();
	texture_table.clear();
	material_table.clear();
	object_table.clear();
	camera_table.clear();
	background_table.clear();
	integrator_table.clear();
	volume_table.clear();
	volumeregion_table.clear();
	imagehandler_table.clear();
}

void renderEnvironment_t::loadPlugins(const std::string &path)
{
	typedef void (reg_t)(renderEnvironment_t &);
	Y_INFO_ENV << "Loading plugins ..." << yendl;
	std::list<std::string> plugins=listDir(path);

	for(auto i=plugins.begin();i!=plugins.end();++i)
	{
		sharedlibrary_t plug(i->c_str());
		if(!plug.isOpen()) continue;
		reg_t *registerPlugin;
		registerPlugin=(reg_t *)plug.getSymbol("registerPlugin");
		if(registerPlugin==nullptr) continue;
		registerPlugin(*this);
		pluginHandlers.push_back(plug);
	}
}

bool renderEnvironment_t::getPluginPath(std::string &path)
{
	//Get plugin path from a subfolder of the current yafaray_xml executable file path
	if(!session.getPathYafaRayXml().empty())
	{
		path = session.getPathYafaRayXml()+"/plugins/";
		return true;
	}
	else return false;
}


material_t* renderEnvironment_t::getMaterial(const std::string &name)const
{
	auto i=material_table.find(name);
	if(i!=material_table.end()) return i->second;
	else return nullptr;
}

texture_t* renderEnvironment_t::getTexture(const std::string &name)const
{
	auto i=texture_table.find(name);
	if(i!=texture_table.end()) return i->second;
	else return nullptr;
}

camera_t* renderEnvironment_t::getCamera(const std::string &name)const
{
	auto i=camera_table.find(name);
	if(i!=camera_table.end()) return i->second;
	else return nullptr;
}

background_t* renderEnvironment_t::getBackground(const std::string &name)const
{
	auto i=background_table.find(name);
	if(i!=background_table.end()) return i->second;
	else return nullptr;
}

integrator_t* renderEnvironment_t::getIntegrator(const std::string &name)const
{
	auto i=integrator_table.find(name);
	if(i!=integrator_table.end()) return i->second;
	else return nullptr;
}

shaderNode_t* renderEnvironment_t::getShaderNode(const std::string &name)const
{
	auto i=shader_table.find(name);
	if(i!=shader_table.end()) return i->second;
	else return nullptr;
}

light_t* renderEnvironment_t::createLight(const std::string &name, paraMap_t &params)
{
	std::string pname = "Light";
	if(light_table.find(name) != light_table.end() )
	{
		WarnExist; return nullptr;
	}
	std::string type;
	if(! params.getParam("type", type) )
	{
		ErrNoType; return nullptr;
	}
	light_t* light;
	auto i=light_factory.find(type);
	if(i!=light_factory.end()) light = i->second(params,*this);
	else
	{
		ErrUnkType(type); return nullptr;
	}
	if(light)
	{
		light_table[name] = light;
		
		if(light->lightEnabled()) InfoVerboseSuccess(name, type);
		else InfoVerboseSuccessDisabled(name, type);
		
		return light;
	}
	ErrOnCreate(type);
	return nullptr;
}

texture_t* renderEnvironment_t::createTexture(const std::string &name, paraMap_t &params)
{
	std::string pname = "Texture";
	if(texture_table.find(name) != texture_table.end() )
	{
		WarnExist; return nullptr;
	}
	std::string type;
	if(! params.getParam("type", type) )
	{
		ErrNoType; return nullptr;
	}
	texture_t* texture;
	auto i=texture_factory.find(type);
	if(i!=texture_factory.end()) texture = i->second(params,*this);
	else
	{
		ErrUnkType(type); return nullptr;
	}
	if(texture)
	{
		texture_table[name] = texture;
		InfoVerboseSuccess(name, type);
		return texture;
	}
	ErrOnCreate(type);
	return nullptr;
}

shaderNode_t* renderEnvironment_t::createShaderNode(const std::string &name, paraMap_t &params)
{
	std::string pname = "ShaderNode";
	if(shader_table.find(name) != shader_table.end() )
	{
		WarnExist; return nullptr;
	}
	std::string type;
	if(! params.getParam("type", type) )
	{
		ErrNoType; return nullptr;
	}
	shaderNode_t* shader;
	auto i=shader_factory.find(type);
	if(i!=shader_factory.end()) shader = i->second(params,*this);
	else
	{
		ErrUnkType(type); return nullptr;
	}
	if(shader)
	{
		shader_table[name] = shader;
		InfoVerboseSuccess(name, type);
		return shader;
	}
	ErrOnCreate(type);
	return nullptr;
}

material_t* renderEnvironment_t::createMaterial(const std::string &name, paraMap_t &params, std::list<paraMap_t> &eparams)
{
	std::string pname = "Material";
	if(material_table.find(name) != material_table.end() )
	{
		WarnExist; return nullptr;
	}
	std::string type;
	if(! params.getParam("type", type) )
	{
		ErrNoType; return nullptr;
	}
	params["name"] = name;
	material_t* material;
	auto i=material_factory.find(type);
	if(i!=material_factory.end()) material = i->second(params, eparams, *this);
	else
	{
		ErrUnkType(type); return nullptr;
	}
	if(material)
	{
		material_table[name] = material;
		InfoVerboseSuccess(name, type);
		return material;
	}
	ErrOnCreate(type);
	return nullptr;
}

background_t* renderEnvironment_t::createBackground(const std::string &name, paraMap_t &params)
{
	std::string pname = "Background";
	if(background_table.find(name) != background_table.end() )
	{
		WarnExist; return nullptr;
	}
	std::string type;
	if(! params.getParam("type", type) )
	{
		ErrNoType; return nullptr;
	}
	background_t* background;
	auto i=background_factory.find(type);
	if(i!=background_factory.end()) background = i->second(params,*this);
	else
	{
		ErrUnkType(type); return nullptr;
	}
	if(background)
	{
		background_table[name] = background;
		InfoVerboseSuccess(name, type);
		return background;
	}
	ErrOnCreate(type);
	return nullptr;
}

imageHandler_t* renderEnvironment_t::createImageHandler(const std::string &name, paraMap_t &params, bool addToTable)
{
	std::string pname = "ImageHandler";
	std::stringstream newname;
	int sufixCount = 0;

	newname << name;

	if(addToTable)
	{
		while(true)
		{
			if(imagehandler_table.find(newname.str()) != imagehandler_table.end() )
			{
				newname.seekg(0, std::ios::beg);
				newname << name << ".";
				newname.width(3);
				newname.fill('0');
				newname.flags(std::ios::right);
				newname << sufixCount;
				sufixCount++;
			}
			else break;
		}
	}

	std::string type;

	if(! params.getParam("type", type) )
	{
		ErrNoType; return nullptr;
	}

	imageHandler_t* ih = nullptr;
	auto i=imagehandler_factory.find(type);

	if(i!=imagehandler_factory.end())
	{
		ih = i->second(params,*this);
	}
	else
	{
		ErrUnkType(type); return nullptr;
	}

	if(ih)
	{
		if(addToTable) imagehandler_table[newname.str()] = ih;

		InfoVerboseSuccess(newname.str(), type);

		return ih;
	}

	ErrOnCreate(type);

	return nullptr;
}

object3d_t* renderEnvironment_t::createObject(const std::string &name, paraMap_t &params)
{
	std::string pname = "Object";
	if(object_table.find(name) != object_table.end() )
	{
		WarnExist; return nullptr;
	}
	std::string type;
	if(! params.getParam("type", type) )
	{
		ErrNoType; return nullptr;
	}
	object3d_t* object;
	auto i=object_factory.find(type);
	if(i!=object_factory.end()) object = i->second(params,*this);
	else
	{
		ErrUnkType(type); return nullptr;
	}
	if(object)
	{
		object_table[name] = object;
		InfoVerboseSuccess(name, type);
		return object;
	}
	ErrOnCreate(type);
	return nullptr;
}

camera_t* renderEnvironment_t::createCamera(const std::string &name, paraMap_t &params)
{
	std::string pname = "Camera";
	if(camera_table.find(name) != camera_table.end() )
	{
		WarnExist; return nullptr;
	}
	std::string type;
	if(! params.getParam("type", type) )
	{
		ErrNoType; return nullptr;
	}
	camera_t* camera;
	auto i=camera_factory.find(type);
	if(i!=camera_factory.end()) camera = i->second(params,*this);
	else
	{
		ErrUnkType(type); return nullptr;
	}
	if(camera)
	{
		camera_table[name] = camera;
		InfoVerboseSuccess(name, type);
		int viewNumber = renderPasses.view_names.size();
		camera->set_camera_name(name);
		renderPasses.view_names.push_back(camera->get_view_name());
		
		Y_INFO << "Environment: View number=" << viewNumber << ", view name: '" << renderPasses.view_names[viewNumber] << "', camera name: '" << camera->get_camera_name() << "'" << yendl;

		return camera;
	}
	ErrOnCreate(type);
	return nullptr;
}

integrator_t* renderEnvironment_t::createIntegrator(const std::string &name, paraMap_t &params)
{
	std::string pname = "Integrator";
	if(integrator_table.find(name) != integrator_table.end() )
	{
		WarnExist; return nullptr;
	}
	std::string type;
	if(! params.getParam("type", type) )
	{
		ErrNoType; return nullptr;
	}
	integrator_t* integrator;
	auto i=integrator_factory.find(type);
	if(i!=integrator_factory.end()) integrator = i->second(params,*this);
	else
	{
		ErrUnkType(type); return nullptr;
	}
	if(integrator)
	{
		integrator_table[name] = integrator;
		InfoVerboseSuccess(name, type);
		if(type == "bidirectional") Y_WARNING << "The Bidirectional integrator is DEPRECATED. It might give unexpected and perhaps even incorrect render results. This integrator is no longer supported, will not receive any fixes/updates in the short/medium term and might be removed in future versions. Use at your own risk." << yendl; 
		return integrator;
	}
	ErrOnCreate(type);
	return nullptr;
}

void renderEnvironment_t::setupRenderPasses(const paraMap_t &params)
{
	std::string externalPass, internalPass;
	int pass_mask_obj_index = 0, pass_mask_mat_index = 0;
	bool pass_mask_invert = false;
	bool pass_mask_only = false;

	params.getParam("pass_mask_obj_index", pass_mask_obj_index);
	params.getParam("pass_mask_mat_index", pass_mask_mat_index);
	params.getParam("pass_mask_invert", pass_mask_invert);
	params.getParam("pass_mask_only", pass_mask_only);

	//Adding the render passes and associating them to the internal YafaRay pass defined in the Blender Exporter "pass_xxx" parameters.
	for(auto it = renderPasses.extPassMapIntString.begin(); it != renderPasses.extPassMapIntString.end(); ++it)
	{
		externalPass = it->second;
		params.getParam("pass_" + externalPass, internalPass);
		if(internalPass != "disabled" && internalPass != "") renderPasses.extPass_add(externalPass, internalPass);
	}

	renderPasses.set_pass_mask_obj_index((float) pass_mask_obj_index);
	renderPasses.set_pass_mask_mat_index((float) pass_mask_mat_index);
	renderPasses.set_pass_mask_invert(pass_mask_invert);
	renderPasses.set_pass_mask_only(pass_mask_only);
}
		
imageFilm_t* renderEnvironment_t::createImageFilm(const paraMap_t &params, colorOutput_t &output)
{
	const std::string *name=0;
	const std::string *tiles_order=0;
	int width=320, height=240, xstart=0, ystart=0;
	std::string color_space_string = "Raw_Manual_Gamma";
	colorSpaces_t color_space = RAW_MANUAL_GAMMA;
	std::string color_space_string2 = "Raw_Manual_Gamma";
	colorSpaces_t color_space2 = RAW_MANUAL_GAMMA;
	float filt_sz = 1.5, gamma=1.f, gamma2=1.f;
	bool showSampledPixels = false;
	int tileSize = 32;
	bool premult = false;
	bool premult2 = false;
	std::string images_autosave_interval_type_string = "none";
	int images_autosave_interval_type = AUTOSAVE_NONE;
	int images_autosave_interval_passes = 1;
	double images_autosave_interval_seconds = 300.0;
	std::string film_save_load_string = "none";
	int film_save_load = FILM_FILE_NONE;
	bool film_save_binary_format = true;
	std::string film_autosave_interval_type_string = "none";
	int film_autosave_interval_type = AUTOSAVE_NONE;
	int film_autosave_interval_passes = 1;
	double film_autosave_interval_seconds = 300.0;
	
	params.getParam("color_space", color_space_string);
	params.getParam("gamma", gamma);
	params.getParam("color_space2", color_space_string2);
	params.getParam("gamma2", gamma2);
	params.getParam("AA_pixelwidth", filt_sz);
	params.getParam("width", width); // width of rendered image
	params.getParam("height", height); // height of rendered image
	params.getParam("xstart", xstart); // x-offset (for cropped rendering)
	params.getParam("ystart", ystart); // y-offset (for cropped rendering)
	params.getParam("filter_type", name); // AA filter type
	params.getParam("show_sam_pix", showSampledPixels); // Show pixels marked to be resampled on adaptative sampling
	params.getParam("tile_size", tileSize); // Size of the render buckets or tiles
	params.getParam("tiles_order", tiles_order); // Order of the render buckets or tiles
	params.getParam("premult", premult); // Premultipy Alpha channel for better alpha antialiasing against bg
	params.getParam("premult2", premult2); // Premultipy Alpha channel for better alpha antialiasing against bg, for the optional secondary output
	params.getParam("images_autosave_interval_type", images_autosave_interval_type_string);
	params.getParam("images_autosave_interval_passes", images_autosave_interval_passes);
	params.getParam("images_autosave_interval_seconds", images_autosave_interval_seconds);
	params.getParam("film_save_load", film_save_load_string);
	params.getParam("film_save_binary_format", film_save_binary_format); // If enabled, it will autosave the Image Film in binary format (faster, smaller, but not portable). Otherwise it will autosave in text format (portable but bigger and slower)
	params.getParam("film_autosave_interval_type", film_autosave_interval_type_string);
	params.getParam("film_autosave_interval_passes", film_autosave_interval_passes);
	params.getParam("film_autosave_interval_seconds", film_autosave_interval_seconds);
	
	Y_DEBUG << "Images autosave: " << images_autosave_interval_type_string << ", " << images_autosave_interval_passes << ", " << images_autosave_interval_seconds << yendl;

	Y_DEBUG << "ImageFilm autosave: " << film_save_load_string << ", " << film_autosave_interval_type_string << ", " << film_autosave_interval_passes << ", " << film_autosave_interval_seconds << yendl;
		
	if(color_space_string == "sRGB") color_space = SRGB;
	else if(color_space_string == "XYZ") color_space = XYZ_D65;
	else if(color_space_string == "LinearRGB") color_space = LINEAR_RGB;
	else if(color_space_string == "Raw_Manual_Gamma") color_space = RAW_MANUAL_GAMMA;
	else color_space = SRGB;

	if(color_space_string2 == "sRGB") color_space2 = SRGB;
	else if(color_space_string2 == "XYZ") color_space2 = XYZ_D65;
	else if(color_space_string2 == "LinearRGB") color_space2 = LINEAR_RGB;
	else if(color_space_string2 == "Raw_Manual_Gamma") color_space2 = RAW_MANUAL_GAMMA;
	else color_space2 = SRGB;
	
	if(images_autosave_interval_type_string == "pass-interval") images_autosave_interval_type = AUTOSAVE_PASS_INTERVAL;
	else if(images_autosave_interval_type_string == "time-interval") images_autosave_interval_type = AUTOSAVE_TIME_INTERVAL;
	else images_autosave_interval_type = AUTOSAVE_NONE;

	if(film_save_load_string == "load-save") film_save_load = FILM_FILE_LOAD_SAVE;
	else if(film_save_load_string == "save") film_save_load = FILM_FILE_SAVE;
	else film_save_load = FILM_FILE_NONE;

	if(film_autosave_interval_type_string == "pass-interval") film_autosave_interval_type = AUTOSAVE_PASS_INTERVAL;
	else if(film_autosave_interval_type_string == "time-interval") film_autosave_interval_type = AUTOSAVE_TIME_INTERVAL;
	else film_autosave_interval_type = AUTOSAVE_NONE;
	
    output.initTilesPasses(camera_table.size(), renderPasses.extPassesSize());
    
	imageFilm_t::filterType type=imageFilm_t::BOX;
	if(name)
	{
		if(*name == "mitchell") type = imageFilm_t::MITCHELL;
		else if(*name == "gauss") type = imageFilm_t::GAUSS;
		else if(*name == "lanczos") type = imageFilm_t::LANCZOS;
		else type = imageFilm_t::BOX;
	}
	else Y_WARN_ENV << "No AA filter defined defaulting to Box!" << yendl;

	imageSpliter_t::tilesOrderType tilesOrder=imageSpliter_t::CENTRE_RANDOM;
	if(tiles_order)
	{
		if(*tiles_order == "linear") tilesOrder = imageSpliter_t::LINEAR;
		else if(*tiles_order == "random") tilesOrder = imageSpliter_t::RANDOM;
		else if(*tiles_order == "centre") tilesOrder = imageSpliter_t::CENTRE_RANDOM;
	}
	else Y_VERBOSE_ENV << "Defaulting to Centre tiles order." << yendl; // this is info imho not a warning

	imageFilm_t *film = new imageFilm_t(width, height, xstart, ystart, output, filt_sz, type, this, showSampledPixels, tileSize, tilesOrder, premult);
	
	if(color_space == RAW_MANUAL_GAMMA)
	{
		if(gamma > 0 && std::fabs(1.f-gamma) > 0.001) film->setColorSpace(color_space, gamma);
		else film->setColorSpace(LINEAR_RGB, 1.f); //If the gamma is too close to 1.f, or negative, ignore gamma and do a pure linear RGB processing without gamma.
	}
	else film->setColorSpace(color_space, gamma);

	if(color_space2 == RAW_MANUAL_GAMMA)
	{
		if(gamma2 > 0 && std::fabs(1.f-gamma2) > 0.001) film->setColorSpace2(color_space2, gamma2);
		else film->setColorSpace2(LINEAR_RGB, 1.f); //If the gamma is too close to 1.f, or negative, ignore gamma and do a pure linear RGB processing without gamma.
	}
	else film->setColorSpace2(color_space2, gamma2);

	film->setPremult2(premult2);

	film->setImagesAutoSaveIntervalType(images_autosave_interval_type);
	film->setImagesAutoSaveIntervalSeconds(images_autosave_interval_seconds);
	film->setImagesAutoSaveIntervalPasses(images_autosave_interval_passes);

	film->setFilmFileSaveLoad(film_save_load);
	film->setFilmFileSaveBinaryFormat(film_save_binary_format);
	film->setFilmAutoSaveIntervalType(film_autosave_interval_type);
	film->setFilmAutoSaveIntervalSeconds(film_autosave_interval_seconds);
	film->setFilmAutoSaveIntervalPasses(film_autosave_interval_passes);
	
	if(images_autosave_interval_type == AUTOSAVE_PASS_INTERVAL) Y_INFO_ENV << "AutoSave partially rendered image every " << images_autosave_interval_passes << " passes" << yendl;

	if(images_autosave_interval_type == AUTOSAVE_TIME_INTERVAL) Y_INFO_ENV << "AutoSave partially rendered image every " << images_autosave_interval_seconds << " seconds" << yendl;

	if(film_save_load != FILM_FILE_NONE && film_save_binary_format) Y_INFO_ENV << "Enabling imageFilm file saving feature in binary format (smaller, faster but not portable among systems)" << yendl;
	if(film_save_load != FILM_FILE_NONE && !film_save_binary_format) Y_INFO_ENV << "Enabling imageFilm file saving in text format (portable among systems but bigger and slower)" << yendl;
	if(film_save_load == FILM_FILE_LOAD_SAVE) Y_INFO_ENV << "Enabling imageFilm Loading feature. It will load and combine the ImageFilm files from the currently selected image output folder before start rendering, autodetecting each film format (binary/text) automatically. If they don't match exactly the scene, bad results could happen. Use WITH CARE!" << yendl;

	if(film_autosave_interval_type == AUTOSAVE_PASS_INTERVAL) Y_INFO_ENV << "AutoSave internal imageFilm every " << film_autosave_interval_passes << " passes" << yendl;

	if(film_autosave_interval_type == AUTOSAVE_TIME_INTERVAL) Y_INFO_ENV << "AutoSave internal imageFilm image every " << film_autosave_interval_seconds << " seconds" << yendl;
	
	return film;
}

volumeHandler_t* renderEnvironment_t::createVolumeH(const std::string &name, const paraMap_t &params)
{
	std::string pname = "VolumeHandler";
	if(volume_table.find(name) != volume_table.end() )
	{
		WarnExist; return nullptr;
	}
	std::string type;
	if(! params.getParam("type", type) )
	{
		ErrNoType; return nullptr;
	}
	volumeHandler_t* volume;
	auto i=volume_factory.find(type);
	if(i!=volume_factory.end()) volume = i->second(params,*this);
	else
	{
		ErrUnkType(type); return nullptr;
	}
	if(volume)
	{
		volume_table[name] = volume;
		InfoVerboseSuccess(name, type);
		return volume;
	}
	ErrOnCreate(type);
	return nullptr;
}

VolumeRegion* renderEnvironment_t::createVolumeRegion(const std::string &name, paraMap_t &params)
{
	std::string pname = "VolumeRegion";
	if(volumeregion_table.find(name) != volumeregion_table.end() )
	{
		WarnExist; return nullptr;
	}
	std::string type;
	if(! params.getParam("type", type) )
	{
		ErrNoType; return nullptr;
	}
	VolumeRegion* volumeregion;
	auto i=volumeregion_factory.find(type);
	if(i!=volumeregion_factory.end()) volumeregion = i->second(params,*this);
	else
	{
		ErrUnkType(type); return nullptr;
	}
	if(volumeregion)
	{
		volumeregion_table[name] = volumeregion;
		InfoVerboseSuccess(name, type);
		return volumeregion;
	}
	ErrOnCreate(type);
	return nullptr;
}

void renderEnvironment_t::setupLoggingAndBadge(const paraMap_t &params)
{
	bool logging_saveLog = false;
	bool logging_saveHTML = false;
	bool logging_drawRenderSettings = true;
	bool logging_drawAANoiseSettings = true;
	const std::string *logging_paramsBadgePosition = nullptr;
	const std::string *logging_title = nullptr;
	const std::string *logging_author = nullptr;
	const std::string *logging_contact = nullptr;
	const std::string *logging_comments = nullptr;
	const std::string *logging_customIcon = nullptr;

	params.getParam("logging_paramsBadgePosition", logging_paramsBadgePosition);
	params.getParam("logging_saveLog", logging_saveLog);
	params.getParam("logging_saveHTML", logging_saveHTML);
	params.getParam("logging_drawRenderSettings", logging_drawRenderSettings);
	params.getParam("logging_drawAANoiseSettings", logging_drawAANoiseSettings);
	params.getParam("logging_author", logging_author);
	params.getParam("logging_title", logging_title);
	params.getParam("logging_contact", logging_contact);
	params.getParam("logging_comments", logging_comments);
	params.getParam("logging_customIcon", logging_customIcon);

	yafLog.setSaveLog(logging_saveLog);
	yafLog.setSaveHTML(logging_saveHTML);
	yafLog.setDrawRenderSettings(logging_drawRenderSettings);
	yafLog.setDrawAANoiseSettings(logging_drawAANoiseSettings);
	if(logging_paramsBadgePosition) yafLog.setParamsBadgePosition(*logging_paramsBadgePosition);
	if(logging_title) yafLog.setLoggingTitle(*logging_title);
	if(logging_author) yafLog.setLoggingAuthor(*logging_author);
	if(logging_contact) yafLog.setLoggingContact(*logging_contact);
	if(logging_comments) yafLog.setLoggingComments(*logging_comments);
	if(logging_customIcon) yafLog.setLoggingCustomIcon(*logging_customIcon);
}

/*! setup the scene for rendering (set camera, background, integrator, create image film,
	set antialiasing etc.)
	attention: since this function creates an image film and asigns it to the scene,
	you need to delete it before deleting the scene!
*/
bool renderEnvironment_t::setupScene(scene_t &scene, const paraMap_t &params, colorOutput_t &output, progressBar_t *pb)
{
	const std::string *name=0;
	int AA_passes=1, AA_samples=1, AA_inc_samples=1, nthreads=-1, nthreads_photons=-1;
	double AA_threshold=0.05;
	float AA_resampled_floor=0.f;
	float AA_sample_multiplier_factor = 1.f;
	float AA_light_sample_multiplier_factor = 1.f;
	float AA_indirect_sample_multiplier_factor = 1.f;
	bool AA_detect_color_noise = false;
	std::string AA_dark_detection_type_string = "none";
	int AA_dark_detection_type = DARK_DETECTION_NONE;
	float AA_dark_threshold_factor = 0.f;
	int AA_variance_edge_size = 10;
	int AA_variance_pixels = 0;
	float AA_clamp_samples = 0.f;
	float AA_clamp_indirect = 0.f;
	
	bool adv_auto_shadow_bias_enabled=true;
	float adv_shadow_bias_value=YAF_SHADOW_BIAS;
	bool adv_auto_min_raydist_enabled=true;
	float adv_min_raydist_value=MIN_RAYDIST;
	int adv_base_sampling_offset = 0;
	int adv_computer_node = 0;

	if(! params.getParam("camera_name", name) )
	{
		Y_ERROR_ENV << "Specify a Camera!!" << yendl;
		return false;
	}
	
	if(!params.getParam("integrator_name", name) )
	{
		Y_ERROR_ENV << "Specify an Integrator!!" << yendl;
		return false;
	}

	integrator_t *inte = this->getIntegrator(*name);

	if(!inte)
	{
		Y_ERROR_ENV << "Specify an _existing_ Integrator!!" << yendl;
		return false;
	}

	if(inte->integratorType() != integrator_t::SURFACE)
	{
		Y_ERROR_ENV << "Integrator is no surface integrator!" << yendl;
		return false;
	}

	if(! params.getParam("volintegrator_name", name) )
	{
		Y_ERROR_ENV << "Specify a Volume Integrator!" << yendl;
		return false;
	}

	integrator_t *volInte = this->getIntegrator(*name);

	background_t *backg = nullptr;
	if( params.getParam("background_name", name) )
	{
		backg = this->getBackground(*name);
		if(!backg) Y_ERROR_ENV << "please specify an _existing_ Background!!" << yendl;
	}

	params.getParam("AA_passes", AA_passes);
	params.getParam("AA_minsamples", AA_samples);
	AA_inc_samples = AA_samples;
	params.getParam("AA_inc_samples", AA_inc_samples);
	params.getParam("AA_threshold", AA_threshold);
	params.getParam("AA_resampled_floor", AA_resampled_floor);
	params.getParam("AA_sample_multiplier_factor", AA_sample_multiplier_factor);
	params.getParam("AA_light_sample_multiplier_factor", AA_light_sample_multiplier_factor);
	params.getParam("AA_indirect_sample_multiplier_factor", AA_indirect_sample_multiplier_factor);
	params.getParam("AA_detect_color_noise", AA_detect_color_noise);
	params.getParam("AA_dark_detection_type", AA_dark_detection_type_string);
	params.getParam("AA_dark_threshold_factor", AA_dark_threshold_factor);
	params.getParam("AA_variance_edge_size", AA_variance_edge_size);
	params.getParam("AA_variance_pixels", AA_variance_pixels);
	params.getParam("AA_clamp_samples", AA_clamp_samples);
	params.getParam("AA_clamp_indirect", AA_clamp_indirect);
	params.getParam("threads", nthreads); // number of threads, -1 = auto detection
	
	nthreads_photons = nthreads;	//if no "threads_photons" parameter exists, make "nthreads_photons" equal to render threads
	
	params.getParam("threads_photons", nthreads_photons); // number of threads for photon mapping, -1 = auto detection
	params.getParam("adv_auto_shadow_bias_enabled", adv_auto_shadow_bias_enabled);
	params.getParam("adv_shadow_bias_value", adv_shadow_bias_value);
	params.getParam("adv_auto_min_raydist_enabled", adv_auto_min_raydist_enabled);
	params.getParam("adv_min_raydist_value", adv_min_raydist_value);
	params.getParam("adv_base_sampling_offset", adv_base_sampling_offset); //Base sampling offset, in case of multi-computer rendering each should have a different offset so they don't "repeat" the same samples (user configurable)
	params.getParam("adv_computer_node", adv_computer_node); //Computer node in multi-computer render environments/render farms
	imageFilm_t *film = createImageFilm(params, output);

	if (pb)
	{
		film->setProgressBar(pb);
		inte->setProgressBar(pb);
	}

	params.getParam("filter_type", name); // AA filter type
	
	std::stringstream aaSettings;
	aaSettings << "AA Settings (" << ((name)?*name:"box") << "):";
	yafLog.appendAANoiseSettings(aaSettings.str());
	
	if(AA_dark_detection_type_string == "linear") AA_dark_detection_type = DARK_DETECTION_LINEAR;
	else if(AA_dark_detection_type_string == "curve") AA_dark_detection_type = DARK_DETECTION_CURVE;
	else AA_dark_detection_type = DARK_DETECTION_NONE;
	
	//setup scene and render.
	scene.setImageFilm(film);
	scene.setSurfIntegrator((surfaceIntegrator_t*)inte);
	scene.setVolIntegrator((volumeIntegrator_t*)volInte);
	scene.setAntialiasing(AA_samples, AA_passes, AA_inc_samples, AA_threshold, AA_resampled_floor, AA_sample_multiplier_factor, AA_light_sample_multiplier_factor, AA_indirect_sample_multiplier_factor, AA_detect_color_noise, AA_dark_detection_type, AA_dark_threshold_factor, AA_variance_edge_size, AA_variance_pixels, AA_clamp_samples, AA_clamp_indirect);
	scene.setNumThreads(nthreads);
	scene.setNumThreadsPhotons(nthreads_photons);
	if(backg) scene.setBackground(backg);
	scene.shadowBiasAuto = adv_auto_shadow_bias_enabled;
	scene.shadowBias = adv_shadow_bias_value;
	scene.rayMinDistAuto = adv_auto_min_raydist_enabled;
	scene.rayMinDist = adv_min_raydist_value;

	Y_DEBUG << "adv_base_sampling_offset="<<adv_base_sampling_offset<<yendl;
	film->setBaseSamplingOffset(adv_base_sampling_offset);
	film->setComputerNode(adv_computer_node);

	return true;
}

void renderEnvironment_t::registerFactory(const std::string &name,light_factory_t *f)
{
	light_factory[name]=f;
	SuccessVerboseReg("Light", name);
}

void renderEnvironment_t::registerFactory(const std::string &name,material_factory_t *f)
{
	material_factory[name]=f;
	SuccessVerboseReg("Material", name);
}

void renderEnvironment_t::registerFactory(const std::string &name,texture_factory_t *f)
{
	texture_factory[name]=f;
	SuccessVerboseReg("Texture", name);
}

void renderEnvironment_t::registerFactory(const std::string &name,shader_factory_t *f)
{
	shader_factory[name]=f;
	SuccessVerboseReg("ShaderNode", name);
}

void renderEnvironment_t::registerFactory(const std::string &name,object_factory_t *f)
{
	object_factory[name]=f;
	SuccessVerboseReg("Object", name);
}

void renderEnvironment_t::registerFactory(const std::string &name,camera_factory_t *f)
{
	camera_factory[name]=f;
	SuccessVerboseReg("Camera", name);
}

void renderEnvironment_t::registerFactory(const std::string &name,background_factory_t *f)
{
	background_factory[name]=f;
	SuccessVerboseReg("Background", name);
}

void renderEnvironment_t::registerFactory(const std::string &name,integrator_factory_t *f)
{
	integrator_factory[name]=f;
	SuccessVerboseReg("Integrator", name);
}

void renderEnvironment_t::registerFactory(const std::string &name,volume_factory_t *f)
{
	volume_factory[name]=f;
	SuccessVerboseReg("VolumetricHandler", name);
}

void renderEnvironment_t::registerFactory(const std::string &name,volumeregion_factory_t *f)
{
	volumeregion_factory[name]=f;
	SuccessVerboseReg("VolumeRegion", name);
}

void renderEnvironment_t::registerImageHandler(const std::string &name, const std::string &validExtensions, const std::string &fullName, imagehandler_factory_t *f)
{
	imagehandler_factory[name] = f;
	imagehandler_fullnames[name] = fullName;
	imagehandler_extensions[name] = validExtensions;
	SuccessVerboseReg("ImageHandler", name);
}

std::vector<std::string> renderEnvironment_t::listImageHandlers()
{
	std::vector<std::string> ret;
	if(imagehandler_fullnames.size() > 0)
	{
		for(auto i=imagehandler_fullnames.begin(); i != imagehandler_fullnames.end(); ++i)
		{
			ret.push_back(i->first);
		}
	}
	else Y_ERROR_ENV << "There is no image handlers registrered" << yendl;

	return ret;
}

std::vector<std::string> renderEnvironment_t::listImageHandlersFullName()
{
	std::vector<std::string> ret;
	if(imagehandler_fullnames.size() > 0)
	{
		for(auto i=imagehandler_fullnames.begin(); i != imagehandler_fullnames.end(); ++i)
		{
			ret.push_back(i->second);
		}
	}
	else Y_ERROR_ENV << "There is no image handlers registrered" << yendl;

	return ret;
}

std::string renderEnvironment_t::getImageFormatFromFullName(const std::string &fullname)
{
	std::string ret;
	if(imagehandler_fullnames.size() > 0)
	{
		for(auto i=imagehandler_fullnames.begin(); i != imagehandler_fullnames.end(); ++i)
		{
			if(i->second == fullname) ret = i->first;
		}
	}
	else Y_ERROR_ENV << "There is no image handlers registrered" << yendl;

	return ret;
}

std::string renderEnvironment_t::getImageFormatFromExtension(const std::string &ext)
{
	std::string ret = "";

	if(ext == "" || ext == " ") return ret;

	if(imagehandler_extensions.size() > 0)
	{
		for(auto i=imagehandler_extensions.begin(); i != imagehandler_extensions.end(); ++i)
		{
			if(i->second.find(ext) != std::string::npos) ret = i->first;
		}
	}
	else Y_ERROR_ENV << "There is no image handlers registrered" << yendl;

	return ret;
}

std::string renderEnvironment_t::getImageFullNameFromFormat(const std::string &format)
{
	std::string ret;
	if(imagehandler_fullnames.size() > 0)
	{
		for(auto i=imagehandler_fullnames.begin(); i != imagehandler_fullnames.end(); ++i)
		{
			if(i->first == format) ret = i->second;
		}
	}
	else Y_ERROR_ENV << "There is no image handlers registrered" << yendl;

	return ret;
}

renderEnvironment_t::shader_factory_t* renderEnvironment_t::getShaderNodeFactory(const std::string &name)const
{
	auto i=shader_factory.find(name);
	if(i!=shader_factory.end()) return i->second;
	Y_ERROR_ENV << "There is no factory for '"<<name<<"'\n";
	return nullptr;
}

__END_YAFRAY
