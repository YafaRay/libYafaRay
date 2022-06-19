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
#include "geometry/uv.h"
#include "geometry/vector.h"
#include "public_api/yafaray_c_api.h"
#include <list>
#include <memory>
#include <string>
#include <vector>

namespace yafaray {

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
		virtual int addVertex(Point3 &&vertex, int time_step) noexcept; //!< add vertex to mesh; returns index to be used for addTriangle/addQuad
		virtual int addVertex(Point3 &&vertex, Point3 &&orco, int time_step) noexcept; //!< add vertex with Orco to mesh; returns index to be used for addTriangle/addQuad
		virtual void addVertexNormal(Vec3 &&normal, int time_step) noexcept; //!< add vertex normal to mesh; the vertex that will be attached to is the last one inserted by addVertex method
		virtual bool addFace(std::vector<int> &&vertices, std::vector<int> &&uv_indices) noexcept; //!< add a mesh face given vertex indices and optionally uv_indices
		virtual int addUv(Uv<float> &&uv) noexcept; //!< add a UV coordinate pair; returns index to be used for addTriangle/addQuad
		virtual bool smoothVerticesNormals(std::string &&name, double angle) noexcept; //!< smooth vertex normals of mesh with given ID and angle (in degrees)
		virtual int createInstance() noexcept;
		virtual bool addInstanceObject(int instance_id, std::string &&base_object_name) noexcept;
		virtual bool addInstanceOfInstance(int instance_id, size_t base_instance_id) noexcept;
		virtual bool addInstanceMatrix(int instance_id, Matrix4 &&obj_to_world, float time) noexcept;
		virtual void paramsSetVector(std::string &&name, Vec3 &&v) noexcept;
		virtual void paramsSetString(std::string &&name, std::string &&s) noexcept;
		virtual void paramsSetBool(std::string &&name, bool b) noexcept;
		virtual void paramsSetInt(std::string &&name, int i) noexcept;
		virtual void paramsSetFloat(std::string &&name, double f) noexcept;
		virtual void paramsSetColor(std::string &&name, Rgba &&col) noexcept;
		void paramsSetColor(std::string &&name, Rgb &&col) noexcept { paramsSetColor(std::move(name), std::move(Rgba{col})); };
		virtual void paramsSetMatrix(std::string &&name, Matrix4 &&matrix, bool transpose) noexcept;
		void paramsSetMatrix(std::string &&name, Matrix4 &&matrix) noexcept { paramsSetMatrix(std::move(name), std::move(matrix), false); };
		virtual void paramsClearAll() noexcept; 	//!< clear the paramMap and paramList
		virtual void paramsPushList() noexcept; 	//!< push new list item in paramList (e.g. new shader node description)
		virtual void paramsEndList() noexcept; 	//!< revert to writing to normal paramMap
		virtual void setCurrentMaterial(std::string &&name) noexcept;
		virtual Object *createObject(std::string &&name) noexcept;
		virtual Light *createLight(std::string &&name) noexcept;
		virtual Texture *createTexture(std::string &&name) noexcept;
		virtual const Material *createMaterial(std::string &&name) noexcept;
		virtual const Camera * createCamera(std::string &&name) noexcept;
		virtual const Background * createBackground(std::string &&name) noexcept;
		virtual Integrator *createIntegrator(std::string &&name) noexcept;
		virtual VolumeRegion *createVolumeRegion(std::string &&name) noexcept;
		virtual RenderView *createRenderView(std::string &&name) noexcept;
		virtual Image *createImage(std::string &&name) noexcept;
		virtual ImageOutput *createOutput(std::string &&name) noexcept;
		void setRenderNotifyViewCallback(yafaray_RenderNotifyViewCallback_t callback, void *callback_data) noexcept;
		void setRenderNotifyLayerCallback(yafaray_RenderNotifyLayerCallback_t callback, void *callback_data) noexcept;
		void setRenderPutPixelCallback(yafaray_RenderPutPixelCallback_t callback, void *callback_data) noexcept;
		void setRenderHighlightPixelCallback(yafaray_RenderHighlightPixelCallback_t callback, void *callback_data) noexcept;
		void setRenderFlushAreaCallback(yafaray_RenderFlushAreaCallback_t callback, void *callback_data) noexcept;
		void setRenderFlushCallback(yafaray_RenderFlushCallback_t callback, void *callback_data) noexcept;
		void setRenderHighlightAreaCallback(yafaray_RenderHighlightAreaCallback_t callback, void *callback_data) noexcept;
		bool removeOutput(std::string &&name) noexcept;
		virtual void clearOutputs() noexcept;
		virtual void clearAll() noexcept;
		virtual void setupRender() noexcept;
		virtual void render(std::shared_ptr<ProgressBar> &&progress_bar) noexcept; //!< render the scene...
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
		ColorSpace input_color_space_ = ColorSpace::RawManualGamma;
};

} //namespace yafaray

#endif // YAFARAY_INTERFACE_H
