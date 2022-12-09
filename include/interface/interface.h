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

#ifndef LIBYAFARAY_INTERFACE_H
#define LIBYAFARAY_INTERFACE_H

#include "color/color.h"
#include "geometry/uv.h"
#include "geometry/vector.h"
#include "public_api/yafaray_c_api.h"
#include "integrator/surface/integrator_surface.h"
#include <list>
#include <memory>
#include <string>
#include <vector>

namespace yafaray {

class Light;
template <typename IndexType> class FaceIndices;
class Texture;
class Material;
class Camera;
class Background;
class VolumeRegion;
class Scene;
class ImageOutput;
class RenderView;
class ParamMap;
class ImageFilm;
class Format;
template <typename T, size_t N> class SquareMatrix;
typedef SquareMatrix<float, 4> Matrix4f;
class Image;
class Logger;
class ProgressBar;

class Interface
{
	public:
		explicit Interface(::yafaray_LoggerCallback logger_callback = nullptr, void *callback_data = nullptr, ::yafaray_DisplayConsole logger_display_console = YAFARAY_DISPLAY_CONSOLE_NORMAL) noexcept;
		virtual ~Interface() noexcept;
		void setLoggingCallback(::yafaray_LoggerCallback logger_callback, void *callback_data) noexcept;
		virtual void createScene() noexcept;
		virtual int getSceneFilmWidth() const noexcept;
		virtual int getSceneFilmHeight() const noexcept;
		std::string printLayersTable() const noexcept;
		std::string printViewsTable() const noexcept;
		virtual bool initObject(size_t object_id, size_t material_id) noexcept; //!< initialize/calculate object. The material_id might parameter be used or not depending on the type of object
		virtual int addVertex(size_t object_id, Point3f &&vertex, unsigned char time_step) noexcept; //!< add vertex to mesh; returns index to be used for addTriangle/addQuad
		virtual int addVertex(size_t object_id, Point3f &&vertex, Point3f &&orco, unsigned char time_step) noexcept; //!< add vertex with Orco to mesh; returns index to be used for addTriangle/addQuad
		virtual void addVertexNormal(size_t object_id, Vec3f &&normal, unsigned char time_step) noexcept; //!< add vertex normal to mesh; the vertex that will be attached to is the last one inserted by addVertex method
		virtual bool addFace(size_t object_id, const FaceIndices<int> &face_indices, size_t material_id) noexcept; //!< add a mesh face given vertex indices and optionally uv_indices
		virtual int addUv(size_t object_id, Uv<float> &&uv) noexcept; //!< add a UV coordinate pair; returns index to be used for addTriangle/addQuad
		virtual bool smoothVerticesNormals(size_t object_id, double angle) noexcept; //!< smooth vertex normals of mesh with given ID and angle (in degrees)
		virtual size_t createInstance() noexcept;
		virtual bool addInstanceObject(size_t instance_id, size_t base_object_id) noexcept;
		virtual bool addInstanceOfInstance(size_t instance_id, size_t base_instance_id) noexcept;
		virtual bool addInstanceMatrix(size_t instance_id, Matrix4f &&obj_to_world, float time) noexcept;
		virtual void paramsSetVector(std::string &&name, Vec3f &&v) noexcept;
		virtual void paramsSetString(std::string &&name, std::string &&s) noexcept;
		virtual void paramsSetBool(std::string &&name, bool b) noexcept;
		virtual void paramsSetInt(std::string &&name, int i) noexcept;
		virtual void paramsSetFloat(std::string &&name, double f) noexcept;
		virtual void paramsSetColor(std::string &&name, Rgba &&col) noexcept;
		void paramsSetColor(std::string &&name, Rgb &&col) noexcept { paramsSetColor(std::move(name), std::move(Rgba{col})); };
		virtual void paramsSetMatrix(std::string &&name, Matrix4f &&matrix, bool transpose) noexcept;
		void paramsSetMatrix(std::string &&name, Matrix4f &&matrix) noexcept { paramsSetMatrix(std::move(name), std::move(matrix), false); };
		virtual void paramsClearAll() noexcept; 	//!< clear the paramMap and paramList
		virtual void paramsPushList() noexcept; 	//!< push new list item in paramList (e.g. new shader node description)
		virtual void paramsEndList() noexcept; 	//!< revert to writing to normal paramMap
		std::pair<Size2i, bool> getImageSize(size_t image_id) const noexcept;
		std::pair<Rgba, bool> getImageColor(size_t image_id, const Point2i &point) const noexcept;
		bool setImageColor(size_t image_id, const Point2i &point, const Rgba &col) noexcept;
		std::pair<size_t, ResultFlags> getImageId(std::string &&name) noexcept;
		std::pair<size_t, ResultFlags> getObjectId(std::string &&name) noexcept;
		std::pair<size_t, ResultFlags> getMaterialId(std::string &&name) noexcept;
		virtual std::pair<size_t, ParamResult> createObject(std::string &&name) noexcept;
		virtual std::pair<size_t, ParamResult> createLight(std::string &&name) noexcept;
		virtual std::pair<size_t, ParamResult> createTexture(std::string &&name) noexcept;
		virtual std::pair<size_t, ParamResult> createMaterial(std::string &&name) noexcept;
		virtual std::pair<size_t, ParamResult> createCamera(std::string &&name) noexcept;
		virtual ParamResult defineBackground() noexcept;
		virtual ParamResult defineSurfaceIntegrator() noexcept;
		virtual ParamResult defineVolumeIntegrator() noexcept;
		virtual std::pair<size_t, ParamResult> createVolumeRegion(std::string &&name) noexcept;
		virtual std::pair<size_t, ParamResult> createRenderView(std::string &&name) noexcept;
		virtual std::pair<size_t, ParamResult> createImage(std::string &&name) noexcept;
		virtual std::pair<size_t, ParamResult> createOutput(std::string &&name) noexcept;
		void setRenderNotifyViewCallback(yafaray_RenderNotifyViewCallback callback, void *callback_data) noexcept;
		void setRenderNotifyLayerCallback(yafaray_RenderNotifyLayerCallback callback, void *callback_data) noexcept;
		void setRenderPutPixelCallback(yafaray_RenderPutPixelCallback callback, void *callback_data) noexcept;
		void setRenderHighlightPixelCallback(yafaray_RenderHighlightPixelCallback callback, void *callback_data) noexcept;
		void setRenderFlushAreaCallback(yafaray_RenderFlushAreaCallback callback, void *callback_data) noexcept;
		void setRenderFlushCallback(yafaray_RenderFlushCallback callback, void *callback_data) noexcept;
		void setRenderHighlightAreaCallback(yafaray_RenderHighlightAreaCallback callback, void *callback_data) noexcept;
		bool removeOutput(std::string &&name) noexcept;
		virtual void clearOutputs() noexcept;
		virtual void clearAll() noexcept;
		virtual void setupRender() noexcept;
		virtual void render(std::unique_ptr<ProgressBar> progress_bar) noexcept; //!< render the scene...
		virtual void defineLayer() noexcept;
		virtual void cancel() noexcept;

