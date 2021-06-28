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
#include "common/version_build_info.h"
#include "common/logger.h"
#include "scene/scene.h"
#include "geometry/matrix4.h"
#include "render/imagefilm.h"
#include "common/param.h"
#include "output/output.h"
#include "render/progress_bar.h"
#if defined(_WIN32)
#include <windows.h>
#endif

BEGIN_YAFARAY

Interface::Interface(const ::yafaray_LoggerCallback_t logger_callback, void *callback_user_data, ::yafaray_DisplayConsole_t logger_display_console)
{
	logger_ = std::unique_ptr<Logger>(new Logger(logger_callback, callback_user_data, logger_display_console));
	params_ = std::unique_ptr<ParamMap>(new ParamMap);
	eparams_ = std::unique_ptr<std::list<ParamMap>>(new std::list<ParamMap>);
	cparams_ = params_.get();
#if defined(_WIN32)
	if(logger_display_console == YAFARAY_DISPLAY_CONSOLE_NORMAL) SetConsoleOutputCP(65001);	//set Windows Console to UTF8 so the image path can be displayed correctly
#endif
}

void Interface::setLoggingCallback(const ::yafaray_LoggerCallback_t logger_callback, void *callback_user_data)
{
	logger_->setCallback(logger_callback, callback_user_data);
}

void Interface::createScene() noexcept
{
	scene_ = Scene::factory(*logger_, *params_);
	params_->clear();
}

Interface::~Interface() noexcept
{
	if(logger_->isVerbose()) logger_->logVerbose("Interface: Deleting scene...");
	logger_->logInfo("Interface: Done.");
	logger_->clearAll();
}

void Interface::clearAll() noexcept
{
	if(logger_->isVerbose()) logger_->logVerbose("Interface: Cleaning scene...");
	scene_->clearAll();
	params_->clear();
	eparams_->clear();
	cparams_ = params_.get();
	if(logger_->isVerbose()) logger_->logVerbose("Interface: Cleanup done.");
}

void Interface::defineLayer() noexcept
{
	scene_->defineLayer(*params_);
}

bool Interface::setupLayersParameters() noexcept
{
	scene_->setupLayersParameters(*params_);
	return true;
}

bool Interface::setInteractive(bool interactive) noexcept
{
	if(scene_)
	{
		scene_->getRenderControl().setInteractive(interactive);
		return true;
	}
	else return false;
}

bool Interface::startGeometry() noexcept { return scene_->startObjects(); }

bool Interface::endGeometry() noexcept { return scene_->endObjects(); }

unsigned int Interface::getNextFreeId() noexcept
{
	ObjId_t id;
	id = scene_->getNextFreeId();
	return id;
}

bool Interface::endObject() noexcept { return scene_->endObject(); }

int  Interface::addVertex(double x, double y, double z) noexcept { return scene_->addVertex(Point3(x, y, z)); }

int  Interface::addVertex(double x, double y, double z, double ox, double oy, double oz) noexcept
{
	return scene_->addVertex(Point3(x, y, z), Point3(ox, oy, oz));
}

void Interface::addNormal(double x, double y, double z) noexcept
{
	scene_->addNormal(Vec3(x, y, z));
}

bool Interface::addFace(int a, int b, int c) noexcept
{
	return scene_->addFace({a, b, c});
}

bool Interface::addFace(int a, int b, int c, int uv_a, int uv_b, int uv_c) noexcept
{
	return scene_->addFace({a, b, c}, {uv_a, uv_b, uv_c});
}

int Interface::addUv(float u, float v) noexcept { return scene_->addUv(u, v); }

bool Interface::smoothMesh(const char *name, double angle) noexcept { return scene_->smoothNormals(name, angle); }

bool Interface::addInstance(const char *base_object_name, const Matrix4 &obj_to_world) noexcept
{
	return scene_->addInstance(base_object_name, obj_to_world);
}

void Interface::paramsSetVector(const char *name, double x, double y, double z) noexcept
{
	(*cparams_)[std::string(name)] = Parameter(Vec3(x, y, z));
}

void Interface::paramsSetString(const char *name, const char *s) noexcept
{
	(*cparams_)[std::string(name)] = Parameter(std::string(s));
}

void Interface::paramsSetBool(const char *name, bool b) noexcept
{
	(*cparams_)[std::string(name)] = Parameter(b);
}

void Interface::paramsSetInt(const char *name, int i) noexcept
{
	(*cparams_)[std::string(name)] = Parameter(i);
}

