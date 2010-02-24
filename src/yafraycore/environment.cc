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
#ifndef __MINGW32__
#define NOMINMAX
#endif
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
#include <core_api/object3d.h>
#include <core_api/volume.h>
#include <yafraycore/std_primitives.h>
#include <yaf_revision.h>
#include <string>
#include <sstream>

__BEGIN_YAFRAY
#define ENV_TAG << " [Environment] "
#define Y_INFO_ENV Y_INFO ENV_TAG
#define Y_ERROR_ENV Y_ERROR ENV_TAG
#define Y_WARN_ENV Y_WARNING ENV_TAG

#define WarnExist Y_WARN_ENV << "Sorry, " << pname << " already exists!\n"

#define ErrNoType Y_ERROR_ENV << "Type of " << pname << " not specified!\n";
#define ErrUnkType(t) Y_ERROR_ENV << "Don't know how to create " << pname << " of type '" << t << "'!\n"
#define ErrOnCreate(t) Y_ERROR_ENV << "No " << pname << " was constructed by plugin '" << t << "'!\n"

#define InfoSucces(name, t) Y_INFO_ENV << "Added " << pname << " '"<< name << "' (" << t << ")!\n"

#define SuccessReg(t, name) Y_INFO_ENV << "Registered " << t << " type '" << name << "'\n"

renderEnvironment_t::renderEnvironment_t()
{
#ifdef RELEASE
	std::cout << PACKAGE << " " << VERSION << std::endl;
#else
	std::cout << PACKAGE << " (" << YAF_SVN_REV << ")" << std::endl;
#endif
	// add builtin types to factory tables:
	//camera_factory["perspective"] = perspectiveCam_t::factory;
	//camera_factory["architect"] = architectCam_t::factory;
	//camera_factory["orthographic"] = orthoCam_t::factory;
	//camera_factory["angular"] = angularCam_t::factory;
	object_factory["sphere"] = sphere_factory;
	Debug=0;
}

template <class T>
void freeMap(std::map< std::string, T* > &map)
{
	typename std::map<std::string, T *>::iterator i;
	for(i=map.begin(); i!=map.end(); ++i) delete i->second;
}

renderEnvironment_t::~renderEnvironment_t()
{
	freeMap(light_table);
	freeMap(texture_table);
	freeMap(material_table);
	freeMap(object_table);
	freeMap(camera_table);
//	FREEMAP(filter_t,filter_table);
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

	light_table.clear();
	texture_table.clear();
	material_table.clear();
	object_table.clear();
	camera_table.clear();
	background_table.clear();
	integrator_table.clear();
	volume_table.clear();
	volumeregion_table.clear();
}

void renderEnvironment_t::loadPlugins(const std::string &path)
{
	typedef void (reg_t)(renderEnvironment_t &);
	Y_INFO_ENV << "Loading plugins ...\n";
	std::list<std::string> plugins=listDir(path);
	
	for(std::list<std::string>::iterator i=plugins.begin();i!=plugins.end();++i)
	{
		sharedlibrary_t plug(i->c_str());
		if(!plug.isOpen()) continue;
		reg_t *registerPlugin;
		registerPlugin=(reg_t *)plug.getSymbol("registerPlugin");
		if(registerPlugin==NULL) continue;
		registerPlugin(*this);
		pluginHandlers.push_back(plug);
	}
}

bool renderEnvironment_t::getPluginPath(std::string &path)
{
#ifdef _WIN32
	HKEY	hkey;
	DWORD 	dwType, dwSize;
	
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,"Software\\YafRay Team\\YafaRay",0,KEY_READ,&hkey)==ERROR_SUCCESS)
	{
		dwType = REG_EXPAND_SZ;
	 	dwSize = MAX_PATH;
		DWORD dwStat;

		char *pInstallDir=(char *)malloc(MAX_PATH);

  		dwStat=RegQueryValueEx(hkey, TEXT("InstallDir"), 
			NULL, NULL,(LPBYTE)pInstallDir, &dwSize);
		
		if (dwStat == NO_ERROR)
		{
			path = std::string(pInstallDir) + "\\plugins";
			free(pInstallDir);
			RegCloseKey(hkey);
			return true;
		}
		Y_ERROR_ENV << "Couldn't READ \'InstallDir\' value.\n";
		free(pInstallDir);
		RegCloseKey(hkey);
	}
	else Y_ERROR_ENV << "Couldn't find registry key.\n";

	std::cout <<"\n\nPlease fix your registry. Maybe you need add/modify\n\n" 
				<< " HKEY_LOCAL_MACHINE\\Software\\YafRay Team\\YafRay\\InstallDir\n\n"
				<< "key at registry. You can use \"regedit.exe\" to adjust it at "
				<< "your own risk.\n"
				<< "If you are unsure, reinstall YafRay\n";

	return false;
