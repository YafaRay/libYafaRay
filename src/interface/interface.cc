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
#include "output/output.h"
#include "render/progress_bar.h"
#include <signal.h>

BEGIN_YAFARAY

Interface::Interface(const ::yafaray4_LoggerCallback_t logger_callback, void *callback_user_data, ::yafaray4_DisplayConsole_t logger_display_console)
{
	logger_ = std::unique_ptr<Logger>(new Logger(logger_callback, callback_user_data, logger_display_console));
	params_ = std::unique_ptr<ParamMap>(new ParamMap);
	eparams_ = std::unique_ptr<std::list<ParamMap>>(new std::list<ParamMap>);
	cparams_ = params_.get();
}

void Interface::createScene()
{
	scene_ = Scene::factory(*logger_, *params_);
	params_->clear();
}

Interface::~Interface()
{
	if(logger_->isVerbose()) logger_->logVerbose("Interface: Deleting scene...");
	logger_->logInfo("Interface: Done.");
	logger_->clearAll();
}

void Interface::clearAll()
{
	if(logger_->isVerbose()) logger_->logVerbose("Interface: Cleaning scene...");
	scene_->clearAll();
	params_->clear();
	eparams_->clear();
	cparams_ = params_.get();
	if(logger_->isVerbose()) logger_->logVerbose("Interface: Cleanup done.");
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

void Interface::paramsSetColor(const char *name, const float *rgb, bool with_alpha)
{
	Rgba col(rgb[0], rgb[1], rgb[2], (with_alpha ? rgb[3] : 1.f));
	col.linearRgbFromColorSpace(input_color_space_, input_gamma_);
	(*cparams_)[std::string(name)] = Parameter(col);
}

void Interface::paramsSetMatrix(const char *name, const float m[4][4], bool transpose)
{
	if(transpose)	(*cparams_)[std::string(name)] = Matrix4(m).transpose();
	else		(*cparams_)[std::string(name)] = Matrix4(m);
}

void Interface::paramsSetMatrix(const char *name, const double m[4][4], bool transpose)
{
	if(transpose)	(*cparams_)[std::string(name)] = Matrix4(m).transpose();
	else		(*cparams_)[std::string(name)] = Matrix4(m);
}

void Interface::paramsSetMemMatrix(const char *name, const float *matrix, bool transpose)
{
	float mat[4][4];
	int i, j;
	for(i = 0; i < 4; i++)
		for(j = 0; j < 4; j++)
			mat[i][j] = *(matrix + i * 4 + j);
	paramsSetMatrix(name, mat, transpose);
}

void Interface::paramsSetMemMatrix(const char *name, const double *matrix, bool transpose)
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
	cparams_ = params_.get();
}

void Interface::paramsPushList()
{
	eparams_->push_back(ParamMap());
	cparams_ = &eparams_->back();
}

void Interface::paramsEndList()
{
	cparams_ = params_.get();
}

Object *Interface::createObject(const char *name) { return scene_->createObject(name, *params_); }
Light *Interface::createLight(const char *name) { return scene_->createLight(name, *params_); }
Texture *Interface::createTexture(const char *name) { return scene_->createTexture(name, *params_); }
Material *Interface::createMaterial(const char *name) { return scene_->createMaterial(name, *params_, *eparams_); }
Camera *Interface::createCamera(const char *name) { return scene_->createCamera(name, *params_); }
Background *Interface::createBackground(const char *name) { return scene_->createBackground(name, *params_).get(); }
Integrator *Interface::createIntegrator(const char *name) { return scene_->createIntegrator(name, *params_); }
VolumeRegion *Interface::createVolumeRegion(const char *name) { return scene_->createVolumeRegion(name, *params_); }
RenderView *Interface::createRenderView(const char *name) { return scene_->createRenderView(name, *params_); }
Image *Interface::createImage(const char *name) { return scene_->createImage(name, *params_).get(); }

ColorOutput *Interface::createOutput(const char *name, bool auto_delete, void *callback_user_data, yafaray4_OutputPutpixelCallback_t output_putpixel_callback, yafaray4_OutputFlushAreaCallback_t output_flush_area_callback, yafaray4_OutputFlushCallback_t output_flush_callback)
{
	return scene_->createOutput(name, *params_, auto_delete, callback_user_data, output_putpixel_callback, output_flush_area_callback, output_flush_callback);
}

ColorOutput *Interface::createOutput(const char *name, ColorOutput *output, bool auto_delete)
{
	auto output_unique = UniquePtr_t<ColorOutput>(output);
	return scene_->createOutput(name, std::move(output_unique), auto_delete);
}

bool Interface::removeOutput(const char *name)
{
	return scene_->removeOutput(name);
}

void Interface::clearOutputs()
{
	return scene_->clearOutputs();
}

void Interface::cancel()
{
	if(scene_) scene_->getRenderControl().setCanceled();
	logger_->logWarning("Interface: Render canceled by user.");
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
	if(logger_->isDebug())logger_->logDebug(msg);
}

void Interface::printVerbose(const std::string &msg) const
{
	if(logger_->isVerbose()) logger_->logVerbose(msg);
}

void Interface::printInfo(const std::string &msg) const
{
	logger_->logInfo(msg);
}

void Interface::printParams(const std::string &msg) const
{
	logger_->logParams(msg);
}

void Interface::printWarning(const std::string &msg) const
{
	logger_->logWarning(msg);
}

void Interface::printError(const std::string &msg) const
{
	logger_->logError(msg);
}

void Interface::render(ProgressBar *pb, bool auto_delete_progress_bar, ::yafaray4_DisplayConsole_t progress_bar_display_console)
{
	std::shared_ptr<ProgressBar> progress_bar(pb, CustomDeleter<ProgressBar>());
	progress_bar->setAutoDelete(auto_delete_progress_bar);
	if(!scene_->setupScene(*scene_, *params_, progress_bar)) return;
	scene_->render();
}

void Interface::enablePrintDateTime(bool value)
{
	logger_->enablePrintDateTime(value);
}

void Interface::setConsoleVerbosityLevel(const ::yafaray4_LogLevel_t &log_level)
{
	logger_->setConsoleMasterVerbosity(log_level);
}

void Interface::setLogVerbosityLevel(const ::yafaray4_LogLevel_t &log_level)
{
	logger_->setLogMasterVerbosity(log_level);
}

void Interface::setConsoleLogColorsEnabled(bool console_log_colors_enabled) const
{
	logger_->setConsoleLogColorsEnabled(console_log_colors_enabled);
}

END_YAFARAY

