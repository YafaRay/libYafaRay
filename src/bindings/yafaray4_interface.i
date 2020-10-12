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

#if defined(SWIGRUBY)
  %module yafaray4_interface_ruby
#else
  %module yafaray4_interface
#endif

%include "cpointer.i"
%pointer_functions(float, floatp);
%pointer_functions(int, intp);
%pointer_functions(unsigned int, uintp);

%include "carrays.i"
%include "std_string.i"
%include "std_vector.i"

%array_functions(float, floatArray);

#ifdef SWIGPYTHON  // Begining of python specific code

%{
#include <sstream>
#include "render/monitor.h"
#include "output/output.h"
#include "interface/interface.h"
#include "color/color_layers.h"

struct YafTilePixel
{
	float r;
	float g;
	float b;
	float a;
};

struct YafTileObject
{
	PyObject_HEAD
	int resx, resy;
	int x0, x1, y0, y1;
	int w, h;
	int color_components; //RGBA (4), RGB (3) or Grayscale (1). Grayscale would use component "r" only for the grayscale value.
	YafTilePixel *mem;
};


static Py_ssize_t yaf_tile_length(YafTileObject *self)
{
	self->w = (self->x1 - self->x0);
	self->h = (self->y1 - self->y0);

	return self->w * self->h;
}


static PyObject *yaf_tile_subscript_int(YafTileObject *self, int keynum)
{
	// Check boundaries and fill w and h
	if (keynum >= yaf_tile_length(self) || keynum < 0)
	{
		PyObject* groupPix = PyTuple_New(self->color_components);
		for(int i = 0; i < self->color_components; ++i)
		{
			PyTuple_SET_ITEM(groupPix, i, PyFloat_FromDouble((i == 3 ? 1.f : 0.f)));
		}
		return groupPix;
	}

	// Calc position of the tile in the image region
	int vy = keynum / self->w;
	int vx = keynum - vy * self->w;

	// Map tile position to image buffer
	vx = self->x0 + vx;
	vy = (self->y0 + self->h - 1) - vy;

	// Get pixel
	YafTilePixel &pix = self->mem[ self->resx * vy + vx ];

	PyObject* groupPix = PyTuple_New(self->color_components);
	if(self->color_components >= 1)
	{
		PyTuple_SET_ITEM(groupPix, 0, PyFloat_FromDouble(pix.r));
	}
	if(self->color_components >= 3)
	{
		PyTuple_SET_ITEM(groupPix, 1, PyFloat_FromDouble(pix.g));
		PyTuple_SET_ITEM(groupPix, 2, PyFloat_FromDouble(pix.b));
	}
	if(self->color_components == 4)
	{
		PyTuple_SET_ITEM(groupPix, 3, PyFloat_FromDouble(pix.a));
	}
	return groupPix;
}

static void yafTileDealloc__(YafTileObject *self)
{
	SWIG_PYTHON_THREAD_BEGIN_BLOCK; 
	PyObject_Del(self);
	SWIG_PYTHON_THREAD_END_BLOCK; 
}

PySequenceMethods sequence_methods =
{
	( lenfunc ) yaf_tile_length,
	nullptr,
	nullptr,
	( ssizeargfunc ) yaf_tile_subscript_int
};

PyTypeObject yaf_tile_type =
{
	PyVarObject_HEAD_INIT(nullptr, 0)
	"yaf_tile",							/* tp_name */
	sizeof(YafTileObject),			/* tp_basicsize */
	0,									/* tp_itemsize */
	( destructor ) yafTileDealloc__,	/* tp_dealloc */
	0,                             		/* tp_print / tp_vectorcall_offset */
	nullptr,								/* getattrfunc tp_getattr; */
	nullptr,								/* setattrfunc tp_setattr; */
	nullptr,								/* tp_compare */ /* DEPRECATED in python 3.0! */
	nullptr,								/* tp_repr */
	nullptr,                       		/* PyNumberMethods *tp_as_number; */
	&sequence_methods,					/* PySequenceMethods *tp_as_sequence; */
	nullptr,								/* PyMappingMethods *tp_as_mapping; */
	nullptr,								/* hashfunc tp_hash; */
	nullptr,								/* ternaryfunc tp_call; */
	nullptr,                       		/* reprfunc tp_str; */
	nullptr,								/* getattrofunc tp_getattro; */
	nullptr,								/* setattrofunc tp_setattro; */
	nullptr,                       		/* PyBufferProcs *tp_as_buffer; */
	Py_TPFLAGS_DEFAULT,         		/* long tp_flags; */
};


class YafPyOutput : public yafaray4::ColorOutput
{

public:

	YafPyOutput(int x, int y, int borderStartX, int borderStartY, bool prev, PyObject *drawAreaCallback, PyObject *flushCallback) :
	resx(x),
	resy(y),
	bsX(borderStartX),
	bsY(borderStartY),
	preview(prev),
	mDrawArea(drawAreaCallback),
	mFlush(flushCallback)
	{
	}

    virtual void initLayers(int totalViews, int numRenderLayers)
    { 
		SWIG_PYTHON_THREAD_BEGIN_BLOCK; 
		PyGILState_STATE gstate;
		gstate = PyGILState_Ensure();

		tiles_layers.resize(totalViews);
    
        for(size_t view = 0; view < tiles_layers.size(); ++view)
		{
			for(int idx = 0; idx < numRenderLayers; ++idx)
			{
				YafTileObject* tile = PyObject_New(YafTileObject, &yaf_tile_type);
				tiles_layers.at(view).push_back(tile);
				tiles_layers.at(view)[idx]->mem = new YafTilePixel[resx*resy];
				tiles_layers.at(view)[idx]->resx = resx;
				tiles_layers.at(view)[idx]->resy = resy;
			}
		}
		
		PyGILState_Release(gstate);
		SWIG_PYTHON_THREAD_END_BLOCK; 
    }

	virtual ~YafPyOutput() override
	{
		SWIG_PYTHON_THREAD_BEGIN_BLOCK; 
		PyGILState_STATE gstate;
		gstate = PyGILState_Ensure();

		for(size_t view = 0; view < tiles_layers.size(); ++view)
		{
			for(size_t idx = 0; idx < tiles_layers.at(view).size(); ++idx)
			{
				if(tiles_layers.at(view)[idx]->mem) delete [] tiles_layers.at(view)[idx]->mem;
				//Py_XDECREF(tiles_layers.at(view)[idx]);
			}
			tiles_layers.at(view).clear();
		}
		tiles_layers.clear();
		
		PyGILState_Release(gstate);
		SWIG_PYTHON_THREAD_END_BLOCK; 
	}

	virtual bool putPixel(int x, int y, const yafaray4::ColorLayer &color_layer) override
	{
		if(render_layer < (int) tiles_layers.at(num_view).size())
		{
			YafTilePixel &pix = tiles_layers.at(num_view)[render_layer]->mem[resx * y + x];
			pix = color;
		}
		return true;
	}

	virtual bool isPreview() const override { return preview; }

	virtual void flush() override
	{
		SWIG_PYTHON_THREAD_BEGIN_BLOCK; 
		PyGILState_STATE gstate;
		gstate = PyGILState_Ensure();
		
		PyObject* groupTile = PyTuple_New(tiles_layers.size() * tiles_layers.at(0).size());

		for(size_t view = 0; view < tiles_layers.size(); ++view)
		{
			std::string view_name = render_layers_->view_names_.at(view);

			for(size_t idx = 0; idx < tiles_layers.at(view).size(); ++idx)
			{
				tiles_layers.at(view)[idx]->x0 = 0;
				tiles_layers.at(view)[idx]->x1 = resx;
				tiles_layers.at(view)[idx]->y0 = 0;
				tiles_layers.at(view)[idx]->y1 = resy;
				
				tiles_layers.at(view)[idx]->color_components = render_layers_->getNumChannels(idx);

				std::stringstream extPassName;
				extPassName << (*render_layers_(idx)).getName();
				PyObject* groupItem = Py_BuildValue("ssO", view_name.c_str(), extPassName.str().c_str(), tiles_layers.at(view)[idx]);
				PyTuple_SET_ITEM(groupTile, tiles_layers.at(view).size()*view + idx, (PyObject*) groupItem);

				//std::cout << "flush: groupItem->ob_refcnt=" << groupItem->ob_refcnt << std::endl;
			}
		}		
		PyObject* result = PyObject_CallFunction(mFlush, "iiiO", resx, resy, 0, groupTile);

		Py_XDECREF(result);
		Py_XDECREF(groupTile);
		
		//std::cout << "flush: result->ob_refcnt=" << result->ob_refcnt << std::endl;
		//std::cout << "flush: groupTile->ob_refcnt=" << groupTile->ob_refcnt << std::endl;

		PyGILState_Release(gstate);
		SWIG_PYTHON_THREAD_END_BLOCK; 
	}

	virtual void flushArea(int x0, int y0, int x1, int y1) override
	{
		SWIG_PYTHON_THREAD_BEGIN_BLOCK; 
		std::string view_name = std::to_string(num_view); //FIXME! render_layers->view_names_.at(num_view);
		
		// Do nothing if we are rendering preview renders
		if(preview) return;

		int w = x1 - x0;
		int h = y1 - y0;

		PyGILState_STATE gstate;
		gstate = PyGILState_Ensure();

		PyObject* groupTile = PyTuple_New(tiles_layers.at(num_view).size());

		for(size_t idx = 0; idx < tiles_layers.at(num_view).size(); ++idx)
		{
			tiles_layers.at(num_view)[idx]->x0 = x0 - bsX;
			tiles_layers.at(num_view)[idx]->x1 = x1 - bsX;
			tiles_layers.at(num_view)[idx]->y0 = y0 - bsY;
			tiles_layers.at(num_view)[idx]->y1 = y1 - bsY;
			
			tiles_layers.at(num_view)[idx]->color_components = render_layers_->getNumChannels(idx);

			std::stringstream extPassName;
			extPassName << (*render_layers_(idx)).getName();
			PyObject* groupItem = Py_BuildValue("ssO", view_name.c_str(), extPassName.str().c_str(), tiles_layers.at(num_view)[idx]);
			PyTuple_SET_ITEM(groupTile, idx, (PyObject*) groupItem);

			//std::cout << "flushArea: groupItem->ob_refcnt=" << groupItem->ob_refcnt << std::endl;
		}

		PyObject* result = PyObject_CallFunction(mDrawArea, "iiiiiO", tiles_layers.at(num_view)[0]->x0, resy - tiles_layers.at(num_view)[0]->y1, w, h, num_view, groupTile);
		
		Py_XDECREF(result);
		Py_XDECREF(groupTile);
		//std::cout << "flushArea: result->ob_refcnt=" << result->ob_refcnt << std::endl;
		//std::cout << "flushArea: groupTile->ob_refcnt=" << groupTile->ob_refcnt << std::endl;
		
		PyGILState_Release(gstate);
		SWIG_PYTHON_THREAD_END_BLOCK; 
	}

	virtual void highlightArea(int x0, int y0, int x1, int y1) override
	{
		SWIG_PYTHON_THREAD_BEGIN_BLOCK; 
		std::string view_name = "";
		
		// Do nothing if we are rendering preview renders
		if(preview) return;

		tiles_layers.at(num_view)[0]->x0 = x0 - bsX;
		tiles_layers.at(num_view)[0]->x1 = x1 - bsX;
		tiles_layers.at(num_view)[0]->y0 = y0 - bsY;
		tiles_layers.at(num_view)[0]->y1 = y1 - bsY;

		int w = x1 - x0;
		int h = y1 - y0;
		int lineL = std::min( 4, std::min( h - 1, w - 1 ) );

		drawCorner(num_view, tiles_layers.at(num_view)[0]->x0, tiles_layers.at(num_view)[0]->y0, lineL, TL_CORNER);
		drawCorner(num_view, tiles_layers.at(num_view)[0]->x1, tiles_layers.at(num_view)[0]->y0, lineL, TR_CORNER);
		drawCorner(num_view, tiles_layers.at(num_view)[0]->x0, tiles_layers.at(num_view)[0]->y1, lineL, BL_CORNER);
		drawCorner(num_view, tiles_layers.at(num_view)[0]->x1, tiles_layers.at(num_view)[0]->y1, lineL, BR_CORNER);

		PyGILState_STATE gstate;
		gstate = PyGILState_Ensure();
		

		PyObject* groupTile = PyTuple_New(1);

		tiles_layers.at(num_view)[0]->color_components = 4;
		
		PyObject* groupItem = Py_BuildValue("ssO", view_name.c_str(), "Combined", tiles_layers.at(num_view)[0]);
		PyTuple_SET_ITEM(groupTile, 0, (PyObject*) groupItem);

		//std::cout << "highlightArea: groupItem->ob_refcnt=" << groupItem->ob_refcnt << std::endl;
		
		PyObject* result = PyObject_CallFunction(mDrawArea, "iiiiiO", tiles_layers.at(num_view)[0]->x0, resy - tiles_layers.at(num_view)[0]->y1, w, h, num_view, groupTile);
		
		Py_XDECREF(result);
		Py_XDECREF(groupTile);

		//std::cout << "highlightArea: result->ob_refcnt=" << result->ob_refcnt << std::endl;
		//std::cout << "highlightArea: groupTile->ob_refcnt=" << groupTile->ob_refcnt << std::endl;

		PyGILState_Release(gstate);
		SWIG_PYTHON_THREAD_END_BLOCK; 
	}

private:

	enum corner
	{
		TL_CORNER,
		TR_CORNER,
		BL_CORNER,
		BR_CORNER
	};

	void drawCorner(int num_view, int x, int y, int len, corner pos)
	{
		int minX = 0;
		int minY = 0;
		int maxX = 0;
		int maxY = 0;

		switch(pos)
		{
			case TL_CORNER:
				minX = x;
				minY = y;
				maxX = x + len;
				maxY = y + len;
				break;

			case TR_CORNER:
				minX = x - len - 1;
				minY = y;
				maxX = x - 1;
				maxY = y + len;
				--x;
				break;

			case BL_CORNER:
				minX = x;
				minY = y - len - 1;
				maxX = x + len;
				maxY = y - 1;
				--y;
				break;

			case BR_CORNER:
				minX = x - len - 1;
				minY = y - len - 1;
				maxX = x;
				maxY = y - 1;
				--x;
				--y;
				break;
		}

		for(int i = minX; i < maxX; ++i)
		{
			YafTilePixel &pix = tiles_layers.at(num_view)[0]->mem[resx * y + i];
			pix.r = 0.625f;
			pix.g = 0.f;
			pix.b = 0.f;
			pix.a = 1.f;
		}

		for(int j = minY; j < maxY; ++j)
		{
			YafTilePixel &pix = tiles_layers.at(num_view)[0]->mem[resx * j + x];
			pix.r = 0.625f;
			pix.g = 0.f;
			pix.b = 0.f;
			pix.a = 1.f;
		}
	}

	int resx, resy;
	int bsX, bsY;
	bool preview;
	PyObject *mDrawArea;
	PyObject *mFlush;
	std::vector< std::vector<YafTileObject*> > tiles_layers;
};

class YafPyProgress : public yafaray4::ProgressBar
{

public:

	YafPyProgress(PyObject *callback) : callb(callback) {}

	void report_progress(float percent)
	{
		PyGILState_STATE gstate;
		gstate = PyGILState_Ensure();
		PyObject* result = PyObject_CallFunction(callb, "sf", "progress", percent);
		Py_XDECREF(result);
		//std::cout << "report_progress: result->ob_refcnt=" << result->ob_refcnt << std::endl;
		PyGILState_Release(gstate);
	}

	virtual void init(int totalSteps)
	{
		nSteps = totalSteps;
		steps_to_percent = 1.f / (float) nSteps;
		doneSteps = 0;
		report_progress(0.f);
	}

	virtual void update(int steps = 1)
	{
		doneSteps += steps;
		report_progress(doneSteps * steps_to_percent);
	}

	virtual void done()
	{
		report_progress(1.f);
	}

	virtual void setTag(const char* text)
	{
		tag = std::string(text);
		PyGILState_STATE gstate;
		gstate = PyGILState_Ensure();
		PyObject* result = PyObject_CallFunction(callb, "ss", "tag", text);
		Py_XDECREF(result);
		//std::cout << "setTag: result->ob_refcnt=" << result->ob_refcnt << std::endl;
		PyGILState_Release(gstate);
	}

	virtual void setTag(std::string text)
	{
		tag = text;
		PyGILState_STATE gstate;
		gstate = PyGILState_Ensure();
		PyObject* result = PyObject_CallFunction(callb, "ss", "tag", text.c_str());
		Py_XDECREF(result);
		//std::cout << "setTag: result->ob_refcnt=" << result->ob_refcnt << std::endl;
		PyGILState_Release(gstate);
	}
	
	virtual std::string getTag() const
	{
		return tag;
	}
	
	virtual float getPercent() const { return 100.f * std::min(1.f, (float) doneSteps * steps_to_percent); }
	virtual float getTotalSteps() const { return nSteps; } 

private:

	PyObject *callb;
	float steps_to_percent;
	int doneSteps, nSteps;
	std::string tag;
};

%}

