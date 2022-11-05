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

#ifndef YAFARAY_EXPORT_C_H
#define YAFARAY_EXPORT_C_H

#include "geometry/vector.h"
#include "interface/interface.h"
#include "integrator/surface/integrator_surface.h"
#include <fstream>
#include <iostream>
#include <map>

namespace yafaray {

class Parameter;

class ExportC: public Interface
{
	public:
		explicit ExportC(const char *fname, ::yafaray_LoggerCallback_t logger_callback = nullptr, void *callback_data = nullptr, ::yafaray_DisplayConsole_t logger_display_console = YAFARAY_DISPLAY_CONSOLE_NORMAL);
		void createScene() noexcept override;
		int getSceneFilmWidth() const noexcept override { return 0; }
		int getSceneFilmHeight() const noexcept override { return 0; }
		void defineLayer() noexcept override;
		bool startGeometry() noexcept override;
		bool endGeometry() noexcept override;
		unsigned int getNextFreeId() noexcept override;
		bool endObject() noexcept override;
		int createInstance() noexcept override;
		bool addInstanceObject(int instance_id, std::string &&base_object_name) noexcept override;
		bool addInstanceOfInstance(int instance_id, size_t base_instance_id) noexcept override;
		bool addInstanceMatrix(int instance_id, Matrix4f &&obj_to_world, float time) noexcept override;
		int addVertex(Point3f &&vertex, int time_step) noexcept override; //!< add vertex to mesh; returns index to be used for addTriangle/addQuad
		int addVertex(Point3f &&vertex, Point3f &&orco, int time_step) noexcept override; //!< add vertex with Orco to mesh; returns index to be used for addTriangle/addQuad
		void addVertexNormal(Vec3f &&normal, int time_step) noexcept override; //!< add vertex normal to mesh; the vertex that will be attached to is the last one inserted by addVertex method
		bool addFace(const FaceIndices &face_indices) noexcept override;
		int addUv(Uv<float> &&uv) noexcept override;
		bool smoothVerticesNormals(std::string &&name, double angle) noexcept override;
		void setCurrentMaterial(std::string &&name) noexcept override;
		std::pair<size_t, ParamResult> createObject(std::string &&name) noexcept override;
		std::pair<size_t, ParamResult> createLight(std::string &&name) noexcept override;
		std::pair<size_t, ParamResult> createTexture(std::string &&name) noexcept override;
		std::pair<size_t, ParamResult> createMaterial(std::string &&name) noexcept override;
		std::pair<size_t, ParamResult> createCamera(std::string &&name) noexcept override;
		ParamResult defineBackground() noexcept override;
		ParamResult defineSurfaceIntegrator() noexcept override;
		ParamResult defineVolumeIntegrator() noexcept override;
		std::pair<size_t, ParamResult> createVolumeRegion(std::string &&name) noexcept override;
		std::pair<size_t, ParamResult> createRenderView(std::string &&name) noexcept override;
		std::pair<Image *, ParamResult> createImage(std::string &&name) noexcept override;
		std::pair<size_t, ParamResult> createOutput(std::string &&name) noexcept override;
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
		static std::string generateHeader();
		std::string sectionSplit();
		std::string generateSectionsCalls() const;
		std::string generateMain() const;
		std::ofstream file_;
		std::string file_name_;
		std::string current_material_;
		int current_instance_id_ = 0;
		int n_uvs_ = 0;
		unsigned int next_obj_ = 0;
		float gamma_ = 1.f;
		int section_num_lines_ = 0;
		int num_sections_ = 1;
		ColorSpace color_space_{ColorSpace::Srgb};
		static constexpr inline int section_max_lines_ = 50000; //To divide the exported C code in sections of this maximum amount of lines, so the C compilers do not crash due to out of memory errors in large C scenes.
};

} //namespace yafaray

#endif // YAFARAY_EXPORT_C_H
