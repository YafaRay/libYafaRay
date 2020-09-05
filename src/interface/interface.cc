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

#include "interface/interface.h"
#include "yafaray_config.h"
#include "common/logging.h"
#include "common/session.h"
#include "common/environment.h"
#include "common/scene.h"
#include "common/matrix4.h"
#include "common/imagefilm.h"
#include "common/param.h"
#include <signal.h>

#ifdef WIN32
#include <windows.h>
#endif

BEGIN_YAFARAY

Scene *global_scene__ = nullptr;

#ifdef WIN32
BOOL WINAPI ctrlCHandler__(DWORD signal)
{
	if(global_scene__)
	{
		global_scene__->abort();
		session__.setStatusRenderAborted();
		Y_WARNING << "Interface: Render aborted by user." << YENDL;
	}
	else
	{
		session__.setStatusRenderAborted();
		Y_WARNING << "Interface: Render aborted by user." << YENDL;
		exit(1);
	}
	return TRUE;
}
#else
void ctrlCHandler__(int signal)
{
	if(global_scene__)
	{
		global_scene__->abort();
		session__.setStatusRenderAborted();
		Y_WARNING << "Interface: Render aborted by user." << YENDL;
	}
	else
	{
		session__.setStatusRenderAborted();
		Y_WARNING << "Interface: Render aborted by user." << YENDL;
		exit(1);
	}
}
#endif

Interface::Interface(): scene_(nullptr), film_(nullptr), input_gamma_(1.f), input_color_space_(RawManualGamma)
{
	//handle CTRL+C events
#ifdef WIN32
	SetConsoleCtrlHandler(ctrlCHandler__, true);
#else
	struct ::sigaction signal_handler;
	signal_handler.sa_handler = ctrlCHandler__;
	sigemptyset(&signal_handler.sa_mask);
	signal_handler.sa_flags = 0;
	sigaction(SIGINT, &signal_handler, nullptr);
#endif

	env_ = new RenderEnvironment();
	params_ = new ParamMap;
	eparams_ = new std::list<ParamMap>;
	cparams_ = params_;
}

Interface::~Interface()
{
	Y_VERBOSE << "Interface: Deleting scene..." << YENDL;
	if(scene_) delete scene_;
	Y_VERBOSE << "Interface: Deleting environment..." << YENDL;
	if(env_) delete env_;
	Y_INFO << "Interface: Done." << YENDL;
	if(film_) delete film_;
	delete params_;
	delete eparams_;
	logger__.clearAll();
}

void Interface::clearAll()
{
	Y_VERBOSE << "Interface: Cleaning environment..." << YENDL;
	env_->clearAll();
	Y_VERBOSE << "Interface: Deleting scene..." << YENDL;
	if(scene_) delete scene_;
	Y_VERBOSE << "Interface: Clearing film and parameter maps scene..." << YENDL;
	scene_ = nullptr;//new scene_t();
	if(film_) delete film_;
	film_ = nullptr;
	params_->clear();
	eparams_->clear();
	cparams_ = params_;
	Y_VERBOSE << "Interface: Cleanup done." << YENDL;
}

bool Interface::startScene(int type)
{
	if(scene_) delete scene_;
	scene_ = new Scene(env_);
	global_scene__ = scene_;	//for the CTRL+C handler
	scene_->setMode(type);
	env_->setScene(scene_);
	return true;
}

bool Interface::setLoggingAndBadgeSettings()
{
	env_->setupLoggingAndBadge(*params_);
	return true;
}

bool Interface::setupRenderPasses()
{
	env_->setupRenderPasses(*params_);
	return true;
}

bool Interface::setInteractive(bool interactive)
{
	session__.setInteractive(interactive);
	return true;
}

bool Interface::startGeometry() { return scene_->startGeometry(); }

bool Interface::endGeometry() { return scene_->endGeometry(); }

unsigned int Interface::getNextFreeId()
{
	ObjId_t id;
	id = scene_->getNextFreeId();
	return id;
}

bool Interface::startTriMesh(unsigned int id, int vertices, int triangles, bool has_orco, bool has_uv, int type, int obj_pass_index)
{
	bool success = scene_->startTriMesh(id, vertices, triangles, has_orco, has_uv, type, obj_pass_index);
	return success;
}

bool Interface::startCurveMesh(unsigned int id, int vertices, int obj_pass_index)
{
	bool success = scene_->startCurveMesh(id, vertices, obj_pass_index);
	return success;
}