%init %{
	PyType_Ready(&yaf_tile_type);
%}

%typemap(in) PyObject *pyfunc
{
	if (!PyCallable_Check($input))
	{
		PyErr_SetString(PyExc_TypeError, "Need a callback method.");
		return nullptr;
	}

	$1 = $input;
}

%extend yafaray4::Interface
{
	void render(int x, int y, int borderStartX, int borderStartY, bool prev, PyObject *drawAreaCallBack, PyObject *flushCallBack, PyObject *progressCallback)
	{
		YafPyOutput output_wrap(x, y, borderStartX, borderStartY, prev, drawAreaCallBack, flushCallBack);
		YafPyProgress *pbar_wrap = new YafPyProgress(progressCallback);

		Py_BEGIN_ALLOW_THREADS;
		self->render(output_wrap, pbar_wrap);
		Py_END_ALLOW_THREADS;
	}
}

#endif // End of python specific code

%{
#include "constants.h"
#include "interface/interface.h"
#include "export/export_xml.h"
#include "output/output_image.h"
#include "output/output_memory.h"
#include "geometry/matrix4.h"
#include "format/format.h"
using namespace yafaray4;
%}

#ifdef SWIGRUBY
%feature("director") colorOutput_t::putPixel;
%feature("director") colorOutput_t::flush;
%feature("director") colorOutput_t::flushArea;
#endif

