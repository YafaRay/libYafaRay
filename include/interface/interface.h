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

#include "constants.h"
#include "color/color.h"
#include <list>
#include <vector>
#include <string>
#include <memory>

BEGIN_YAFARAY

class Light;
class Texture;
class Material;
class Camera;
class Background;
class Integrator;
class VolumeRegion;
class Scene;
class ColorOutput;
class RenderView;
class ParamMap;
class ImageFilm;
class Format;
class ProgressBar;
class Matrix4;
class Object;

class Interface
{
	public:
		Interface();
		virtual ~Interface();
		virtual void createScene();
		virtual bool startGeometry(); //!< call before creating geometry; only meshes and vmaps can be created in this state
		virtual bool endGeometry(); //!< call after creating geometry;
		virtual unsigned int getNextFreeId();
		virtual bool endObject(); //!< end current mesh and return to geometry state
		virtual int  addVertex(double x, double y, double z); //!< add vertex to mesh; returns index to be used for addTriangle
		virtual int  addVertex(double x, double y, double z, double ox, double oy, double oz); //!< add vertex with Orco to mesh; returns index to be used for addTriangle
		virtual void addNormal(double nx, double ny, double nz); //!< add vertex normal to mesh; the vertex that will be attached to is the last one inserted by addVertex method
		virtual bool addFace(int a, int b, int c); //!< add a triangle given vertex indices and material pointer
		virtual bool addFace(int a, int b, int c, int uv_a, int uv_b, int uv_c); //!< add a triangle given vertex and uv indices and material pointer
		virtual int  addUv(float u, float v); //!< add a UV coordinate pair; returns index to be used for addTriangle
		virtual bool smoothMesh(const char *name, double angle); //!< smooth vertex normals of mesh with given ID and angle (in degrees)
		virtual bool addInstance(const char *base_object_name, const Matrix4 &obj_to_world);
		// functions to build paramMaps instead of passing them from Blender
		// (decouling implementation details of STL containers, paraMap_t etc. as much as possible)
		virtual void paramsSetVector(const char *name, double x, double y, double z);
		virtual void paramsSetString(const char *name, const char *s);
		virtual void paramsSetBool(const char *name, bool b);
		virtual void paramsSetInt(const char *name, int i);
		virtual void paramsSetFloat(const char *name, double f);
		virtual void paramsSetColor(const char *name, float r, float g, float b, float a = 1.f);
		virtual void paramsSetColor(const char *name, const float *rgb, bool with_alpha = false);
		virtual void paramsSetMatrix(const char *name, const float m[4][4], bool transpose = false);
		virtual void paramsSetMatrix(const char *name, const double m[4][4], bool transpose = false);
		virtual void paramsSetMemMatrix(const char *name, const float *matrix, bool transpose = false);
		virtual void paramsSetMemMatrix(const char *name, const double *matrix, bool transpose = false);
		virtual void paramsClearAll(); 	//!< clear the paramMap and paramList
		virtual void paramsStartList(); //!< start writing parameters to the extended paramList (used by materials)
		virtual void paramsPushList(); 	//!< push new list item in paramList (e.g. new shader node description)
		virtual void paramsEndList(); 	//!< revert to writing to normal paramMap
		virtual void setCurrentMaterial(const char *name);
		virtual Object *createObject(const char *name);
		virtual Light *createLight(const char *name);
		virtual Texture *createTexture(const char *name);
		virtual Material *createMaterial(const char *name);
		virtual Camera *createCamera(const char *name);
		virtual Background *createBackground(const char *name);
		virtual Integrator *createIntegrator(const char *name);
		virtual VolumeRegion *createVolumeRegion(const char *name);
		virtual RenderView *createRenderView(const char *name);
		virtual ColorOutput *createOutput(const char *name, bool auto_delete = true); //!< ColorOutput creation, usually for internally-owned outputs that are destroyed when the scene is deleted or when libYafaRay instance is closed. If the client wants to keep ownership, it can set the "auto_delete" to false.
		virtual ColorOutput *createOutput(const char *name, ColorOutput *output, bool auto_delete = false); //!< ColorOutput creation, usually for externally client-owned and client-supplied outputs that are *NOT* destroyed when the scene is deleted or when libYafaRay instance is closed. If the client wants to transfer ownership to libYafaRay, it can set the "auto_delete" to true.
		bool removeOutput(const char *name);
		virtual void clearOutputs();
		virtual void clearAll();
		virtual void render(ProgressBar *pb = nullptr, bool auto_delete_progress_bar = false); //!< render the scene...
		virtual void defineLayer(const std::string &layer_type_name, const std::string &exported_image_type_name, const std::string &exported_image_name, const std::string &image_type_name = "");
		virtual bool setupLayersParameters();
		virtual void abort();
		virtual ParamMap *getRenderParameters() { return params_.get(); }

		bool setInteractive(bool interactive);
		void enablePrintDateTime(bool value);
		void setConsoleVerbosityLevel(const std::string &str_v_level);
		void setLogVerbosityLevel(const std::string &str_v_level);
		std::string getVersion() const; //!< Get version to check against the exporters

		/*! Console Printing wrappers to report in color with yafaray's own console coloring */
		void printDebug(const std::string &msg) const;
		void printVerbose(const std::string &msg) const;
		void printInfo(const std::string &msg) const;
		void printParams(const std::string &msg) const;
		void printWarning(const std::string &msg) const;
		void printError(const std::string &msg) const;

		void setInputColorSpace(const std::string &color_space_string, float gamma_val);

	protected:
		virtual void setCurrentMaterial(const Material *material);
		virtual const Material *getCurrentMaterial() const;
		std::unique_ptr<ParamMap> params_;
		std::unique_ptr<std::list<ParamMap>> eparams_; //! for materials that need to define a whole shader tree etc.
		ParamMap *cparams_ = nullptr; //! just a pointer to the current paramMap, either params or a eparams element
		std::unique_ptr<Scene> scene_;
		float input_gamma_ = 1.f;
		ColorSpace input_color_space_ = RawManualGamma;
};

END_YAFARAY

#endif // YAFARAY_INTERFACE_H