bool Interface::startTriMeshPtr(unsigned int *id, int vertices, int triangles, bool has_orco, bool has_uv, int type, int obj_pass_index)
{
	Y_WARNING << "Interface: This method is going to be removed, please use getNextFreeID() and startTriMesh() for trimesh generation" << YENDL;
	ObjId_t obj_id;
	obj_id = scene_->getNextFreeId();
	if(obj_id > 0)
	{
		bool success = scene_->startTriMesh(obj_id, vertices, triangles, has_orco, has_uv, type, obj_pass_index);
		*id = obj_id;
		return success;
	}
	else
	{
		return false;
	}
}

bool Interface::endTriMesh() { return scene_->endTriMesh(); }
bool Interface::endCurveMesh(const Material *mat, float strand_start, float strand_end, float strand_shape) { return scene_->endCurveMesh(mat, strand_start, strand_end, strand_shape); }

int  Interface::addVertex(double x, double y, double z) { return scene_->addVertex(Point3(x, y, z)); }

int  Interface::addVertex(double x, double y, double z, double ox, double oy, double oz)
{
	return scene_->addVertex(Point3(x, y, z), Point3(ox, oy, oz));
}

void Interface::addNormal(double x, double y, double z)
{
	scene_->addNormal(Normal(x, y, z));
}

bool Interface::addTriangle(int a, int b, int c, const Material *mat) { return scene_->addTriangle(a, b, c, mat); }

bool Interface::addTriangle(int a, int b, int c, int uv_a, int uv_b, int uv_c, const Material *mat)
{
	return scene_->addTriangle(a, b, c, uv_a, uv_b, uv_c, mat);
}

int Interface::addUv(float u, float v) { return scene_->addUv(u, v); }

bool Interface::smoothMesh(unsigned int id, double angle) { return scene_->smoothMesh(id, angle); }

bool Interface::addInstance(unsigned int base_object_id, Matrix4 obj_to_world)
{
	return scene_->addInstance(base_object_id, obj_to_world);
}
// paraMap_t related functions:
void Interface::paramsSetPoint(const char *name, double x, double y, double z)
{
	(*cparams_)[std::string(name)] = Parameter(Point3(x, y, z));
}

void Interface::paramsSetString(const char *name, const char *s)
{
	(*cparams_)[std::string(name)] = Parameter(std::string(s));
}

void Interface::paramsSetBool(const char *name, bool b)
{
	(*cparams_)[std::string(name)] = Parameter(b);
}

void Interface::paramsSetInt(const char *name, int i)
{
	(*cparams_)[std::string(name)] = Parameter(i);
}

void Interface::paramsSetFloat(const char *name, double f)
{
	(*cparams_)[std::string(name)] = Parameter(f);
}

void Interface::paramsSetColor(const char *name, float r, float g, float b, float a)
{
	Rgba col(r, g, b, a);
	col.linearRgbFromColorSpace(input_color_space_, input_gamma_);
	(*cparams_)[std::string(name)] = Parameter(col);
}

void Interface::paramsSetColor(const char *name, float *rgb, bool with_alpha)
{
	Rgba col(rgb[0], rgb[1], rgb[2], (with_alpha ? rgb[3] : 1.f));
	col.linearRgbFromColorSpace(input_color_space_, input_gamma_);
	(*cparams_)[std::string(name)] = Parameter(col);
}

void Interface::paramsSetMatrix(const char *name, float m[4][4], bool transpose)
{
	if(transpose)	(*cparams_)[std::string(name)] = Matrix4(m).transpose();
	else		(*cparams_)[std::string(name)] = Matrix4(m);
}

void Interface::paramsSetMatrix(const char *name, double m[4][4], bool transpose)
{
	if(transpose)	(*cparams_)[std::string(name)] = Matrix4(m).transpose();
	else		(*cparams_)[std::string(name)] = Matrix4(m);
}

void Interface::paramsSetMemMatrix(const char *name, float *matrix, bool transpose)
{
	float mat[4][4];
	int i, j;
	for(i = 0; i < 4; i++)
		for(j = 0; j < 4; j++)
			mat[i][j] = *(matrix + i * 4 + j);
	paramsSetMatrix(name, mat, transpose);
}

void Interface::paramsSetMemMatrix(const char *name, double *matrix, bool transpose)
{
	double mat[4][4];
	int i, j;
	for(i = 0; i < 4; i++)
		for(j = 0; j < 4; j++)
			mat[i][j] = *(matrix + i * 4 + j);
	paramsSetMatrix(name, mat, transpose);
}