namespace yafaray4
{
	// Required abstracts

	class ColorOutput
	{
		public:
		virtual ~ColorOutput() = default;
		virtual bool putPixel(int x, int y, const ColorLayer &color_layer) = 0;
		virtual void flush() = 0;
		virtual void flushArea(int x_0, int y_0, int x_1, int y_1) = 0;
		virtual void highlightArea(int x_0, int y_0, int x_1, int y_1) {};
		virtual bool isImageOutput() const { return false; }
		virtual bool isPreview() const { return false; }
		virtual std::string getDenoiseParams() const { return ""; }
	};

	class Format
	{
		public:
		static Format *factory(ParamMap &params);
		virtual ~Format() = default;
		virtual Image *loadFromFile(const std::string &name) = 0;
		virtual Image *loadFromMemory(const uint8_t *data, size_t size) {return nullptr; }
		virtual bool saveToFile(const std::string &name, const Image *image) = 0;
		virtual bool saveToFileMultiChannel(const std::string &name, const ImageLayers *image_layers) { return false; };
		virtual bool isHdr() const { return false; }
		bool isMultiLayer() const { return multi_layer_; }
		bool denoiseEnabled() const { return denoise_; }
		void setGrayScaleSetting(bool grayscale) { grayscale_ = grayscale; }
		std::string getDenoiseParams() const;
		void initForOutput(int width, int height, const ImageLayers &render_layers, bool denoise_enabled, int denoise_h_lum, int denoise_h_col, float denoise_mix, bool with_alpha = false, bool multi_layer = false, bool grayscale = false);
	};
	// Outputs

