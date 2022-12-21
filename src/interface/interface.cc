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
#include "render/progress_bar.h"
#include <memory>
#include "common/version_build_info.h"
#include "common/logger.h"
#include "scene/scene.h"
#include "geometry/matrix.h"
#include "render/imagefilm.h"
#include "param/param.h"
#include "image/image_output.h"
#if defined(_WIN32)
#include <windows.h>
#endif

namespace yafaray {

Interface::Interface(const ::yafaray_LoggerCallback logger_callback, void *callback_data, ::yafaray_DisplayConsole logger_display_console) noexcept
{
	logger_ = std::make_unique<Logger>(logger_callback, callback_data, logger_display_console);
	params_ = std::make_unique<ParamMap>();
	cparams_ = params_.get();
#if defined(_WIN32)
	if(logger_display_console == YAFARAY_DISPLAY_CONSOLE_NORMAL) SetConsoleOutputCP(65001);	//set Windows Console to UTF8 so the image path can be displayed correctly
#endif
}

void Interface::setLoggingCallback(const ::yafaray_LoggerCallback logger_callback, void *callback_data) noexcept
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
	scene_->defineLayer(std::move(*params_));
}

bool Interface::initObject(size_t object_id, size_t material_id) noexcept { return scene_->initObject(object_id, material_id); }

int Interface::addVertex(size_t object_id, Point3f &&vertex, unsigned char time_step) noexcept { return scene_->addVertex(object_id, std::move(vertex), time_step); }

int Interface::addVertex(size_t object_id, Point3f &&vertex, Point3f &&orco, unsigned char time_step) noexcept
{
	return scene_->addVertex(object_id, std::move(vertex), std::move(orco), time_step);
}

void Interface::addVertexNormal(size_t object_id, Vec3f &&normal, unsigned char time_step) noexcept
{
	scene_->addVertexNormal(object_id, std::move(normal), time_step);
}

bool Interface::addFace(size_t object_id, const FaceIndices<int> &face_indices, size_t material_id) noexcept
{
	return scene_->addFace(object_id, face_indices, material_id);
}

int Interface::addUv(size_t object_id, Uv<float> &&uv) noexcept { return scene_->addUv(object_id, std::move(uv)); }

bool Interface::smoothVerticesNormals(size_t object_id, double angle) noexcept { return scene_->smoothVerticesNormals(object_id, angle); }

size_t Interface::createInstance() noexcept
{
	return scene_->createInstance();
}

bool Interface::addInstanceObject(size_t instance_id, size_t base_object_id) noexcept
{
	return scene_->addInstanceObject(instance_id, base_object_id);
}

bool Interface::addInstanceOfInstance(size_t instance_id, size_t base_instance_id) noexcept
{
	return scene_->addInstanceOfInstance(instance_id, base_instance_id);
}

bool Interface::addInstanceMatrix(size_t instance_id, Matrix4f &&obj_to_world, float time) noexcept
{
	return scene_->addInstanceMatrix(instance_id, std::move(obj_to_world), time);
}

void Interface::paramsSetVector(const std::string &name, Vec3f &&v) noexcept
{
	(*cparams_)[name] = Parameter{std::move(v)};
}

void Interface::paramsSetString(const std::string &name, std::string &&s) noexcept
{
	(*cparams_)[name] = Parameter(std::move(s));
}

void Interface::paramsSetBool(const std::string &name, bool b) noexcept
{
	(*cparams_)[std::string(name)] = b;
}

void Interface::paramsSetInt(const std::string &name, int i) noexcept
{
	(*cparams_)[std::string(name)] = i;
}

void Interface::paramsSetFloat(const std::string &name, double f) noexcept
{
	(*cparams_)[std::string(name)] = Parameter{f};
}

void Interface::paramsSetColor(const std::string &name, Rgba &&col) noexcept
{
	col.linearRgbFromColorSpace(input_color_space_, input_gamma_);
	(*cparams_)[std::string(name)] = std::move(col);
}

void Interface::paramsSetMatrix(const std::string &name, Matrix4f &&matrix, bool transpose) noexcept
{
	if(transpose)
	{
		(*cparams_)[std::string(name)] = std::move(Matrix4f{matrix}.transpose());
	}
	else (*cparams_)[std::string(name)] = std::move(matrix);
}

