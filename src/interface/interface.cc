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
#include "common/logger.h"
#include "common/session.h"
#include "scene/scene.h"
#include "geometry/matrix4.h"
#include "render/imagefilm.h"
#include "common/param.h"
#include <signal.h>

#ifdef WIN32
#include <windows.h>
#endif

BEGIN_YAFARAY

RenderControl *global_render_control_global = nullptr;

#ifdef WIN32
BOOL WINAPI ctrlCHandler_global(DWORD signal)
{
	Y_WARNING << "Interface: Render aborted by user." << YENDL;
	if(global_render_control_global)
	{
		global_render_control_global->setAborted();
		return TRUE;
	}
	else exit(1);
}
#else
void ctrlCHandler_global(int signal)
{
	Y_WARNING << "Interface: Render aborted by user." << YENDL;
	if(global_render_control_global) global_render_control_global->setAborted();
	else exit(1);
}
#endif

Interface::Interface()
{
	//handle CTRL+C events
#ifdef WIN32
	SetConsoleCtrlHandler(ctrlCHandler_global, true);
#else
	struct ::sigaction signal_handler;
	signal_handler.sa_handler = ctrlCHandler_global;
	sigemptyset(&signal_handler.sa_mask);
	signal_handler.sa_flags = 0;
	sigaction(SIGINT, &signal_handler, nullptr);
#endif

	params_ = new ParamMap;
	eparams_ = new std::list<ParamMap>;
	cparams_ = params_;
}

void Interface::createScene()
{
	scene_ = Scene::factory(*params_);
	params_->clear();
	global_render_control_global = &scene_->getRenderControl();	//for the CTRL+C handler
}

Interface::~Interface()
{
	Y_VERBOSE << "Interface: Deleting scene..." << YENDL;
	if(scene_) delete scene_;
	Y_INFO << "Interface: Done." << YENDL;
	delete params_;
	delete eparams_;
	logger_global.clearAll();
}

void Interface::clearAll()
{
	Y_VERBOSE << "Interface: Cleaning scene..." << YENDL;
	scene_->clearAll();
	params_->clear();
	eparams_->clear();
	cparams_ = params_;
	Y_VERBOSE << "Interface: Cleanup done." << YENDL;
}

void Interface::defineLayer(const std::string &layer_type_name, const std::string &exported_image_type_name, const std::string &exported_image_name, const std::string &image_type_name)
{
	scene_->defineLayer(layer_type_name, image_type_name, exported_image_type_name, exported_image_name);
}

bool Interface::setupLayersParameters()
{
	scene_->setupLayersParameters(*params_);
	return true;
}

bool Interface::setInteractive(bool interactive)
{
	session_global.setInteractive(interactive);
	return true;
}

bool Interface::startGeometry() { return scene_->startObjects(); }

bool Interface::endGeometry() { return scene_->endObjects(); }

unsigned int Interface::getNextFreeId()
{
	ObjId_t id;
	id = scene_->getNextFreeId();
	return id;
}

bool Interface::endObject() { return scene_->endObject(); }

int  Interface::addVertex(double x, double y, double z) { return scene_->addVertex(Point3(x, y, z)); }

int  Interface::addVertex(double x, double y, double z, double ox, double oy, double oz)
{
	return scene_->addVertex(Point3(x, y, z), Point3(ox, oy, oz));
}

void Interface::addNormal(double x, double y, double z)
{
	scene_->addNormal(Vec3(x, y, z));
}

bool Interface::addFace(int a, int b, int c)
{
	return scene_->addFace({a, b, c});
}

bool Interface::addFace(int a, int b, int c, int uv_a, int uv_b, int uv_c)
{
	return scene_->addFace({a, b, c}, {uv_a, uv_b, uv_c});
}

int Interface::addUv(float u, float v) { return scene_->addUv(u, v); }

bool Interface::smoothMesh(const char *name, double angle) { return scene_->smoothNormals(name, angle); }

bool Interface::addInstance(const char *base_object_name, const Matrix4 &obj_to_world)
{
	return scene_->addInstance(base_object_name, obj_to_world);
}

