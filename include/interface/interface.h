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

#include "color/color.h"
#include "common/yafaray_common.h"
#include "geometry/uv.h"
#include "public_api/yafaray_c_api.h"
#include <list>
#include <memory>
#include <string>
#include <vector>

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
		explicit Interface( ::yafaray_LoggerCallback_t logger_callback = nullptr, void *callback_data = nullptr, ::yafaray_DisplayConsole_t logger_display_console = YAFARAY_DISPLAY_CONSOLE_NORMAL);
		virtual ~Interface() noexcept;
		void setLoggingCallback( ::yafaray_LoggerCallback_t logger_callback, void *callback_data);
		virtual void createScene() noexcept;
		virtual int getSceneFilmWidth() const noexcept;
		virtual int getSceneFilmHeight() const noexcept;
		std::string printLayersTable() const noexcept;
		std::string printViewsTable() const noexcept;
		virtual bool startGeometry() noexcept; //!< call before creating geometry; only meshes and vmaps can be created in this state
		virtual bool endGeometry() noexcept; //!< call after creating geometry;
		virtual unsigned int getNextFreeId() noexcept;
		virtual bool endObject() noexcept; //!< end current mesh and return to geometry state
		virtual int addVertex(double x, double y, double z, size_t time_step) noexcept; //!< add vertex to mesh; returns index to be used for addTriangle/addQuad
		virtual int addVertex(double x, double y, double z, double ox, double oy, double oz, size_t time_step) noexcept; //!< add vertex with Orco to mesh; returns index to be used for addTriangle/addQuad
		virtual void addVertexNormal(double x, double y, double z, size_t time_step) noexcept; //!< add vertex normal to mesh; the vertex that will be attached to is the last one inserted by addVertex method
		virtual bool addFace(std::vector<int> &&vertices, std::vector<int> &&uv_indices) noexcept; //!< add a mesh face given vertex indices and optionally uv_indices
		virtual int addUv(Uv &&uv) noexcept; //!< add a UV coordinate pair; returns index to be used for addTriangle/addQuad
		virtual bool smoothVerticesNormals(const char *name, double angle) noexcept; //!< smooth vertex normals of mesh with given ID and angle (in degrees)
		virtual size_t createInstance() noexcept;
		virtual bool addInstanceObject(size_t instance_id, const char *base_object_name) noexcept;
		virtual bool addInstanceOfInstance(size_t instance_id, size_t base_instance_id) noexcept;
		virtual bool addInstanceMatrix(size_t instance_id, Matrix4 &&obj_to_world, float time) noexcept;
		virtual void paramsSetVector(const char *name, double x, double y, double z) noexcept;
		virtual void paramsSetString(const char *name, const char *s) noexcept;
		virtual void paramsSetBool(const char *name, bool b) noexcept;
		virtual void paramsSetInt(const char *name, int i) noexcept;
		virtual void paramsSetFloat(const char *name, double f) noexcept;
		virtual void paramsSetColor(const char *name, float r, float g, float b, float a) noexcept;
		void paramsSetColor(const char *name, float r, float g, float b) noexcept { paramsSetColor(name, r, g, b, 1.f); };
		virtual void paramsSetMatrix(const char *name, Matrix4 &&matrix, bool transpose) noexcept;
		void paramsSetMatrix(const char *name, Matrix4 &&matrix) noexcept { paramsSetMatrix(name, std::move(matrix), false); };
		virtual void paramsClearAll() noexcept; 	//!< clear the paramMap and paramList
		virtual void paramsPushList() noexcept; 	//!< push new list item in paramList (e.g. new shader node description)
		virtual void paramsEndList() noexcept; 	//!< revert to writing to normal paramMap
		virtual void setCurrentMaterial(const char *name) noexcept;
		virtual Object *createObject(const char *name) noexcept;
		virtual Light *createLight(const char *name) noexcept;
		virtual Texture *createTexture(const char *name) noexcept;
		virtual const Material *createMaterial(const char *name) noexcept;
		virtual const Camera * createCamera(const char *name) noexcept;
		virtual const Background * createBackground(const char *name) noexcept;
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
		virtual void render(const std::shared_ptr<ProgressBar> &progress_bar) noexcept; //!< render the scene...
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
		virtual void setCurrentMaterial(const std::unique_ptr<const Material> *material) noexcept;
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
