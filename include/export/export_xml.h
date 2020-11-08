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

class LIBYAFARAY_EXPORT XmlExport: public Interface
{
	public:
		XmlExport(const char *fname, int type);
		virtual bool setupLayersParameters() override; //!< setup render passes information
		virtual void defineLayer(const std::string &layer_type_name, const std::string &exported_image_type_name, const std::string &exported_image_name) override;
		virtual bool startGeometry() override;
		virtual bool endGeometry() override;
		virtual unsigned int getNextFreeId() override;
		virtual bool startTriMesh(const char *name, int vertices, int triangles, bool has_orco, bool has_uv = false, int type = 0, int obj_pass_index = 0) override;
		virtual bool startCurveMesh(const char *name, int vertices, int obj_pass_index = 0) override;
		virtual bool endTriMesh() override;
		virtual bool addInstance(const char *base_object_name, const Matrix4 &obj_to_world) override;
		virtual bool endCurveMesh(const Material *mat, float strand_start, float strand_end, float strand_shape) override;
		virtual int  addVertex(double x, double y, double z) override; //!< add vertex to mesh; returns index to be used for addTriangle
		virtual int  addVertex(double x, double y, double z, double ox, double oy, double oz) override; //!< add vertex with Orco to mesh; returns index to be used for addTriangle
		virtual void addNormal(double nx, double ny, double nz) override; //!< add vertex normal to mesh; the vertex that will be attached to is the last one inserted by addVertex method
		virtual bool addTriangle(int a, int b, int c, const Material *mat) override;
		virtual bool addTriangle(int a, int b, int c, int uv_a, int uv_b, int uv_c, const Material *mat) override;
		virtual int  addUv(float u, float v) override;
		virtual bool smoothMesh(const char *name, double angle) override;

		// functions directly related to Scene_t
		virtual Light 		*createLight(const char *name) override;
		virtual Texture 		*createTexture(const char *name) override;
		virtual Material 	*createMaterial(const char *name) override;
		virtual Camera 		*createCamera(const char *name) override;
		virtual Background 	*createBackground(const char *name) override;
		virtual Integrator 	*createIntegrator(const char *name) override;
		virtual VolumeRegion 	*createVolumeRegion(const char *name) override;
		virtual ColorOutput *createOutput(const char *name) override;
		virtual RenderView *createRenderView(const char *name) override;
		virtual unsigned int 	createObject(const char *name) override;
		virtual void clearAll() override; //!< clear the whole environment + scene, i.e. free (hopefully) all memory.
		virtual void render(ProgressBar *pb = nullptr) override; //!< render the scene...
		void setXmlColorSpace(std::string color_space_string, float gamma_val);

	protected:
		void writeParamMap(const ParamMap &param_map, int indent = 1);
		void writeParamList(int indent);

		std::map<const Material *, std::string> materials_;
		std::ofstream xml_file_;
		std::string xml_name_;
		const Material *last_mat_ = nullptr;
		size_t nmat_ = 0;
		int n_uvs_ = 0;
		unsigned int next_obj_ = 0;
		float xml_gamma_ = 1.f;
		ColorSpace xml_color_space_ = RawManualGamma;
};

typedef XmlExport *XmlExportConstructor_t();

END_YAFARAY

#endif // YAFARAY_EXPORT_XML_H
