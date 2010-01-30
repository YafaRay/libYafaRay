%module yafrayinterface

%include "cpointer.i"
%pointer_functions(float, floatp);
%pointer_functions(int, intp);
%pointer_functions(unsigned int, uintp);

%include "carrays.i"
%include "std_string.i"
%array_functions(float, floatArray);


%{
#include <interface/yafrayinterface.h>
#include <interface/xmlinterface.h>
#include <yafraycore/tga_io.h>
#include <yafraycore/EXR_io.h>
#include <yafraycore/memoryIO.h>
using namespace yafaray;
%}

namespace yafaray {

class colorOutput_t
{
	public:
		virtual ~colorOutput_t() {};
		virtual bool putPixel(int x, int y, const float *c, int channels)=0;
		virtual void flush()=0;
		virtual void flushArea(int x0, int y0, int x1, int y1)=0;
};

class yafrayInterface_t
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
		virtual bool startTriMesh(unsigned int id, int vertices, int triangles, bool hasOrco, bool hasUV=false, int type=0);
		virtual bool startCurveMesh(unsigned int id, int vertices);
		virtual bool startTriMeshPtr(unsigned int *id, int vertices, int triangles, bool hasOrco, bool hasUV=false, int type=0);
		virtual unsigned int getNextFreeID();
		virtual bool endTriMesh(); //!< end current mesh and return to geometry state
		virtual bool endCurveMesh(const material_t *mat, float strandStart, float strandEnd, float strandShape); //!< end current mesh and return to geometry state
		virtual int  addVertex(double x, double y, double z); //!< add vertex to mesh; returns index to be used for addTriangle
		virtual int  addVertex(double x, double y, double z, double ox, double oy, double oz); //!< add vertex with Orco to mesh; returns index to be used for addTriangle
		virtual bool addTriangle(int a, int b, int c, const material_t *mat); //!< add a triangle given vertex indices and material pointer
		virtual bool addTriangle(int a, int b, int c, int uv_a, int uv_b, int uv_c, const material_t *mat); //!< add a triangle given vertex and uv indices and material pointer
		virtual int  addUV(float u, float v); //!< add a UV coordinate pair; returns index to be used for addTriangle
		virtual bool startVmap(int id, int type, int dimensions); //!< start a vertex map of given type and dimension; gets added to last created mesh
		virtual bool endVmap(); //!< finish editing current vertex map and return to geometry state
		virtual bool addVmapValues(float *val); //!< add vertex map values; val must point to array of 3*dimension floats (one triangle)
		virtual bool smoothMesh(unsigned int id, double angle); //!< smooth vertex normals of mesh with given ID and angle (in degrees)
		// functions to build paramMaps instead of passing them from Blender
		// (decouling implementation details of STL containers, paramMap_t etc. as much as possible)
		virtual void paramsSetPoint(const char* name, double x, double y, double z);
		virtual void paramsSetString(const char* name, const char* s);
		virtual void paramsSetBool(const char* name, bool b);
		virtual void paramsSetInt(const char* name, int i);
		virtual void paramsSetFloat(const char* name, double f);
		virtual void paramsSetColor(const char* name, float r, float g, float b, float a=1.f);
		virtual void paramsSetColor(const char* name, float *rgb, bool with_alpha=false);
		virtual void paramsSetMatrix(const char* name, float m[4][4], bool transpose=false);
		virtual void paramsSetMatrix(const char* name, double m[4][4], bool transpose=false);
		virtual void paramsSetMemMatrix(const char* name, float *matrix, bool transpose=false);
		virtual void paramsSetMemMatrix(const char* name, double *matrix, bool transpose=false);
		virtual void paramsClearAll(); 	//!< clear the paramMap and paramList
		virtual void paramsStartList(); //!< start writing parameters to the extended paramList (used by materials)
		virtual void paramsPushList(); 	//!< push new list item in paramList (e.g. new shader node description)
		virtual void paramsEndList(); 	//!< revert to writing to normal paramMap
		// functions directly related to renderEnvironment_t
		virtual light_t* 		createLight		(const char* name);
		virtual texture_t* 		createTexture	(const char* name);
		virtual material_t* 	createMaterial	(const char* name);
		virtual camera_t* 		createCamera	(const char* name);
		virtual background_t* 	createBackground(const char* name);
		virtual integrator_t* 	createIntegrator(const char* name);
		virtual VolumeRegion* 	createVolumeRegion(const char* name);
		virtual unsigned int 	createObject	(const char* name);
		virtual void clearAll(); //!< clear the whole environment + scene, i.e. free (hopefully) all memory.
		virtual void render(colorOutput_t &output); //!< render the scene...
		virtual bool startScene(int type=0); //!< start a new scene; Must be called before any of the scene_t related callbacks!
		virtual void setInputGamma(float gammaVal, bool enable);
		virtual void addToParamsString(const char* params);
		virtual void clearParamsString();
		virtual void setDrawParams(bool b);
		virtual void abort();
		virtual bool getRenderedImage(colorOutput_t &output);
		virtual char* getVersion() const;
			