void Interface::setInputColorSpace(const std::string &color_space_string, float gamma_val) noexcept
{
	input_color_space_.initFromString(color_space_string);
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

std::pair<size_t, ResultFlags> Interface::getImageId(const std::string &name) noexcept
{
	if(scene_)
	{
		const auto [image, image_id, image_result]{scene_->getImage(name)};
		return {image_id, image_result};
	}
	else return {0, YAFARAY_RESULT_ERROR_NOT_FOUND};
}

std::pair<size_t, ResultFlags> Interface::getObjectId(const std::string &name) noexcept
{
	if(scene_)
	{
		const auto [object, object_id, object_result]{scene_->getObject(name)};
		return {object_id, object_result};
	}
	else return {0, YAFARAY_RESULT_ERROR_NOT_FOUND};
}

std::pair<size_t, ResultFlags> Interface::getMaterialId(const std::string &name) noexcept
{
	if(scene_) return scene_->getMaterial(name);
	else return {0, YAFARAY_RESULT_ERROR_NOT_FOUND};
}

std::pair<size_t, ParamResult> Interface::createObject(const std::string &name) noexcept { return scene_->createObject(std::move(name), std::move(*params_)); }
std::pair<size_t, ParamResult> Interface::createLight(const std::string &name) noexcept { return scene_->createLight(std::move(name), std::move(*params_)); }
std::pair<size_t, ParamResult> Interface::createTexture(const std::string &name) noexcept { return scene_->createTexture(std::move(name), std::move(*params_)); }
std::pair<size_t, ParamResult> Interface::createMaterial(const std::string &name) noexcept { return scene_->createMaterial(std::move(name), std::move(*params_), std::move(nodes_params_)); }
std::pair<size_t, ParamResult> Interface::createCamera(const std::string &name) noexcept { return scene_->createCamera(std::move(name), std::move(*params_)); }
ParamResult Interface::defineBackground() noexcept { return scene_->defineBackground(std::move(*params_)); }
ParamResult Interface::defineSurfaceIntegrator() noexcept { return scene_->defineSurfaceIntegrator(std::move(*params_)); }
ParamResult Interface::defineVolumeIntegrator() noexcept { return scene_->defineVolumeIntegrator(std::move(*params_)); }
std::pair<size_t, ParamResult> Interface::createVolumeRegion(const std::string &name) noexcept { return scene_->createVolumeRegion(std::move(name), std::move(*params_)); }
std::pair<size_t, ParamResult> Interface::createRenderView(const std::string &name) noexcept { return scene_->createRenderView(std::move(name), std::move(*params_)); }
std::pair<size_t, ParamResult> Interface::createImage(const std::string &name) noexcept { return scene_->createImage(std::move(name), std::move(*params_)); }
std::pair<size_t, ParamResult> Interface::createOutput(const std::string &name) noexcept { return scene_->createOutput(std::move(name), std::move(*params_)); }

void Interface::setRenderNotifyViewCallback(yafaray_RenderNotifyViewCallback callback, void *callback_data) noexcept
{
	if(scene_) scene_->setRenderNotifyViewCallback(callback, callback_data);
}

void Interface::setRenderNotifyLayerCallback(yafaray_RenderNotifyLayerCallback callback, void *callback_data) noexcept
{
	if(scene_) scene_->setRenderNotifyLayerCallback(callback, callback_data);
}

void Interface::setRenderPutPixelCallback(yafaray_RenderPutPixelCallback callback, void *callback_data) noexcept
{
	if(scene_) scene_->setRenderPutPixelCallback(callback, callback_data);
}

void Interface::setRenderHighlightPixelCallback(yafaray_RenderHighlightPixelCallback callback, void *callback_data) noexcept
{
	if(scene_) scene_->setRenderHighlightPixelCallback(callback, callback_data);
}

void Interface::setRenderFlushAreaCallback(yafaray_RenderFlushAreaCallback callback, void *callback_data) noexcept
{
	if(scene_) scene_->setRenderFlushAreaCallback(callback, callback_data);
}

void Interface::setRenderFlushCallback(yafaray_RenderFlushCallback callback, void *callback_data) noexcept
{
	if(scene_) scene_->setRenderFlushCallback(callback, callback_data);
}

void Interface::setRenderHighlightAreaCallback(yafaray_RenderHighlightAreaCallback callback, void *callback_data) noexcept
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
		for(const auto &view : views)
		{
			ss << view.name_ << '\n';
		}
		return ss.str();
	}
	else return "";
}

bool Interface::removeOutput(const std::string &name) noexcept
{
	return scene_->disableOutput(std::move(name));
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
	scene_->setupSceneRenderParams(*scene_, std::move(*params_));
}

void Interface::render(std::unique_ptr<ProgressBar> progress_bar) noexcept
{
	scene_->render(std::move(progress_bar));
}

void Interface::enablePrintDateTime(bool value) noexcept
{
	logger_->enablePrintDateTime(value);
}

void Interface::setConsoleVerbosityLevel(const ::yafaray_LogLevel &log_level) noexcept
{
	logger_->setConsoleMasterVerbosity(log_level);
}

void Interface::setLogVerbosityLevel(const ::yafaray_LogLevel &log_level) noexcept
{
	logger_->setLogMasterVerbosity(log_level);
}

void Interface::setConsoleLogColorsEnabled(bool console_log_colors_enabled) const noexcept
{
	logger_->setConsoleLogColorsEnabled(console_log_colors_enabled);
}

std::pair<Rgba, bool> Interface::getImageColor(size_t image_id, const Point2i &point) const noexcept
{
	return scene_->getImageColor(image_id, point);
}

bool Interface::setImageColor(size_t image_id, const Point2i &point, const Rgba &col) noexcept
{
	return scene_->setImageColor(image_id, point, col);
}

std::pair<Size2i, bool> Interface::getImageSize(size_t image_id) const noexcept
{
	return scene_->getImageSize(image_id);
}

} //namespace yafaray