	class ImageOutput : public ColorOutput
	{
		public:
		ImageOutput(Format *handle, const std::string &name, int bx, int by);
		private:
		virtual bool putPixel(int num_view, int x, int y, int render_layer, const Rgba &color, bool alpha = true) override;
		virtual bool putPixel(int num_view, int x, int y, const std::vector<Rgba> &colors, bool alpha = true) override;
		virtual void flush(int num_view) override;
		virtual void flushArea(int num_view, int x_0, int y_0, int x_1, int y_1) override {} // not used by images... yet
	};

	// Utility classes

	class Matrix4
	{
		public:
		Matrix4() = default;
		Matrix4(const float init);
		Matrix4(const Matrix4 &source);
		Matrix4(const float source[4][4]);
		Matrix4(const double source[4][4]);
		/*! attention, this function can cause the matrix to become invalid!
			unless you are sure the matrix is invertible, check invalid() afterwards! */
		Matrix4 &inverse();
		Matrix4 &transpose();
		void identity();
		void translate(float dx, float dy, float dz);
		void rotateX(float degrees);
		void rotateY(float degrees);
		void rotateZ(float degrees);
		void scale(float sx, float sy, float sz);
		int invalid() const { return invalid_; }
		//const float *operator [](int i) const { return matrix_[i]; }
		//float *operator [](int i) { return matrix_[i]; }
		void setVal(int row, int col, float val) { matrix_[row][col] = val; };
		float getVal(int row, int col) { return matrix_[row][col]; }
	};

