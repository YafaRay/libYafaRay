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
#include <list>
#include <vector>
#include <string>
#include "common/color.h"


BEGIN_YAFARAY

class Light;
class Texture;
class Material;
class Camera;
class Background;
class Integrator;
class VolumeRegion;
class Scene;
class RenderEnvironment;
class ColorOutput;
class ParamMap;
class ImageFilm;
class ImageHandler;
class ProgressBar;
class Matrix4;

class LIBYAFARAY_EXPORT Interface
{
	public:
		Interface();
		virtual ~Interface();
		// directly related to scene_t:
		virtual bool startGeometry(); //!< call before creating geometry; only meshes and vmaps can be created in this state
		virtual bool endGeometry(); //!< call after creating geometry;
		/*! start a triangle mesh
			in this state only vertices, UVs and triangles can be created
			\param id returns the ID of the created mesh
		*/
		virtual unsigned int getNextFreeId();
		virtual bool startTriMesh(unsigned int id, int vertices, int triangles, bool has_orco, bool has_uv = false, int type = 0, int obj_pass_index = 0);
		virtual bool startCurveMesh(unsigned int id, int vertices, int obj_pass_index = 0);
		virtual bool startTriMeshPtr(unsigned int *id, int vertices, int triangles, bool has_orco, bool has_uv = false, int type = 0, int obj_pass_index = 0);
		virtual bool endTriMesh(); //!< end current mesh and return to geometry state
		virtual bool endCurveMesh(const Material *mat, float strand_start, float strand_end, float strand_shape); //!< end current mesh and return to geometry state
		virtual int  addVertex(double x, double y, double z); //!< add vertex to mesh; returns index to be used for addTriangle
		virtual int  addVertex(double x, double y, double z, double ox, double oy, double oz); //!< add vertex with Orco to mesh; returns index to be used for addTriangle
		virtual void addNormal(double nx, double ny, double nz); //!< add vertex normal to mesh; the vertex that will be attached to is the last one inserted by addVertex method
		virtual bool addTriangle(int a, int b, int c, const Material *mat); //!< add a triangle given vertex indices and material pointer
		virtual bool addTriangle(int a, int b, int c, int uv_a, int uv_b, int uv_c, const Material *mat); //!< add a triangle given vertex and uv indices and material pointer
		virtual int  addUv(float u, float v); //!< add a UV coordinate pair; returns index to be used for addTriangle
		virtual bool smoothMesh(unsigned int id, double angle); //!< smooth vertex normals of mesh with given ID and angle (in degrees)
		virtual bool addInstance(unsigned int base_object_id, Matrix4 obj_to_world);
		// functions to build paramMaps instead of passing them from Blender
		// (decouling implementation details of STL containers, paraMap_t etc. as much as possible)
		virtual void paramsSetPoint(const char *name, double x, double y, double z);
		virtual void paramsSetString(const char *name, const char *s);
		virtual void paramsSetBool(const char *name, bool b);
		virtual void paramsSetInt(const char *name, int i);
		virtual void paramsSetFloat(const char *name, double f);
		virtual void paramsSetColor(const char *name, float r, float g, float b, float a = 1.f);
		virtual void paramsSetColor(const char *name, float *rgb, bool with_alpha = false);
		virtual void paramsSetMatrix(const char *name, float m[4][4], bool transpose = false);
		virtual void paramsSetMatrix(const char *name, double m[4][4], bool transpose = false);
		virtual void paramsSetMemMatrix(const char *name, float *matrix, bool transpose = false);
		virtual void paramsSetMemMatrix(const char *name, double *matrix, bool transpose = false);
		virtual void paramsClearAll(); 	//!< clear the paramMap and paramList
		virtual void paramsStartList(); //!< start writing parameters to the extended paramList (used by materials)
		virtual void paramsPushList(); 	//!< push new list item in paramList (e.g. new shader node description)
		virtual void paramsEndList(); 	//!< revert to writing to normal paramMap
		// functions directly related to renderEnvironment_t
		virtual Light 		*createLight(const char *name);
		virtual Texture 		*createTexture(const char *name);
		virtual Material 	*createMaterial(const char *name);
		virtual Camera 		*createCamera(const char *name);
		virtual Background 	*createBackground(const char *name);
		virtual Integrator 	*createIntegrator(const char *name);
		virtual VolumeRegion 	*createVolumeRegion(const char *name);
		virtual ImageHandler	*createImageHandler(const char *name, bool add_to_table = true);  //!< The addToTable parameter, if true, allows to avoid the interface from taking ownership of the image handler
		virtual unsigned int 	createObject(const char *name);
		virtual void clearAll(); //!< clear the whole environment + scene, i.e. free (hopefully) all memory.
		virtual void render(ColorOutput &output, ProgressBar *pb = nullptr); //!< render the scene...
		virtual bool startScene(int type = 0); //!< start a new scene; Must be called before any of the scene_t related callbacks!
		virtual bool setLoggingAndBadgeSettings();
		virtual bool setupRenderPasses(); //!< setup render passes information
		bool setInteractive(bool interactive);
		virtual void abort();
		virtual ParamMap *getRenderParameters() { return params_; }
		virtual bool getRenderedImage(int num_view, ColorOutput &output); //!< put the rendered image to output

		void setConsoleVerbosityLevel(const std::string &str_v_level);
		void setLogVerbosityLevel(const std::string &str_v_level);

		virtual void setParamsBadgePosition(const std::string &badge_position = "none");
		virtual bool getDrawParams();

		std::string getVersion() const; //!< Get version to check against the exporters

		/*! Console Printing wrappers to report in color with yafaray's own console coloring */
		void printDebug(const std::string &msg);
		void printVerbose(const std::string &msg);
		void printInfo(const std::string &msg);
		void printParams(const std::string &msg);
		void printWarning(const std::string &msg);
		void printError(const std::string &msg);

		void setInputColorSpace(std::string color_space_string, float gamma_val);
		void setOutput2(ColorOutput *out_2);

	protected:
		ParamMap *params_;
		std::list<ParamMap> *eparams_; //! for materials that need to define a whole shader tree etc.
		ParamMap *cparams_; //! just a pointer to the current paramMap, either params or a eparams element
		RenderEnvironment *env_;
		Scene *scene_;
		ImageFilm *film_;
		float input_gamma_;
		ColorSpace input_color_space_;
};

typedef Interface *InterfaceConstructor_t();


END_YAFARAY

#endif // YAFARAY_INTERFACE_H