		void enablePrintDateTime(bool value) noexcept;
		void setConsoleVerbosityLevel(const ::yafaray_LogLevel &log_level) noexcept;
		void setLogVerbosityLevel(const ::yafaray_LogLevel &log_level) noexcept;
		void printDebug(const std::string &msg) const noexcept;
		void printVerbose(const std::string &msg) const noexcept;
		void printInfo(const std::string &msg) const noexcept;
		void printParams(const std::string &msg) const noexcept;
		void printWarning(const std::string &msg) const noexcept;
		void printError(const std::string &msg) const noexcept;
		void setConsoleLogColorsEnabled(bool console_log_colors_enabled) const noexcept;
		void setInputColorSpace(const std::string &color_space_string, float gamma_val) noexcept;

	protected:
		std::unique_ptr<Logger> logger_;
		std::unique_ptr<ParamMap> params_;
		std::list<ParamMap> nodes_params_; //! for materials that need to define a whole shader tree etc.
		ParamMap *cparams_ = nullptr; //! just a pointer to the current paramMap, either params or a nodes_params element
		std::unique_ptr<Scene> scene_;
		float input_gamma_ = 1.f;
		ColorSpace input_color_space_ = ColorSpace::RawManualGamma;
};

} //namespace yafaray

#endif // LIBYAFARAY_INTERFACE_H