void Interface::paramsSetVector(const char *name, double x, double y, double z)
{
	(*cparams_)[std::string(name)] = Parameter(Vec3(x, y, z));
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

void Interface::setInputColorSpace(const std::string &color_space_string, float gamma_val)
{
	input_color_space_ = Rgb::colorSpaceFromName(color_space_string);
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

Object *Interface::createObject(const char *name) { return scene_->createObject(name, *params_); }
Light *Interface::createLight(const char *name) { return scene_->createLight(name, *params_); }
Texture *Interface::createTexture(const char *name) { return scene_->createTexture(name, *params_); }
Material *Interface::createMaterial(const char *name) { return scene_->createMaterial(name, *params_, *eparams_); }
Camera *Interface::createCamera(const char *name) { return scene_->createCamera(name, *params_); }
Background *Interface::createBackground(const char *name) { return scene_->createBackground(name, *params_); }
Integrator *Interface::createIntegrator(const char *name) { return scene_->createIntegrator(name, *params_); }
VolumeRegion *Interface::createVolumeRegion(const char *name) { return scene_->createVolumeRegion(name, *params_); }
RenderView *Interface::createRenderView(const char *name) { return scene_->createRenderView(name, *params_); }
ColorOutput *Interface::createOutput(const char *name) { return scene_->createOutput(name, *params_); } //We do *NOT* have ownership of the outputs, do *NOT* delete them!
ColorOutput *Interface::createOutput(const std::string &name, ColorOutput *output) { return scene_->createOutput(name, output); } //We do *NOT* have ownership of the outputs, do *NOT* delete them!
bool Interface::removeOutput(const std::string &name) { return scene_->removeOutput(name); } //Caution: this will delete outputs, only to be called by the client on demand, we do *NOT* have ownership of the outputs
void Interface::clearOutputs() { return scene_->clearOutputs(); } //Caution: this will delete outputs, only to be called by the client on demand, we do *NOT* have ownership of the outputs

void Interface::abort()
{
	if(scene_) scene_->getRenderControl().setAborted();
	Y_WARNING << "Interface: Render aborted by user." << YENDL;
}

void Interface::setCurrentMaterial(const Material *material)
{
	if(scene_) scene_->setCurrentMaterial(material);
}

void Interface::setCurrentMaterial(const char *name)
{
	if(scene_) scene_->setCurrentMaterial(scene_->getMaterial(std::string(name)));
}

const Material *Interface::getCurrentMaterial() const
{
	if(scene_) return scene_->getCurrentMaterial();
	else return nullptr;
}

std::string Interface::getVersion() const
{
	return YAFARAY_BUILD_VERSION;
}

void Interface::printDebug(const std::string &msg) const
{
	Y_DEBUG << msg << YENDL;
}

void Interface::printVerbose(const std::string &msg) const
{
	Y_VERBOSE << msg << YENDL;
}

void Interface::printInfo(const std::string &msg) const
{
	Y_INFO << msg << YENDL;
}

void Interface::printParams(const std::string &msg) const
{
	Y_PARAMS << msg << YENDL;
}

void Interface::printWarning(const std::string &msg) const
{
	Y_WARNING << msg << YENDL;
}

void Interface::printError(const std::string &msg) const
{
	Y_ERROR << msg << YENDL;
}

void Interface::render(ProgressBar *pb)
{
	if(! scene_->setupScene(*scene_, *params_, pb)) return;
	scene_->render();
}

void Interface::enablePrintDateTime(bool value)
{
	logger_global.enablePrintDateTime(value);
}

void Interface::setConsoleVerbosityLevel(const std::string &str_v_level)
{
	logger_global.setConsoleMasterVerbosity(str_v_level);
}

void Interface::setLogVerbosityLevel(const std::string &str_v_level)
{
	logger_global.setLogMasterVerbosity(str_v_level);
}

// export "factory"...

extern "C"
{
	Interface *getYafray_global()
	{
		return new Interface();
	}
}

END_YAFARAY