#else
	path = std::string(Y_PLUGINPATH);
	return true;
#endif
}


material_t* renderEnvironment_t::getMaterial(const std::string &name)const
{
	std::map<std::string,material_t *>::const_iterator i=material_table.find(name);
	if(i!=material_table.end()) return i->second;
	else return NULL;
}

texture_t* renderEnvironment_t::getTexture(const std::string &name)const
{
	std::map<std::string,texture_t *>::const_iterator i=texture_table.find(name);
	if(i!=texture_table.end()) return i->second;
	else return NULL;
}

camera_t* renderEnvironment_t::getCamera(const std::string &name)const
{
	std::map<std::string,camera_t *>::const_iterator i=camera_table.find(name);
	if(i!=camera_table.end()) return i->second;
	else return NULL;
}

background_t* renderEnvironment_t::getBackground(const std::string &name)const
{
	std::map<std::string,background_t *>::const_iterator i=background_table.find(name);
	if(i!=background_table.end()) return i->second;
	else return NULL;
}

integrator_t* renderEnvironment_t::getIntegrator(const std::string &name)const
{
	std::map<std::string,integrator_t *>::const_iterator i=integrator_table.find(name);
	if(i!=integrator_table.end()) return i->second;
	else return NULL;
}

shaderNode_t* renderEnvironment_t::getShaderNode(const std::string &name)const
{
	std::map<std::string,shaderNode_t *>::const_iterator i=shader_table.find(name);
	if(i!=shader_table.end()) return i->second;
	else return NULL;
}

light_t* renderEnvironment_t::createLight(const std::string &name, paraMap_t &params)
{
	std::string pname = "Light";
	if(light_table.find(name) != light_table.end() )
	{
		WarnExist; return 0;
	}
	std::string type;
	if(! params.getParam("type", type) )
	{
		ErrNoType; return 0;
	}
	light_t* light;
	std::map<std::string,light_factory_t *>::iterator i=light_factory.find(type);
	if(i!=light_factory.end()) light = i->second(params,*this);
	else
	{
		ErrUnkType(type); return 0;
	}
	if(light)
	{
		light_table[name] = light;
		InfoSucces(name, type);
		return light;
	}
	ErrOnCreate(type);
	return 0;
}

texture_t* renderEnvironment_t::createTexture(const std::string &name, paraMap_t &params)
{
	std::string pname = "Texture";
	if(texture_table.find(name) != texture_table.end() )
	{
		WarnExist; return 0;
	}
	std::string type;
	if(! params.getParam("type", type) )
	{
		ErrNoType; return 0;
	}
	texture_t* texture;
	std::map<std::string,texture_factory_t *>::iterator i=texture_factory.find(type);
	if(i!=texture_factory.end()) texture = i->second(params,*this);
	else
	{
		ErrUnkType(type); return 0;
	}
	if(texture)
	{
		texture_table[name] = texture;
		InfoSucces(name, type);
		return texture;
	}
	ErrOnCreate(type);
	return 0;
}

shaderNode_t* renderEnvironment_t::createShaderNode(const std::string &name, paraMap_t &params)
{
	std::string pname = "ShaderNode";
	if(shader_table.find(name) != shader_table.end() )
	{
		WarnExist; return 0;
	}
	std::string type;
	if(! params.getParam("type", type) )
	{
		ErrNoType; return 0;
	}
	shaderNode_t* shader;
	std::map<std::string,shader_factory_t *>::iterator i=shader_factory.find(type);
	if(i!=shader_factory.end()) shader = i->second(params,*this);
	else
	{
		ErrUnkType(type); return 0;
	}
	if(shader)
	{
		shader_table[name] = shader;
		InfoSucces(name, type);
		return shader;
	}
	ErrOnCreate(type);
	return 0;
}

