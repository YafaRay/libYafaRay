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

#ifndef YAFARAY_EXPORT_XML_H
#define YAFARAY_EXPORT_XML_H

#include "geometry/uv.h"
#include "geometry/vector.h"
#include "interface/interface.h"
#include <fstream>
#include <iostream>
#include <map>

BEGIN_YAFARAY

class Parameter;

class ExportXml: public Interface
{
	public:
		explicit ExportXml(const char *fname, ::yafaray_LoggerCallback_t logger_callback = nullptr, void *callback_data = nullptr, ::yafaray_DisplayConsole_t logger_display_console = YAFARAY_DISPLAY_CONSOLE_NORMAL);
		void createScene() noexcept override;
		int getSceneFilmWidth() const noexcept override { return 0; }
		int getSceneFilmHeight() const noexcept override { return 0; }
		void defineLayer() noexcept override;
		bool startGeometry() noexcept override;
		bool endGeometry() noexcept override;
		unsigned int getNextFreeId() noexcept override;
		bool endObject() noexcept override;
		size_t createInstance() noexcept override;
		bool addInstanceObject(size_t instance_id, std::string &&base_object_name) noexcept override;
		bool addInstanceOfInstance(size_t instance_id, size_t base_instance_id) noexcept override;
		bool addInstanceMatrix(size_t instance_id, Matrix4 &&obj_to_world, float time) noexcept override;
		int addVertex(Point3 &&vertex, size_t time_step) noexcept override; //!< add vertex to mesh; returns index to be used for addTriangle/addQuad
		int addVertex(Point3 &&vertex, Point3 &&orco, size_t time_step) noexcept override; //!< add vertex with Orco to mesh; returns index to be used for addTriangle/addQuad
		void addVertexNormal(Vec3 &&normal, size_t time_step) noexcept override; //!< add vertex normal to mesh; the vertex that will be attached to is the last one inserted by addVertex method
		bool addFace(std::vector<int> &&vertices, std::vector<int> &&uv_indices) noexcept override;
		int addUv(Uv &&uv) noexcept override;
		bool smoothVerticesNormals(std::string &&name, double angle) noexcept override;
		void setCurrentMaterial(std::string &&name) noexcept override;
		Object *createObject(std::string &&name) noexcept override;
		Light *createLight(std::string &&name) noexcept override;
		Texture *createTexture(std::string &&name) noexcept override;
		const Material *createMaterial(std::string &&name) noexcept override;
		const Camera * createCamera(std::string &&name) noexcept override;
		const Background * createBackground(std::string &&name) noexcept override;
		Integrator *createIntegrator(std::string &&name) noexcept override;
		VolumeRegion *createVolumeRegion(std::string &&name) noexcept override;
		RenderView *createRenderView(std::string &&name) noexcept override;
		Image *createImage(std::string &&name) noexcept override;
		ImageOutput *createOutput(std::string &&name) noexcept override;
		void clearAll() noexcept override; //!< clear the whole environment + scene, i.e. free (hopefully) all memory.
		void clearOutputs() noexcept override { }
		void setupRender() noexcept override;
		void render(std::shared_ptr<ProgressBar> &&progress_bar) noexcept override; //!< render the scene...
		void setColorSpace(const std::string& color_space_string, float gamma_val) noexcept;

	protected:
		void writeParamMap(const ParamMap &param_map, int indent = 1) noexcept;
		void writeParamList(int indent) noexcept;
		static void writeMatrix(const std::string &name, const Matrix4 &m, std::ofstream &file) noexcept;
		static void writeParam(const std::string &name, const Parameter &param, std::ofstream &file, ColorSpace color_space, float gamma) noexcept;
		std::ofstream file_;
		std::string file_name_;
		std::string current_material_;
		size_t current_instance_id_ = 0;
		int n_uvs_ = 0;
		unsigned int next_obj_ = 0;
		float gamma_ = 1.f;
		ColorSpace color_space_ = ColorSpace::RawManualGamma;
};

END_YAFARAY

#endif // YAFARAY_EXPORT_XML_H