void Interface::setInputColorSpace(std::string color_space_string, float gamma_val)
{
	if(color_space_string == "sRGB") input_color_space_ = Srgb;
	else if(color_space_string == "XYZ") input_color_space_ = XyzD65;
	else if(color_space_string == "LinearRGB") input_color_space_ = LinearRgb;
	else if(color_space_string == "Raw_Manual_Gamma") input_color_space_ = RawManualGamma;
	else input_color_space_ = Srgb;

	input_gamma_ = gamma_val;
}

void Interface::paramsClearAll()
{
	params_->clear();
	eparams_->clear();
	cparams_ = params_;
}

void Interface::paramsStartList()
{
	if(!eparams_->empty()) eparams_->push_back(ParamMap());
	else Y_WARNING << "Interface: Appending to existing list!" << YENDL;
	cparams_ = &eparams_->back();
}
void Interface::paramsPushList()
{
	eparams_->push_back(ParamMap());
	cparams_ = &eparams_->back();
}

void Interface::paramsEndList()
{
	cparams_ = params_;
}

Light *Interface::createLight(const char *name)
{
	Light *light = env_->createLight(name, *params_);
	if(light) scene_->addLight(light);
	return light;
}

Texture 		*Interface::createTexture(const char *name) { return env_->createTexture(name, *params_); }
Material 	*Interface::createMaterial(const char *name) { return env_->createMaterial(name, *params_, *eparams_); }
Camera 		*Interface::createCamera(const char *name)
{
	Camera *camera = env_->createCamera(name, *params_);
	return camera;
}
Background 	*Interface::createBackground(const char *name) { return env_->createBackground(name, *params_); }
Integrator 	*Interface::createIntegrator(const char *name) { return env_->createIntegrator(name, *params_); }
ImageHandler *Interface::createImageHandler(const char *name, bool add_to_table) { return env_->createImageHandler(name, *params_); }

VolumeRegion 	*Interface::createVolumeRegion(const char *name)
{
	VolumeRegion *vr = env_->createVolumeRegion(name, *params_);
	if(!vr) return nullptr;
	scene_->addVolumeRegion(vr);
	return nullptr;
}

unsigned int Interface::createObject(const char *name)
{
	ObjectGeometric *object = env_->createObject(name, *params_);
	if(!object) return 0;
	ObjId_t id;
	if(scene_->addObject(object, id)) return id;
	return 0;
}

void Interface::abort()
{
	if(scene_) scene_->abort();
	session__.setStatusRenderAborted();
	Y_WARNING << "Interface: Render aborted by user." << YENDL;
}

bool Interface::getRenderedImage(int num_view, ColorOutput &output)
{
	if(!film_) return false;
	film_->flush(num_view, ImageFilm::Flags::All, &output);
	return true;
}

std::string Interface::getVersion() const
{
	return YAFARAY_BUILD_VERSION;
}

void Interface::printDebug(const std::string &msg)
{
	Y_DEBUG << msg << YENDL;
}

void Interface::printVerbose(const std::string &msg)
{
	Y_VERBOSE << msg << YENDL;
}

void Interface::printInfo(const std::string &msg)
{
	Y_INFO << msg << YENDL;
}

void Interface::printParams(const std::string &msg)
{
	Y_PARAMS << msg << YENDL;
}

void Interface::printWarning(const std::string &msg)
{
	Y_WARNING << msg << YENDL;
}

void Interface::printError(const std::string &msg)
{
	Y_ERROR << msg << YENDL;
}

void Interface::render(ColorOutput &output, ProgressBar *pb)
{
	if(! env_->setupScene(*scene_, *params_, output, pb)) return;
	session__.setStatusRenderStarted();
	scene_->render();
	film_ = scene_->getImageFilm();
	//delete film;
}

void Interface::setParamsBadgePosition(const std::string &badge_position)
{
	logger__.setParamsBadgePosition(badge_position);
}

bool Interface::getDrawParams()
{
	bool dp = false;

	dp = logger__.getUseParamsBadge();

	return dp;
}

void Interface::setOutput2(ColorOutput *out_2)
{
	if(env_) env_->setOutput2(out_2);
}

void Interface::setConsoleVerbosityLevel(const std::string &str_v_level)
{
	logger__.setConsoleMasterVerbosity(str_v_level);
}

void Interface::setLogVerbosityLevel(const std::string &str_v_level)
{
	logger__.setLogMasterVerbosity(str_v_level);
}

// export "factory"...

extern "C"
{
	Interface *getYafray__()
	{
		return new Interface();
	}
}

END_YAFARAY

