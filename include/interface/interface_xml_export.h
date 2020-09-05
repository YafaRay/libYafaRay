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

#ifndef YAFARAY_INTERFACE_XML_EXPORT_H
#define YAFARAY_INTERFACE_XML_EXPORT_H

#include "interface/interface.h"
#include <map>
#include <iostream>
#include <fstream>

BEGIN_YAFARAY

class LIBYAFARAY_EXPORT XmlInterface: public Interface
{
	public:
		XmlInterface();
		// directly related to scene_t:
		virtual bool setLoggingAndBadgeSettings();
		virtual bool setupRenderPasses(); //!< setup render passes information
		virtual bool startGeometry();
		virtual bool endGeometry();
		virtual unsigned int getNextFreeId();
		virtual bool startTriMesh(unsigned int id, int vertices, int triangles, bool has_orco, bool has_uv = false, int type = 0, int obj_pass_index = 0);
		virtual bool startTriMeshPtr(unsigned int *id, int vertices, int triangles, bool has_orco, bool has_uv = false, int type = 0, int obj_pass_index = 0);
		virtual bool startCurveMesh(unsigned int id, int vertices, int obj_pass_index = 0);
		virtual bool endTriMesh();
		virtual bool addInstance(unsigned int base_object_id, Matrix4 obj_to_world);
		virtual bool endCurveMesh(const Material *mat, float strand_start, float strand_end, float strand_shape);
		virtual int  addVertex(double x, double y, double z); //!< add vertex to mesh; returns index to be used for addTriangle
		virtual int  addVertex(double x, double y, double z, double ox, double oy, double oz); //!< add vertex with Orco to mesh; returns index to be used for addTriangle
		virtual void addNormal(double nx, double ny, double nz); //!< add vertex normal to mesh; the vertex that will be attached to is the last one inserted by addVertex method
		virtual bool addTriangle(int a, int b, int c, const Material *mat);
		virtual bool addTriangle(int a, int b, int c, int uv_a, int uv_b, int uv_c, const Material *mat);
		virtual int  addUv(float u, float v);
		virtual bool smoothMesh(unsigned int id, double angle);

		// functions directly related to renderEnvironment_t
		virtual Light 		*createLight(const char *name);
		virtual Texture 		*createTexture(const char *name);
		virtual Material 	*createMaterial(const char *name);
		virtual Camera 		*createCamera(const char *name);
		virtual Background 	*createBackground(const char *name);
		virtual Integrator 	*createIntegrator(const char *name);
		virtual VolumeRegion 	*createVolumeRegion(const char *name);
		virtual unsigned int 	createObject(const char *name);
		virtual void clearAll(); //!< clear the whole environment + scene, i.e. free (hopefully) all memory.
		virtual void render(ColorOutput &output, ProgressBar *pb = nullptr); //!< render the scene...
		virtual bool startScene(int type = 0); //!< start a new scene; Must be called before any of the scene_t related callbacks!

		virtual void setOutfile(const char *fname);

		void setXmlColorSpace(std::string color_space_string, float gamma_val);

	protected:
		void writeParamMap(const ParamMap &pmap, int indent = 1);
		void writeParamList(int indent);

		std::map<const Material *, std::string> materials_;
		std::ofstream xml_file_;
		std::string xml_name_;
		const Material *last_mat_;
		size_t nmat_;
		int n_uvs_;
		unsigned int next_obj_;
		float xml_gamma_;
		ColorSpace xml_color_space_;
};

typedef XmlInterface *XmlInterfaceConstructor_t();

END_YAFARAY

#endif // YAFARAY_INTERFACE_XML_EXPORT_H
