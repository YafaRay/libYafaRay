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

#include "interface/interface.h"
#include <map>
#include <iostream>
#include <fstream>

BEGIN_YAFARAY

class XmlExport: public Interface
{
	public:
		XmlExport(const char *fname, const ::yafaray_LoggerCallback_t logger_callback = nullptr, void *callback_user_data = nullptr, ::yafaray_DisplayConsole_t logger_display_console = YAFARAY_DISPLAY_CONSOLE_NORMAL);
		virtual void createScene() noexcept override;
		virtual bool setupLayersParameters() noexcept override; //!< setup render passes information
		virtual void defineLayer() noexcept override;
		virtual bool startGeometry() noexcept override;
		virtual bool endGeometry() noexcept override;
		virtual unsigned int getNextFreeId() noexcept override;
		virtual bool endObject() noexcept override;
		virtual bool addInstance(const char *base_object_name, const Matrix4 &obj_to_world) noexcept override;
		virtual int  addVertex(double x, double y, double z) noexcept override; //!< add vertex to mesh; returns index to be used for addTriangle
		virtual int  addVertex(double x, double y, double z, double ox, double oy, double oz) noexcept override; //!< add vertex with Orco to mesh; returns index to be used for addTriangle
		virtual void addNormal(double nx, double ny, double nz) noexcept override; //!< add vertex normal to mesh; the vertex that will be attached to is the last one inserted by addVertex method
		virtual bool addFace(int a, int b, int c) noexcept override;
		virtual bool addFace(int a, int b, int c, int uv_a, int uv_b, int uv_c) noexcept override;
		virtual int  addUv(float u, float v) noexcept override;
		virtual bool smoothMesh(const char *name, double angle) noexcept override;
		virtual void setCurrentMaterial(const char *name) noexcept override;
		virtual Object *createObject(const char *name) noexcept override;
		virtual Light *createLight(const char *name) noexcept override;
		virtual Texture *createTexture(const char *name) noexcept override;
		virtual Material *createMaterial(const char *name) noexcept override;
		virtual Camera *createCamera(const char *name) noexcept override;
		virtual Background *createBackground(const char *name) noexcept override;
		virtual Integrator *createIntegrator(const char *name) noexcept override;
		virtual VolumeRegion *createVolumeRegion(const char *name) noexcept override;
		virtual RenderView *createRenderView(const char *name) noexcept override;
		virtual Image *createImage(const char *name) noexcept override;
		virtual ColorOutput *createOutput(const char *name, bool auto_delete = true, void *callback_user_data = nullptr, yafaray_OutputPutpixelCallback_t output_putpixel_callback = nullptr, yafaray_OutputFlushAreaCallback_t output_flush_area_callback = nullptr, yafaray_OutputFlushCallback_t output_flush_callback = nullptr) noexcept override;
		virtual void clearAll() noexcept override; //!< clear the whole environment + scene, i.e. free (hopefully) all memory.
		virtual void clearOutputs() noexcept override { }
		virtual void setupRender() noexcept override;
		virtual void render(ProgressBar *pb = nullptr, bool auto_delete_progress_bar = false, ::yafaray_DisplayConsole_t progress_bar_display_console = YAFARAY_DISPLAY_CONSOLE_NORMAL) noexcept override; //!< render the scene...
		void setXmlColorSpace(std::string color_space_string, float gamma_val) noexcept;

	protected:
		void writeParamMap(const ParamMap &param_map, int indent = 1) noexcept;
		void writeParamList(int indent) noexcept;
		std::ofstream xml_file_;
		std::string xml_name_;
		std::string current_material_;
		int n_uvs_ = 0;
		unsigned int next_obj_ = 0;
		float xml_gamma_ = 1.f;
		ColorSpace xml_color_space_ = RawManualGamma;
};

END_YAFARAY

#endif // YAFARAY_EXPORT_XML_H
