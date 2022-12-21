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

#ifndef YAFARAY_EXPORT_PYTHON_H
#define YAFARAY_EXPORT_PYTHON_H

#include "geometry/vector.h"
#include "interface/interface.h"
#include "integrator/surface/integrator_surface.h"
#include <fstream>
#include <iostream>
#include <map>

namespace yafaray {

class Parameter;

class ExportPython: public Interface
{
	public:
		explicit ExportPython(const char *fname, ::yafaray_LoggerCallback logger_callback = nullptr, void *callback_data = nullptr, ::yafaray_DisplayConsole logger_display_console = YAFARAY_DISPLAY_CONSOLE_NORMAL);
		void createScene() noexcept override;
		int getSceneFilmWidth() const noexcept override { return 0; }
		int getSceneFilmHeight() const noexcept override { return 0; }
		void defineLayer() noexcept override;
		bool initObject(size_t object_id, size_t material_id) noexcept override;
		size_t createInstance() noexcept override;
		bool addInstanceObject(size_t instance_id, size_t base_object_id) noexcept override;
		bool addInstanceOfInstance(size_t instance_id, size_t base_instance_id) noexcept override;
		bool addInstanceMatrix(size_t instance_id, Matrix4f &&obj_to_world, float time) noexcept override;
		int addVertex(size_t object_id, Point3f &&vertex, unsigned char time_step) noexcept override; //!< add vertex to mesh; returns index to be used for addTriangle/addQuad
		int addVertex(size_t object_id, Point3f &&vertex, Point3f &&orco, unsigned char time_step) noexcept override; //!< add vertex with Orco to mesh; returns index to be used for addTriangle/addQuad
		void addVertexNormal(size_t object_id, Vec3f &&normal, unsigned char time_step) noexcept override; //!< add vertex normal to mesh; the vertex that will be attached to is the last one inserted by addVertex method
		bool addFace(size_t object_id, const FaceIndices<int> &face_indices, size_t material_id) noexcept override;
		int addUv(size_t object_id, Uv<float> &&uv) noexcept override;
		bool smoothVerticesNormals(size_t object_id, double angle) noexcept override;
		std::pair<size_t, ParamResult> createObject(const std::string &name) noexcept override;
		std::pair<size_t, ParamResult> createLight(const std::string &name) noexcept override;
		std::pair<size_t, ParamResult> createTexture(const std::string &name) noexcept override;
		std::pair<size_t, ParamResult> createMaterial(const std::string &name) noexcept override;
		std::pair<size_t, ParamResult> createCamera(const std::string &name) noexcept override;
		ParamResult defineBackground() noexcept override;
		ParamResult defineSurfaceIntegrator() noexcept override;
		ParamResult defineVolumeIntegrator() noexcept override;
		std::pair<size_t, ParamResult> createVolumeRegion(const std::string &name) noexcept override;
		std::pair<size_t, ParamResult> createRenderView(const std::string &name) noexcept override;
		std::pair<size_t, ParamResult> createImage(const std::string &name) noexcept override;
		std::pair<size_t, ParamResult> createOutput(const std::string &name) noexcept override;
		void clearAll() noexcept override; //!< clear the whole environment + scene, i.e. free (hopefully) all memory.
		void clearOutputs() noexcept override { }
		void setupRender() noexcept override;
		void render(std::unique_ptr<ProgressBar> progress_bar) noexcept override; //!< render the scene...
		void setColorSpace(const std::string& color_space_string, float gamma_val) noexcept;

	protected:
		void writeParamMap(const ParamMap &param_map, int indent = 1) noexcept;
		void writeParamList(int indent) noexcept;
		static void writeMatrix(const Matrix4f &m, std::ofstream &file) noexcept;
		static void writeParam(const std::string &name, const Parameter &param, std::ofstream &file, ColorSpace color_space, float gamma) noexcept;
		std::ofstream file_;
		std::string file_name_;
		std::string current_material_;
		int current_instance_id_ = 0;
		int n_uvs_ = 0;
		unsigned int next_obj_ = 0;
		float gamma_ = 1.f;
		ColorSpace color_space_{ColorSpace::Srgb};
};

} //namespace yafaray

#endif // YAFARAY_EXPORT_PYTHON_H