	protected:
		paramMap_t *params;
		std::list<paramMap_t> *eparams; //! for materials that need to define a whole shader tree etc.
		paramMap_t *cparams; //! just a pointer to the current paramMap, either params or a eparams element
		renderEnvironment_t *env;
		scene_t *scene;
		imageFilm_t *film;
		float inputGamma;
		bool gcInput;
};


class xmlInterface_t: public yafrayInterface_t
{
	public:
		xmlInterface_t();
		// directly related to scene_t:
		virtual void loadPlugins(const char *path);
		virtual bool startGeometry();
		virtual bool endGeometry();
		virtual unsigned int getNextFreeID();
		virtual bool startTriMesh(unsigned int id, int vertices, int triangles, bool hasOrco, bool hasUV=false, int type=0);
		virtual bool startCurveMesh(unsigned int id, int vertices);
		virtual bool startTriMeshPtr(unsigned int *id, int vertices, int triangles, bool hasOrco, bool hasUV=false, int type=0);
		virtual bool endTriMesh();
		virtual bool endCurveMesh(const material_t *mat, float strandStart, float strandEnd, float strandShape); //!< end current mesh and return to geometry state
		virtual int  addVertex(double x, double y, double z);
		virtual int  addVertex(double x, double y, double z, double ox, double oy, double oz);
		virtual bool addTriangle(int a, int b, int c, const material_t *mat);
		virtual bool addTriangle(int a, int b, int c, int uv_a, int uv_b, int uv_c, const material_t *mat);
		virtual int  addUV(float u, float v);
		virtual bool startVmap(int id, int type, int dimensions);
		virtual bool endVmap();
		virtual bool addVmapValues(float *val);
		virtual bool smoothMesh(unsigned int id, double angle);
		
		// functions directly related to renderEnvironment_t
		virtual light_t* 		createLight		(const char* name);
		virtual texture_t* 		createTexture	(const char* name);
		virtual material_t* 	createMaterial	(const char* name);
		virtual camera_t* 		createCamera	(const char* name);
		virtual background_t* 	createBackground(const char* name);
		virtual integrator_t* 	createIntegrator(const char* name);
		virtual unsigned int 	createObject	(const char* name);
		virtual void clearAll(); //!< clear the whole environment + scene, i.e. free (hopefully) all memory.
		virtual void render(colorOutput_t &output); //!< render the scene...
		virtual bool startScene(int type=0); //!< start a new scene; Must be called before any of the scene_t related callbacks!
		
		virtual void setOutfile(const char *fname);
	protected:
		void writeParamMap(const paramMap_t &pmap, int indent=1);
		void writeParamList(int indent);
		
		std::map<const material_t *, std::string> materials;
		std::ofstream xmlFile;
		std::string xmlName;
		const material_t *last_mat;
		size_t nmat;
		int n_uvs;
		unsigned int nextObj;
};


class outTga_t : public colorOutput_t
{
	public:
		outTga_t(int resx, int resy, const char *fname, bool sv_alpha=false);
		//virtual bool putPixel(int x, int y, const color_t &c, 
		//		CFLOAT alpha=0,PFLOAT depth=0);
		virtual bool putPixel(int x, int y, const float *c, int channels);
		virtual void flush() { savetga(outfile.c_str()); }
		virtual void flushArea(int x0, int y0, int x1, int y1) {}; // no tiled file format...useless
		virtual ~outTga_t();
	protected:
		outTga_t(const outTga_t &o) {}; //forbidden
		bool savetga(const char* filename);
		bool save_alpha;
		unsigned char *data;
		unsigned char *alpha_buf;
		int sizex, sizey;
		std::string outfile;
};

class outEXR_t : public colorOutput_t
{
	public:
		outEXR_t(int resx, int resy, const char *fname, const std::string &exr_flags);
		//virtual bool putPixel(int x, int y, const color_t &c, 
		//		CFLOAT alpha=0,PFLOAT depth=0);
		virtual bool putPixel(int x, int y, const float *c, int channels);
		virtual void flush() { saveEXR(); }
		virtual void flushArea(int x0, int y0, int x1, int y1) {}; // no tiled file format...useless
		virtual ~outEXR_t();
	protected:
		outEXR_t(const outEXR_t &o) {}; //forbidden
		bool saveEXR();
		fcBuffer_t* fbuf;
		gBuf_t<float, 1>* zbuf;
		int sizex, sizey;
		const char* filename;
		std::string out_flags;
};

class memoryIO_t : public colorOutput_t
{
	public:
		memoryIO_t(int resx, int resy, float* iMem);
		virtual bool putPixel(int x, int y, const float *c, int channels);
		void flush();
		virtual void flushArea(int x0, int y0, int x1, int y1) {}; // no tiled file format used...yet
		virtual ~memoryIO_t();
	protected:
		int sizex, sizey;
		float* imageMem;
};

}
