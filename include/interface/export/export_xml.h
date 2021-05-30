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
		XmlExport(const char *fname, const ::yafaray4_LoggerCallback_t logger_callback = nullptr, void *callback_user_data = nullptr, ::yafaray4_DisplayConsole_t logger_display_console = YAFARAY_DISPLAY_CONSOLE_NORMAL);
		virtual void createScene() override;
		virtual bool setupLayersParameters() override; //!< setup render passes information
		virtual void defineLayer(const std::string &layer_type_name, const std::string &exported_image_type_name, const std::string &exported_image_name, const std::string &image_type_name = "") override;
		virtual bool startGeometry() override;
		virtual bool endGeometry() override;
		virtual unsigned int getNextFreeId() override;
		virtual bool endObject() override;
		virtual bool addInstance(const char *base_object_name, const Matrix4 &obj_to_world) override;
		virtual int  addVertex(double x, double y, double z) override; //!< add vertex to mesh; returns index to be used for addTriangle
		virtual int  addVertex(double x, double y, double z, double ox, double oy, double oz) override; //!< add vertex with Orco to mesh; returns index to be used for addTriangle
		virtual void addNormal(double nx, double ny, double nz) override; //!< add vertex normal to mesh; the vertex that will be attached to is the last one inserted by addVertex method
		virtual bool addFace(int a, int b, int c) override;
		virtual bool addFace(int a, int b, int c, int uv_a, int uv_b, int uv_c) override;
		virtual int  addUv(float u, float v) override;
		virtual bool smoothMesh(const char *name, double angle) override;
		virtual void setCurrentMaterial(const char *name) override;
		virtual Object *createObject(const char *name) override;
		virtual Light *createLight(const char *name) override;
		virtual Texture *createTexture(const char *name) override;
		virtual Material *createMaterial(const char *name) override;
		virtual Camera *createCamera(const char *name) override;
		virtual Background *createBackground(const char *name) override;
		virtual Integrator *createIntegrator(const char *name) override;
		virtual VolumeRegion *createVolumeRegion(const char *name) override;
		virtual RenderView *createRenderView(const char *name) override;
		virtual Image *createImage(const char *name) override;
		virtual ColorOutput *createOutput(const char *name, bool auto_delete = true, void *callback_user_data = nullptr, yafaray4_OutputPutpixelCallback_t output_putpixel_callback = nullptr, yafaray4_OutputFlushAreaCallback_t output_flush_area_callback = nullptr, yafaray4_OutputFlushCallback_t output_flush_callback = nullptr) override;
		virtual void clearAll() override; //!< clear the whole environment + scene, i.e. free (hopefully) all memory.
		virtual void clearOutputs() override { }
		virtual void render(ProgressBar *pb = nullptr, bool auto_delete_progress_bar = false, ::yafaray4_DisplayConsole_t progress_bar_display_console = YAFARAY_DISPLAY_CONSOLE_NORMAL) override; //!< render the scene...
		void setXmlColorSpace(std::string color_space_string, float gamma_val);

	protected:
		void writeParamMap(const ParamMap &param_map, int indent = 1);
		void writeParamList(int indent);
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
