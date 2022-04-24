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

#include <memory>
#include "common/version_build_info.h"
#include "common/logger.h"
#include "scene/scene.h"
#include "geometry/matrix4.h"
#include "render/imagefilm.h"
#include "common/param.h"
#include "image/image_output.h"
#include "render/progress_bar.h"
#if defined(_WIN32)
#include <windows.h>
#endif

BEGIN_YAFARAY

Interface::Interface(const ::yafaray_LoggerCallback_t logger_callback, void *callback_data, ::yafaray_DisplayConsole_t logger_display_console)
{
	logger_ = std::make_unique<Logger>(logger_callback, callback_data, logger_display_console);
	params_ = std::make_unique<ParamMap>();
	cparams_ = params_.get();
#if defined(_WIN32)
	if(logger_display_console == YAFARAY_DISPLAY_CONSOLE_NORMAL) SetConsoleOutputCP(65001);	//set Windows Console to UTF8 so the image path can be displayed correctly
#endif
}

void Interface::setLoggingCallback(const ::yafaray_LoggerCallback_t logger_callback, void *callback_data)
{
	logger_->setCallback(logger_callback, callback_data);
}

void Interface::createScene() noexcept
{
	scene_ = std::make_unique<Scene>(*logger_);
	params_->clear();
}

Interface::~Interface() noexcept
{
	if(logger_->isVerbose()) logger_->logVerbose("Interface: Deleting scene...");
	logger_->logInfo("Interface: Done.");
}

void Interface::clearAll() noexcept
{
	if(logger_->isVerbose()) logger_->logVerbose("Interface: Cleaning scene...");
	scene_->clearAll();
	params_->clear();
	nodes_params_.clear();
	cparams_ = params_.get();
	if(logger_->isVerbose()) logger_->logVerbose("Interface: Cleanup done.");
}

void Interface::defineLayer() noexcept
{
	scene_->defineLayer(*params_);
}

bool Interface::startGeometry() noexcept { return scene_->startObjects(); }

bool Interface::endGeometry() noexcept { return scene_->endObjects(); }

unsigned int Interface::getNextFreeId() noexcept
{
	return scene_->getNextFreeId();
}

bool Interface::endObject() noexcept { return scene_->endObject(); }

int  Interface::addVertex(double x, double y, double z) noexcept { return scene_->addVertex({static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)}); }

int  Interface::addVertex(double x, double y, double z, double ox, double oy, double oz) noexcept
{
	return scene_->addVertex({static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)}, {static_cast<float>(ox), static_cast<float>(oy), static_cast<float>(oz)});
}

void Interface::addNormal(double x, double y, double z) noexcept
{
	scene_->addNormal({static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)});
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
	(*cparams_)[std::string(name)] = Parameter{Vec3{static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)}};
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

void Interface::paramsSetMatrix(const char *name, const Matrix4 &matrix, bool transpose) noexcept
{
	if(transpose)
	{
		(*cparams_)[std::string(name)] = Matrix4{matrix}.transpose();
	}
	else (*cparams_)[std::string(name)] = matrix;
}

void Interface::setInputColorSpace(const std::string &color_space_string, float gamma_val) noexcept
{
	input_color_space_ = Rgb::colorSpaceFromName(color_space_string);
	input_gamma_ = gamma_val;
}

void Interface::paramsClearAll() noexcept
{
	params_->clear();
	nodes_params_.clear();
	cparams_ = params_.get();
}

void Interface::paramsPushList() noexcept
{
	nodes_params_.emplace_back();
	cparams_ = &nodes_params_.back();
}

void Interface::paramsEndList() noexcept
{
	cparams_ = params_.get();
}

Object *Interface::createObject(const char *name) noexcept { return scene_->createObject(name, *params_); }
Light *Interface::createLight(const char *name) noexcept { return scene_->createLight(name, *params_); }
Texture *Interface::createTexture(const char *name) noexcept { return scene_->createTexture(name, *params_); }
const Material *Interface::createMaterial(const char *name) noexcept { return scene_->createMaterial(name, *params_, nodes_params_)->get(); }
const Camera * Interface::createCamera(const char *name) noexcept { return scene_->createCamera(name, *params_); }
const Background * Interface::createBackground(const char *name) noexcept { return scene_->createBackground(name, *params_); }
Integrator *Interface::createIntegrator(const char *name) noexcept { return scene_->createIntegrator(name, *params_); }
VolumeRegion *Interface::createVolumeRegion(const char *name) noexcept { return scene_->createVolumeRegion(name, *params_); }
RenderView *Interface::createRenderView(const char *name) noexcept { return scene_->createRenderView(name, *params_); }
Image *Interface::createImage(const char *name) noexcept { return scene_->createImage(name, *params_).get(); }

ImageOutput *Interface::createOutput(const char *name) noexcept
{
	return scene_->createOutput(name, *params_);
}

void Interface::setRenderNotifyViewCallback(yafaray_RenderNotifyViewCallback_t callback, void *callback_data) noexcept
{
	if(scene_) scene_->setRenderNotifyViewCallback(callback, callback_data);
}

void Interface::setRenderNotifyLayerCallback(yafaray_RenderNotifyLayerCallback_t callback, void *callback_data) noexcept
{
	if(scene_) scene_->setRenderNotifyLayerCallback(callback, callback_data);
}

void Interface::setRenderPutPixelCallback(yafaray_RenderPutPixelCallback_t callback, void *callback_data) noexcept
{
	if(scene_) scene_->setRenderPutPixelCallback(callback, callback_data);
}

void Interface::setRenderHighlightPixelCallback(yafaray_RenderHighlightPixelCallback_t callback, void *callback_data) noexcept
{
	if(scene_) scene_->setRenderHighlightPixelCallback(callback, callback_data);
}

void Interface::setRenderFlushAreaCallback(yafaray_RenderFlushAreaCallback_t callback, void *callback_data) noexcept
{
	if(scene_) scene_->setRenderFlushAreaCallback(callback, callback_data);
}

void Interface::setRenderFlushCallback(yafaray_RenderFlushCallback_t callback, void *callback_data) noexcept
{
	if(scene_) scene_->setRenderFlushCallback(callback, callback_data);
}

void Interface::setRenderHighlightAreaCallback(yafaray_RenderHighlightAreaCallback_t callback, void *callback_data) noexcept
{
	if(scene_) scene_->setRenderHighlightAreaCallback(callback, callback_data);
}

int Interface::getSceneFilmWidth() const noexcept
{
	const ImageFilm *image_film = scene_->getImageFilm();
	if(image_film) return image_film->getWidth();
	else return 0;
}

int Interface::getSceneFilmHeight() const noexcept
{
	const ImageFilm *image_film = scene_->getImageFilm();
	if(image_film) return image_film->getHeight();
	else return 0;
}

std::string Interface::printLayersTable() const noexcept
{
	if(scene_) return scene_->getLayers()->printExportedTable();
	else return "";
}

std::string Interface::printViewsTable() const noexcept
{
	if(scene_)
	{
		std::stringstream ss;
		const auto &views = scene_->getRenderViews();
		for(const auto &[view_name, view] : views)
		{
			ss << view_name << '\n';
		}
		return ss.str();
	}
	else return "";
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

void Interface::setCurrentMaterial(const std::unique_ptr<const Material> *material) noexcept
{
	if(scene_) scene_->setCurrentMaterial(material);
}

void Interface::setCurrentMaterial(const char *name) noexcept
{
	if(scene_) scene_->setCurrentMaterial(scene_->getMaterial(std::string(name)));
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

void Interface::render(std::shared_ptr<ProgressBar> progress_bar) noexcept
{
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