	// Interfaces
	class Interface
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
		virtual bool startTriMesh(const char *name, int vertices, int triangles, bool has_orco, bool has_uv = false, int type = 0, int obj_pass_index = 0);
		virtual bool startCurveMesh(const char *name, int vertices, int obj_pass_index = 0);
		virtual bool endTriMesh(); //!< end current mesh and return to geometry state
		virtual bool endCurveMesh(const Material *mat, float strand_start, float strand_end, float strand_shape); //!< end current mesh and return to geometry state
		virtual int  addVertex(double x, double y, double z); //!< add vertex to mesh; returns index to be used for addTriangle
		virtual int  addVertex(double x, double y, double z, double ox, double oy, double oz); //!< add vertex with Orco to mesh; returns index to be used for addTriangle
		virtual void addNormal(double nx, double ny, double nz); //!< add vertex normal to mesh; the vertex that will be attached to is the last one inserted by addVertex method
		virtual bool addTriangle(int a, int b, int c, const Material *mat); //!< add a triangle given vertex indices and material pointer
		virtual bool addTriangle(int a, int b, int c, int uv_a, int uv_b, int uv_c, const Material *mat); //!< add a triangle given vertex and uv indices and material pointer
		virtual int  addUv(float u, float v); //!< add a UV coordinate pair; returns index to be used for addTriangle
		virtual bool smoothMesh(const char *name, double angle); //!< smooth vertex normals of mesh with given ID and angle (in degrees)
		virtual bool addInstance(const char *base_object_name, Matrix4 obj_to_world);
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
		// functions directly related to Scene_t
		virtual Light 		*createLight(const char *name);
		virtual Texture 		*createTexture(const char *name);
		virtual Material 	*createMaterial(const char *name);
		virtual Camera 		*createCamera(const char *name);
		virtual Background 	*createBackground(const char *name);
		virtual Integrator 	*createIntegrator(const char *name);
		virtual VolumeRegion 	*createVolumeRegion(const char *name);
		virtual Format	*createImageHandler(const char *name, bool add_to_table = true);  //!< The addToTable parameter, if true, allows to avoid the interface from taking ownership of the image handler
		virtual unsigned int 	createObject(const char *name);
		virtual void clearAll(); //!< clear the whole environment + scene, i.e. free (hopefully) all memory.
		virtual void render(ProgressBar *pb = nullptr); //!< render the scene...
		virtual bool setLoggingAndBadgeSettings();
		void addLayer(const std::string &layer_type_name, const std::string &exported_image_type_name);
		virtual bool setupLayers();
		bool setInteractive(bool interactive);
		virtual void abort();
		virtual ParamMap *getRenderParameters() { return params_; }