material_t* renderEnvironment_t::createMaterial(const std::string &name, paraMap_t &params, std::list<paraMap_t> &eparams)
{
	std::string pname = "Material";
	if(material_table.find(name) != material_table.end() )
	{
		WarnExist; return 0;
	}
	std::string type;
	if(! params.getParam("type", type) )
	{
		ErrNoType; return 0;
	}
	params["name"] = name;
	material_t* material;
	std::map<std::string,material_factory_t *>::iterator i=material_factory.find(type);
	if(i!=material_factory.end()) material = i->second(params, eparams, *this);
	else
	{
		ErrUnkType(type); return 0;
	}
	if(material)
	{
		material_table[name] = material;
		InfoSucces(name, type);
		return material;
	}
	ErrOnCreate(type);
	return 0;
}	

background_t* renderEnvironment_t::createBackground(const std::string &name, paraMap_t &params)
{
	std::string pname = "Background";
	if(background_table.find(name) != background_table.end() )
	{
		WarnExist; return 0;
	}
	std::string type;
	if(! params.getParam("type", type) )
	{
		ErrNoType; return 0;
	}
	background_t* background;
	std::map<std::string,background_factory_t *>::iterator i=background_factory.find(type);
	if(i!=background_factory.end()) background = i->second(params,*this);
	else
	{
		ErrUnkType(type); return 0;
	}
	if(background)
	{
		background_table[name] = background;
		InfoSucces(name, type);
		return background;
	}
	ErrOnCreate(type);
	return 0;
}

object3d_t* renderEnvironment_t::createObject(const std::string &name, paraMap_t &params)
{
	std::string pname = "Object";
	if(object_table.find(name) != object_table.end() )
	{
		WarnExist; return 0;
	}
	std::string type;
	if(! params.getParam("type", type) )
	{
		ErrNoType; return 0;
	}
	object3d_t* object;
	std::map<std::string,object_factory_t *>::iterator i=object_factory.find(type);
	if(i!=object_factory.end()) object = i->second(params,*this);
	else
	{
		ErrUnkType(type); return 0;
	}
	if(object)
	{
		object_table[name] = object;
		InfoSucces(name, type);
		return object;
	}
	ErrOnCreate(type);
	return 0;
}

camera_t* renderEnvironment_t::createCamera(const std::string &name, paraMap_t &params)
{
	std::string pname = "Camera";
	if(camera_table.find(name) != camera_table.end() )
	{
		WarnExist; return 0;
	}
	std::string type;
	if(! params.getParam("type", type) )
	{
		ErrNoType; return 0;
	}
	camera_t* camera;
	std::map<std::string,camera_factory_t *>::iterator i=camera_factory.find(type);
	if(i!=camera_factory.end()) camera = i->second(params,*this);
	else
	{
		ErrUnkType(type); return 0;
	}
	if(camera)
	{
		camera_table[name] = camera;
		InfoSucces(name, type);
		return camera;
	}
	ErrOnCreate(type);
	return 0;
}

integrator_t* renderEnvironment_t::createIntegrator(const std::string &name, paraMap_t &params)
{
	std::string pname = "Integrator";
	if(integrator_table.find(name) != integrator_table.end() )
	{
		WarnExist; return 0;
	}
	std::string type;
	if(! params.getParam("type", type) )
	{
		ErrNoType; return 0;
	}
	integrator_t* integrator;
	std::map<std::string,integrator_factory_t *>::iterator i=integrator_factory.find(type);
	if(i!=integrator_factory.end()) integrator = i->second(params,*this);
	else
	{
		ErrUnkType(type); return 0;
	}
	if(integrator)
	{
		integrator_table[name] = integrator;
		InfoSucces(name, type);
		return integrator;
	}
	std::cout << "error: no integrator was constructed by plugin '"<<type<<"'!\n";
	return 0;
}