void Interface::paramsSetFloat(const char *name, double f) noexcept
{
	(*cparams_)[std::string(name)] = Parameter(f);
}

void Interface::paramsSetColor(const char *name, float r, float g, float b, float a) noexcept
{
	Rgba col(r, g, b, a);
	col.linearRgbFromColorSpace(input_color_space_, input_gamma_);
	(*cparams_)[std::string(name)] = Parameter(col);
}

void Interface::paramsSetColor(const char *name, const float *rgb, bool with_alpha) noexcept
{
	Rgba col(rgb[0], rgb[1], rgb[2], (with_alpha ? rgb[3] : 1.f));
	col.linearRgbFromColorSpace(input_color_space_, input_gamma_);
	(*cparams_)[std::string(name)] = Parameter(col);
}

void Interface::paramsSetMatrix(const char *name, const float m[4][4], bool transpose) noexcept
{
	if(transpose)	(*cparams_)[std::string(name)] = Matrix4(m).transpose();
	else		(*cparams_)[std::string(name)] = Matrix4(m);
}

void Interface::paramsSetMatrix(const char *name, const double m[4][4], bool transpose) noexcept
{
	if(transpose)	(*cparams_)[std::string(name)] = Matrix4(m).transpose();
	else		(*cparams_)[std::string(name)] = Matrix4(m);
}

void Interface::paramsSetMemMatrix(const char *name, const float *matrix, bool transpose) noexcept
{
	float mat[4][4];
	int i, j;
	for(i = 0; i < 4; i++)
		for(j = 0; j < 4; j++)
			mat[i][j] = *(matrix + i * 4 + j);
	paramsSetMatrix(name, mat, transpose);
}

void Interface::paramsSetMemMatrix(const char *name, const double *matrix, bool transpose) noexcept
{
	double mat[4][4];
	int i, j;
	for(i = 0; i < 4; i++)
		for(j = 0; j < 4; j++)
			mat[i][j] = *(matrix + i * 4 + j);
	paramsSetMatrix(name, mat, transpose);
}

void Interface::setInputColorSpace(const std::string &color_space_string, float gamma_val) noexcept
{
	input_color_space_ = Rgb::colorSpaceFromName(color_space_string);
	input_gamma_ = gamma_val;
}

void Interface::paramsClearAll() noexcept
{
	params_->clear();
	eparams_->clear();
	cparams_ = params_.get();
}

void Interface::paramsPushList() noexcept
{
	eparams_->push_back(ParamMap());
	cparams_ = &eparams_->back();
}

void Interface::paramsEndList() noexcept
{
	cparams_ = params_.get();
}

Object *Interface::createObject(const char *name) noexcept { return scene_->createObject(name, *params_); }
Light *Interface::createLight(const char *name) noexcept { return scene_->createLight(name, *params_); }
Texture *Interface::createTexture(const char *name) noexcept { return scene_->createTexture(name, *params_); }
Material *Interface::createMaterial(const char *name) noexcept { return scene_->createMaterial(name, *params_, *eparams_); }
Camera *Interface::createCamera(const char *name) noexcept { return scene_->createCamera(name, *params_); }
Background *Interface::createBackground(const char *name) noexcept { return scene_->createBackground(name, *params_).get(); }
Integrator *Interface::createIntegrator(const char *name) noexcept { return scene_->createIntegrator(name, *params_); }
VolumeRegion *Interface::createVolumeRegion(const char *name) noexcept { return scene_->createVolumeRegion(name, *params_); }
RenderView *Interface::createRenderView(const char *name) noexcept { return scene_->createRenderView(name, *params_); }
Image *Interface::createImage(const char *name) noexcept { return scene_->createImage(name, *params_).get(); }

ColorOutput *Interface::createOutput(const char *name, bool auto_delete, void *callback_user_data, yafaray_OutputPutpixelCallback_t output_putpixel_callback, yafaray_OutputFlushAreaCallback_t output_flush_area_callback, yafaray_OutputFlushCallback_t output_flush_callback) noexcept
{
	return scene_->createOutput(name, *params_, auto_delete, callback_user_data, output_putpixel_callback, output_flush_area_callback, output_flush_callback);
}

ColorOutput *Interface::createOutput(const char *name, ColorOutput *output, bool auto_delete) noexcept
{
	auto output_unique = UniquePtr_t<ColorOutput>(output);
	return scene_->createOutput(name, std::move(output_unique), auto_delete);
}

