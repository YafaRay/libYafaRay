
#ifndef Y_YAFRAYINTERFACE_H
#define Y_YAFRAYINTERFACE_H

#include <yafray_constants.h>
#include <yaf_revision.h>
#include <list>
#include <vector>
#include <string>
#include <core_api/color.h>


__BEGIN_YAFRAY

class light_t;
class texture_t;
class material_t;
class camera_t;
class background_t;
class integrator_t;
class VolumeRegion;
class scene_t;
class renderEnvironment_t;
class colorOutput_t;
class paraMap_t;
class imageFilm_t;
class imageHandler_t;
class progressBar_t;
class matrix4x4_t;

class YAFRAYPLUGIN_EXPORT yafrayInterface_t
{
	public:
		yafrayInterface_t();
		virtual ~yafrayInterface_t();
		// directly related to scene_t:
		virtual void loadPlugins(const char *path); //!< load plugins from path, if NULL load from default path, if available.
		virtual bool startGeometry(); //!< call before creating geometry; only meshes and vmaps can be created in this state
		virtual bool endGeometry(); //!< call after creating geometry;
		/*! start a triangle mesh
			in this state only vertices, UVs and triangles can be created
			\param id returns the ID of the created mesh
		*/
		virtual unsigned int getNextFreeID();
		virtual bool startTriMesh(unsigned int id, int vertices, int triangles, bool hasOrco, bool hasUV=false, int type=0, int obj_pass_index=0);
		virtual bool startCurveMesh(unsigned int id, int vertices, int obj_pass_index=0);
		virtual bool startTriMeshPtr(unsigned int *id, int vertices, int triangles, bool hasOrco, bool hasUV=false, int type=0, int obj_pass_index=0);
		virtual bool endTriMesh(); //!< end current mesh and return to geometry state
		virtual bool endCurveMesh(const material_t *mat, float strandStart, float strandEnd, float strandShape); //!< end current mesh and return to geometry state
		virtual int  addVertex(double x, double y, double z); //!< add vertex to mesh; returns index to be used for addTriangle
		virtual int  addVertex(double x, double y, double z, double ox, double oy, double oz); //!< add vertex with Orco to mesh; returns index to be used for addTriangle
		virtual void addNormal(double nx, double ny, double nz); //!< add vertex normal to mesh; the vertex that will be attached to is the last one inserted by addVertex method
		virtual bool addTriangle(int a, int b, int c, const material_t *mat); //!< add a triangle given vertex indices and material pointer
		virtual bool addTriangle(int a, int b, int c, int uv_a, int uv_b, int uv_c, const material_t *mat); //!< add a triangle given vertex and uv indices and material pointer
		virtual int  addUV(float u, float v); //!< add a UV coordinate pair; returns index to be used for addTriangle
		virtual bool smoothMesh(unsigned int id, double angle); //!< smooth vertex normals of mesh with given ID and angle (in degrees)
		virtual bool addInstance(unsigned int baseObjectId, matrix4x4_t objToWorld);
		// functions to build paramMaps instead of passing them from Blender
		// (decouling implementation details of STL containers, paraMap_t etc. as much as possible)
		virtual void paramsSetPoint(const char* name, double x, double y, double z);
		virtual void paramsSetString(const char* name, const char* s);
		virtual void paramsSetBool(const char* name, bool b);
		virtual void paramsSetInt(const char* name, int i);
		virtual void paramsSetFloat(const char* name, double f);
		virtual void paramsSetColor(const char* name, float r, float g, float b, float a=1.f);
		virtual void paramsSetColor(const char* name, float *rgb, bool with_alpha=false);
		virtual void paramsSetMatrix(const char* name, float m[4][4], bool transpose=false);
		virtual void paramsSetMatrix(const char* name, double m[4][4], bool transpose=false);
		virtual void paramsSetMemMatrix(const char* name, float* matrix, bool transpose=false);
		virtual void paramsSetMemMatrix(const char* name, double* matrix, bool transpose=false);
		virtual void paramsClearAll(); 	//!< clear the paramMap and paramList
		virtual void paramsStartList(); //!< start writing parameters to the extended paramList (used by materials)
		virtual void paramsPushList(); 	//!< push new list item in paramList (e.g. new shader node description)
		virtual void paramsEndList(); 	//!< revert to writing to normal paramMap
		// functions directly related to renderEnvironment_t
		virtual light_t* 		createLight			(const char* name);
		virtual texture_t* 		createTexture		(const char* name);
		virtual material_t* 	createMaterial		(const char* name);
		virtual camera_t* 		createCamera		(const char* name);
		virtual background_t* 	createBackground	(const char* name);
		virtual integrator_t* 	createIntegrator	(const char* name);
		virtual VolumeRegion* 	createVolumeRegion	(const char* name);
		virtual imageHandler_t*	createImageHandler	(const char* name, bool addToTable = true); //!< The addToTable parameter, if true, allows to avoid the interface from taking ownership of the image handler
		virtual unsigned int 	createObject		(const char* name);
		virtual void clearAll(); //!< clear the whole environment + scene, i.e. free (hopefully) all memory.
		virtual void render(colorOutput_t &output, progressBar_t *pb = 0); //!< render the scene...
		virtual bool startScene(int type=0); //!< start a new scene; Must be called before any of the scene_t related callbacks!
		virtual bool setupRenderPasses(); //!< setup render passes information
		virtual void setInputGamma(float gammaVal, bool enable);	//deprecated: use setInputColorSpace instead
		virtual void abort();
		virtual paraMap_t* getRenderParameters() { return params; }
		virtual bool getRenderedImage(int numView, colorOutput_t &output); //!< put the rendered image to output
		virtual std::vector<std::string> listImageHandlers();
		virtual std::vector<std::string> listImageHandlersFullName();
		virtual std::string getImageFormatFromFullName(const std::string &fullname);
		virtual std::string getImageFullNameFromFormat(const std::string &format);
		
		virtual void setVerbosityLevel(int vlevel);
		virtual void setVerbosityInfo();
		virtual void setVerbosityWarning();
		virtual void setVerbosityError();
		virtual void setVerbosityMute();
		
		virtual void setDrawParams(bool on = true);
		virtual bool getDrawParams();

		virtual char* getVersion() const; //!< Get version to check aginst the exporters
		
		/*! Console Printing wrappers to report in color with yafaray's own console coloring */
		void printInfo(const std::string &msg);
		void printWarning(const std::string &msg);
		void printError(const std::string &msg);
		void printLog(const std::string &msg);
		
		void setInputColorSpace(std::string color_space_string, float gammaVal);
		void setOutput2(colorOutput_t *out2);
	
	protected:
		paraMap_t *params;
		std::list<paraMap_t> *eparams; //! for materials that need to define a whole shader tree etc.
		paraMap_t *cparams; //! just a pointer to the current paramMap, either params or a eparams element
		renderEnvironment_t *env;
		scene_t *scene;
		imageFilm_t *film;
		float inputGamma;
		colorSpaces_t inputColorSpace;
};

typedef yafrayInterface_t * interfaceConstructor();


__END_YAFRAY

#endif // Y_YAFRAYINTERFACE_H