imageFilm_t* renderEnvironment_t::createImageFilm(const paraMap_t &params, colorOutput_t &output)
{
	const std::string *name=0;
	const std::string *tiles_order=0;
	int width=320, height=240, xstart=0, ystart=0;
	float filt_sz = 1.5, gamma=1.f;
	bool clamp = false;
	bool showSampledPixels = false;
	int tileSize = 32;
	bool premult = false;
	bool drawParams = false;
		
	params.getParam("gamma", gamma);
	params.getParam("clamp_rgb", clamp);
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
	params.getParam("drawParams", drawParams);
	
	imageFilm_t::filterType type=imageFilm_t::BOX;
	if(name)
	{
		if(*name == "mitchell") type=imageFilm_t::MITCHELL;
		else if(*name == "gauss") type=imageFilm_t::GAUSS;
	}
	else Y_WARN_ENV << "Defaulting to Box AA filter!\n";

	imageSpliter_t::tilesOrderType tilesOrder=imageSpliter_t::LINEAR;
	if(tiles_order)
	{
		if(*tiles_order == "linear") tilesOrder = imageSpliter_t::LINEAR;
		else if(*tiles_order == "random") tilesOrder = imageSpliter_t::RANDOM;
	}
	else Y_INFO_ENV << "Defaulting to Linear tiles order.\n"; // this is info imho not a warning
	
	imageFilm_t *film = new imageFilm_t(width, height, xstart, ystart, output, filt_sz, type, &(*this), showSampledPixels, tileSize, tilesOrder, premult, drawParams);

	film->setClamp(clamp);
	if(gamma > 0 && std::fabs(1.f-gamma) > 0.001) film->setGamma(gamma, true);

	return film;
}

volumeHandler_t* renderEnvironment_t::createVolumeH(const std::string &name, const paraMap_t &params)
{
	std::string pname = "VolumeHandler";
	if(volume_table.find(name) != volume_table.end() )
	{
		WarnExist; return 0;
	}
	std::string type;
	if(! params.getParam("type", type) )
	{
		ErrNoType; return 0;
	}
	volumeHandler_t* volume;
	std::map<std::string,volume_factory_t *>::iterator i=volume_factory.find(type);
	if(i!=volume_factory.end()) volume = i->second(params,*this);
	else
	{
		ErrUnkType(type); return 0;
	}
	if(volume)
	{
		volume_table[name] = volume;
		InfoSucces(name, type);
		return volume;
	}
	ErrOnCreate(type);
	return 0;
}

VolumeRegion* renderEnvironment_t::createVolumeRegion(const std::string &name, paraMap_t &params)
{
	std::string pname = "VolumeRegion";
	if(volumeregion_table.find(name) != volumeregion_table.end() )
	{
		WarnExist; return 0;
	}
	std::string type;
	if(! params.getParam("type", type) )
	{
		ErrNoType; return 0;
	}
	VolumeRegion* volumeregion;
	std::map<std::string,volumeregion_factory_t *>::iterator i=volumeregion_factory.find(type);
	if(i!=volumeregion_factory.end()) volumeregion = i->second(params,*this);
	else
	{
		ErrUnkType(type); return 0;
	}
	if(volumeregion)
	{
		volumeregion_table[name] = volumeregion;
		InfoSucces(name, type);
		return volumeregion;
	}
	ErrOnCreate(type);
	return 0;
}

/*! setup the scene for rendering (set camera, background, integrator, create image film,
	set antialiasing etc.)
	attention: since this function creates an image film and asigns it to the scene,
	you need to delete it before deleting the scene!
*/
bool renderEnvironment_t::setupScene(scene_t &scene, const paraMap_t &params, colorOutput_t &output, progressBar_t *pb)
{
	const std::string *name=0;
	int AA_passes=1, AA_samples=1, AA_inc_samples=1, nthreads=1;
	double AA_threshold=0.05;
	bool z_chan = false;
	bool drawParams = false;
	const std::string *custString = 0;
	std::stringstream aaSettings;
	
	if(! params.getParam("camera_name", name) ){ Y_ERROR_ENV << "Specify a Camera!!\n"; return false; }
	camera_t *cam = this->getCamera(*name);
	if(! cam){ Y_ERROR_ENV << "Specify an _existing_ Camera!!\n"; return false; }
	if(! params.getParam("integrator_name", name) ){ Y_ERROR_ENV << "Specify an Integrator!!\n"; return false; }
	integrator_t *inte = this->getIntegrator(*name);
	if(! inte){ Y_ERROR_ENV << "Specify an _existing_ Integrator!!\n"; return false; }
	if(inte->integratorType() != integrator_t::SURFACE){ Y_ERROR_ENV << "Integrator is no surface integrator!\n"; return false; }

	if(! params.getParam("volintegrator_name", name) ){ Y_ERROR_ENV << "Specify a Volume Integrator!\n"; return false; }
	integrator_t *volInte = this->getIntegrator(*name);

	background_t *backg = 0;
	if( params.getParam("background_name", name) )
	{
		backg = this->getBackground(*name);
		if(!backg) Y_ERROR_ENV << "please specify an _existing_ Background!!\n";
	}
	params.getParam("AA_passes", AA_passes);
	params.getParam("AA_minsamples", AA_samples);
	AA_inc_samples = AA_samples;
	params.getParam("AA_inc_samples", AA_inc_samples);
	params.getParam("AA_threshold", AA_threshold);
	params.getParam("threads", nthreads); // number of threads, -1 = auto detection
	params.getParam("z_channel", z_chan); // render z-buffer
	params.getParam("drawParams", drawParams);
	params.getParam("customString", custString);
	
	imageFilm_t *film = createImageFilm(params, output);
	if (pb)
	{
		film->setProgressBar(pb);
		inte->setProgressBar(pb);
	}
	if(z_chan) film->addChannel("Depth");
	params.getParam("filter_type", name); // AA filter type
	aaSettings << "AA Settings (" << ((name)?*name:"box") << "): " << AA_passes << ";" << AA_samples << ";" << AA_inc_samples;
	
	film->setAAParams(aaSettings.str());
	if(custString) film->setCustomString(*custString);
	
	//setup scene and render.
	scene.setImageFilm(film);
	scene.depthChannel(z_chan);
	scene.setCamera(cam);
	scene.setSurfIntegrator((surfaceIntegrator_t*)inte);
	scene.setVolIntegrator((volumeIntegrator_t*)volInte);
	scene.setAntialiasing(AA_samples, AA_passes, AA_inc_samples, AA_threshold);
	scene.setNumThreads(nthreads);
	if(backg) scene.setBackground(backg);
	
	return true;
}