void Interface::setOutputPutPixelCallback(const char *output_name, yafaray_OutputPutpixelCallback_t putpixel_callback, void *putpixel_callback_user_data) noexcept
{
	ColorOutput *output = scene_->getOutput(output_name);
	if(output) output->setPutPixelCallback(putpixel_callback_user_data, putpixel_callback);
}

void Interface::setOutputFlushAreaCallback(const char *output_name, yafaray_OutputFlushAreaCallback_t flush_area_callback, void *flush_area_callback_user_data) noexcept
{
	ColorOutput *output = scene_->getOutput(output_name);
	if(output) output->setFlushAreaCallback(flush_area_callback_user_data, flush_area_callback);
}

void Interface::setOutputFlushCallback(const char *output_name, yafaray_OutputFlushCallback_t flush_callback, void *flush_callback_user_data) noexcept
{
	ColorOutput *output = scene_->getOutput(output_name);
	if(output) output->setFlushCallback(flush_callback_user_data, flush_callback);
}

void Interface::setOutputHighlightCallback(const char *output_name, yafaray_OutputHighlightCallback_t highlight_callback, void *highlight_callback_user_data) noexcept
{
	ColorOutput *output = scene_->getOutput(output_name);
	if(output) output->setHighlightCallback(highlight_callback_user_data, highlight_callback);
}

int Interface::getOutputWidth(const char *output_name) const noexcept
{
	const ColorOutput *output = scene_->getOutput(output_name);
	if(output) return output->getWidth();
	else return 0;
}

int Interface::getOutputHeight(const char *output_name) const noexcept
{
	ColorOutput *output = scene_->getOutput(output_name);
	if(output) return output->getHeight();
	else return 0;
}

bool Interface::removeOutput(const char *name) noexcept
{
	return scene_->removeOutput(name);
}

void Interface::clearOutputs() noexcept
{
	return scene_->clearOutputs();
}

void Interface::cancel() noexcept
{
	if(scene_) scene_->getRenderControl().setCanceled();
	logger_->logWarning("Interface: Render canceled by user.");
}

void Interface::setCurrentMaterial(const Material *material) noexcept
{
	if(scene_) scene_->setCurrentMaterial(material);
}

void Interface::setCurrentMaterial(const char *name) noexcept
{
	if(scene_) scene_->setCurrentMaterial(scene_->getMaterial(std::string(name)));
}

const Material *Interface::getCurrentMaterial() const noexcept
{
	if(scene_) return scene_->getCurrentMaterial();
	else return nullptr;
}

void Interface::printDebug(const std::string &msg) const noexcept
{
	if(logger_->isDebug())logger_->logDebug(msg);
}

void Interface::printVerbose(const std::string &msg) const noexcept
{
	if(logger_->isVerbose()) logger_->logVerbose(msg);
}

void Interface::printInfo(const std::string &msg) const noexcept
{
	logger_->logInfo(msg);
}

void Interface::printParams(const std::string &msg) const noexcept
{
	logger_->logParams(msg);
}

void Interface::printWarning(const std::string &msg) const noexcept
{
	logger_->logWarning(msg);
}

void Interface::printError(const std::string &msg) const noexcept
{
	logger_->logError(msg);
}

void Interface::setupRender() noexcept
{
	scene_->setupSceneRenderParams(*scene_, *params_);
}

void Interface::render(ProgressBar *pb, bool auto_delete_progress_bar, ::yafaray_DisplayConsole_t progress_bar_display_console) noexcept
{
	std::shared_ptr<ProgressBar> progress_bar(pb, CustomDeleter<ProgressBar>());
	progress_bar->setAutoDelete(auto_delete_progress_bar);
	if(!scene_->setupSceneProgressBar(*scene_, progress_bar)) return;
	scene_->render();
}

void Interface::enablePrintDateTime(bool value) noexcept
{
	logger_->enablePrintDateTime(value);
}

void Interface::setConsoleVerbosityLevel(const ::yafaray_LogLevel_t &log_level) noexcept
{
	logger_->setConsoleMasterVerbosity(log_level);
}

void Interface::setLogVerbosityLevel(const ::yafaray_LogLevel_t &log_level) noexcept
{
	logger_->setLogMasterVerbosity(log_level);
}

void Interface::setConsoleLogColorsEnabled(bool console_log_colors_enabled) const noexcept
{
	logger_->setConsoleLogColorsEnabled(console_log_colors_enabled);
}

END_YAFARAY