		void setConsoleVerbosityLevel(const std::string &str_v_level);
		void setLogVerbosityLevel(const std::string &str_v_level);

		virtual void setParamsBadgePosition(const std::string &badge_position = "none");
		virtual bool getDrawParams();

		std::string getVersion() const; //!< Get version to check against the exporters

		/*! Console Printing wrappers to report in color with yafaray's own console coloring */
		void printDebug(const std::string &msg) const;
		void printVerbose(const std::string &msg) const;
		void printInfo(const std::string &msg) const;
		void printParams(const std::string &msg) const;
		void printWarning(const std::string &msg) const;
		void printError(const std::string &msg) const;

		void setInputColorSpace(std::string color_space_string, float gamma_val);
	};

	class XmlExport: public Interface
	{
		public:
		XmlExport();
		// directly related to scene_t:
		virtual bool setLoggingAndBadgeSettings() override;
		virtual bool setupLayers() override; //!< setup render passes information
		virtual bool startGeometry() override;
		virtual bool endGeometry() override;
		virtual unsigned int getNextFreeId() override;
		virtual bool startTriMesh(const char *name, int vertices, int triangles, bool has_orco, bool has_uv = false, int type = 0, int obj_pass_index = 0) override;
		virtual bool startCurveMesh(const char *name, int vertices, int obj_pass_index = 0) override;
		virtual bool endTriMesh() override;
		virtual bool addInstance(const char *base_object_name, Matrix4 obj_to_world) override;
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
		virtual unsigned int 	createObject(const char *name) override;
		virtual void clearAll() override; //!< clear the whole environment + scene, i.e. free (hopefully) all memory.
		virtual void render(ProgressBar *pb = nullptr) override; //!< render the scene...
		void setOutfile(const char *fname);
		void setXmlColorSpace(std::string color_space_string, float gamma_val);
	};

}