void renderEnvironment_t::registerFactory(const std::string &name,light_factory_t *f)
{
	light_factory[name]=f;
	SuccessReg("Light", name);
}

void renderEnvironment_t::registerFactory(const std::string &name,material_factory_t *f)
{
	material_factory[name]=f;
	SuccessReg("Material", name);
}

void renderEnvironment_t::registerFactory(const std::string &name,texture_factory_t *f)
{
	texture_factory[name]=f;
	SuccessReg("Texture", name);
}

void renderEnvironment_t::registerFactory(const std::string &name,shader_factory_t *f)
{
	shader_factory[name]=f;
	SuccessReg("ShaderNode", name);
}

void renderEnvironment_t::registerFactory(const std::string &name,object_factory_t *f)
{
	object_factory[name]=f;
	SuccessReg("Object", name);
}

void renderEnvironment_t::registerFactory(const std::string &name,camera_factory_t *f)
{
	camera_factory[name]=f;
	SuccessReg("Camera", name);
}
/*
void render_t::registerFactory(const std::string &name,filter_factory_t *f)
{
	filter_factory[name]=f;
	SuccessReg(<filter type>, name);
}*/

void renderEnvironment_t::registerFactory(const std::string &name,background_factory_t *f)
{
	background_factory[name]=f;
	SuccessReg("Background", name);
}

void renderEnvironment_t::registerFactory(const std::string &name,integrator_factory_t *f)
{
	integrator_factory[name]=f;
	SuccessReg("Integrator", name);
}

void renderEnvironment_t::registerFactory(const std::string &name,volume_factory_t *f)
{
	volume_factory[name]=f;
	SuccessReg("VolumetricHandler", name);
}

void renderEnvironment_t::registerFactory(const std::string &name,volumeregion_factory_t *f)
{
	volumeregion_factory[name]=f;
	SuccessReg("VolumeRegion", name);
}

renderEnvironment_t::shader_factory_t* renderEnvironment_t::getShaderNodeFactory(const std::string &name)const
{
	std::map<std::string,shader_factory_t *>::const_iterator i=shader_factory.find(name);
	if(i!=shader_factory.end()) return i->second;
	Y_ERROR_ENV << "There is no factory for '"<<name<<"'\n";
	return 0;
}


/*void renderEnvironment_t::addToParamsString(const char *params)
{
	paramsString = paramsString + std::string(params);
}

const char* renderEnvironment_t::getParamsString()
{
	return paramsString.c_str();
}

void renderEnvironment_t::clearParamsString()
{
	paramsString = std::string("");
}

void renderEnvironment_t::setDrawParams(bool b) {
	drawParamsString = b;
}

bool renderEnvironment_t::getDrawParams() {
	return drawParamsString;
}
*/

__END_YAFRAY
