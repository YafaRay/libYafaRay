#pragma once
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

#ifndef YAFARAY_INTERFACE_H
#define YAFARAY_INTERFACE_H

#include "common/yafaray_common.h"
#include "public_api/yafaray_c_api.h"
#include "color/color.h"
#include <list>
#include <vector>
#include <string>
#include <memory>

BEGIN_YAFARAY

class Light;
class Texture;
class Material;
class Camera;
class Background;
class Integrator;
class VolumeRegion;
class Scene;
class ImageOutput;
class RenderView;
class ParamMap;
class ImageFilm;
class Format;
class ProgressBar;
class Matrix4;
class Object;
class Image;
class Logger;

class Interface
{
	public:
		Interface(const ::yafaray_LoggerCallback_t logger_callback = nullptr, void *callback_data = nullptr, ::yafaray_DisplayConsole_t logger_display_console = YAFARAY_DISPLAY_CONSOLE_NORMAL);
		virtual ~Interface() noexcept;
		void setLoggingCallback(const ::yafaray_LoggerCallback_t logger_callback, void *callback_data);
		virtual void createScene() noexcept;
		virtual int getSceneFilmWidth() const noexcept;
		virtual int getSceneFilmHeight() const noexcept;
		std::string printLayersTable() const noexcept;
		std::string printViewsTable() const noexcept;
		virtual bool startGeometry() noexcept; //!< call before creating geometry; only meshes and vmaps can be created in this state
		virtual bool endGeometry() noexcept; //!< call after creating geometry;
		virtual unsigned int getNextFreeId() noexcept;
		virtual bool endObject() noexcept; //!< end current mesh and return to geometry state
		virtual int  addVertex(double x, double y, double z) noexcept; //!< add vertex to mesh; returns index to be used for addTriangle
		virtual int  addVertex(double x, double y, double z, double ox, double oy, double oz) noexcept; //!< add vertex with Orco to mesh; returns index to be used for addTriangle
		virtual void addNormal(double nx, double ny, double nz) noexcept; //!< add vertex normal to mesh; the vertex that will be attached to is the last one inserted by addVertex method
		virtual bool addFace(int a, int b, int c) noexcept; //!< add a triangle given vertex indices and material pointer
		virtual bool addFace(int a, int b, int c, int uv_a, int uv_b, int uv_c) noexcept; //!< add a triangle given vertex and uv indices and material pointer
		virtual int  addUv(float u, float v) noexcept; //!< add a UV coordinate pair; returns index to be used for addTriangle
		virtual bool smoothMesh(const char *name, double angle) noexcept; //!< smooth vertex normals of mesh with given ID and angle (in degrees)
		virtual bool addInstance(const char *base_object_name, const Matrix4 &obj_to_world) noexcept;
		virtual void paramsSetVector(const char *name, double x, double y, double z) noexcept;
		virtual void paramsSetString(const char *name, const char *s) noexcept;
		virtual void paramsSetBool(const char *name, bool b) noexcept;
		virtual void paramsSetInt(const char *name, int i) noexcept;
		virtual void paramsSetFloat(const char *name, double f) noexcept;
		virtual void paramsSetColor(const char *name, float r, float g, float b, float a = 1.f) noexcept;
		virtual void paramsSetMatrix(const char *name, const Matrix4 &matrix, bool transpose = false) noexcept;
		virtual void paramsClearAll() noexcept; 	//!< clear the paramMap and paramList
		virtual void paramsPushList() noexcept; 	//!< push new list item in paramList (e.g. new shader node description)
		virtual void paramsEndList() noexcept; 	//!< revert to writing to normal paramMap
		virtual void setCurrentMaterial(const char *name) noexcept;
		virtual Object *createObject(const char *name) noexcept;
		virtual Light *createLight(const char *name) noexcept;
		virtual Texture *createTexture(const char *name) noexcept;
		virtual Material *createMaterial(const char *name) noexcept;
		virtual Camera *createCamera(const char *name) noexcept;
		virtual Background *createBackground(const char *name) noexcept;
		virtual Integrator *createIntegrator(const char *name) noexcept;
		virtual VolumeRegion *createVolumeRegion(const char *name) noexcept;
		virtual RenderView *createRenderView(const char *name) noexcept;
		virtual Image *createImage(const char *name) noexcept;
		virtual ImageOutput *createOutput(const char *name) noexcept;
		void setRenderNotifyViewCallback(yafaray_RenderNotifyViewCallback_t callback, void *callback_data) noexcept;
		void setRenderNotifyLayerCallback(yafaray_RenderNotifyLayerCallback_t callback, void *callback_data) noexcept;
		void setRenderPutPixelCallback(yafaray_RenderPutPixelCallback_t callback, void *callback_data) noexcept;
		void setRenderHighlightPixelCallback(yafaray_RenderHighlightPixelCallback_t callback, void *callback_data) noexcept;
		void setRenderFlushAreaCallback(yafaray_RenderFlushAreaCallback_t callback, void *callback_data) noexcept;
		void setRenderFlushCallback(yafaray_RenderFlushCallback_t callback, void *callback_data) noexcept;
		void setRenderHighlightAreaCallback(yafaray_RenderHighlightAreaCallback_t callback, void *callback_data) noexcept;
		bool removeOutput(const char *name) noexcept;
		virtual void clearOutputs() noexcept;
		virtual void clearAll() noexcept;
		virtual void setupRender() noexcept;
		virtual void render(std::shared_ptr<ProgressBar> progress_bar) noexcept; //!< render the scene...
		virtual void defineLayer() noexcept;
		virtual void cancel() noexcept;

		void enablePrintDateTime(bool value) noexcept;
		void setConsoleVerbosityLevel(const ::yafaray_LogLevel_t &log_level) noexcept;
		void setLogVerbosityLevel(const ::yafaray_LogLevel_t &log_level) noexcept;
		void printDebug(const std::string &msg) const noexcept;
		void printVerbose(const std::string &msg) const noexcept;
		void printInfo(const std::string &msg) const noexcept;
		void printParams(const std::string &msg) const noexcept;
		void printWarning(const std::string &msg) const noexcept;
		void printError(const std::string &msg) const noexcept;
		void setConsoleLogColorsEnabled(bool console_log_colors_enabled) const noexcept;
		void setInputColorSpace(const std::string &color_space_string, float gamma_val) noexcept;

	protected:
		virtual void setCurrentMaterial(const Material *material) noexcept;
		std::unique_ptr<Logger> logger_;
		std::unique_ptr<ParamMap> params_;
		std::list<ParamMap> nodes_params_; //! for materials that need to define a whole shader tree etc.
		ParamMap *cparams_ = nullptr; //! just a pointer to the current paramMap, either params or a nodes_params element
		std::unique_ptr<Scene> scene_;
		float input_gamma_ = 1.f;
		ColorSpace input_color_space_ = RawManualGamma;
};

END_YAFARAY

#endif // YAFARAY_INTERFACE_H
