%module yafaray_e3_interface

%include "cpointer.i"
%pointer_functions(float, floatp);
%pointer_functions(int, intp);
%pointer_functions(unsigned int, uintp);

%include "carrays.i"
%include "std_string.i"
%include "std_vector.i"

%array_functions(float, floatArray);

namespace std
{
	%template(StrVector) vector<string>;
}

#ifdef SWIGPYTHON  // Begining of python specific code

%{
#include <sstream>
#include <yafraycore/monitor.h>
#include <core_api/output.h>
#include <interface/yafrayinterface.h>
#include <core_api/renderpasses.h>

struct yafTilePixel_t
{
	float r;
	float g;
	float b;
	float a;
};

struct YafTileObject_t
{
	PyObject_HEAD
	int resx, resy;
	int x0, x1, y0, y1;
	int w, h;
	yafTilePixel_t *mem;
	int tileType; //RGBA (4), RGB (3) or Grayscale (1). Grayscale would use component "r" only for the grayscale value.
};


static Py_ssize_t yaf_tile_length(YafTileObject_t *self)
{
	self->w = (self->x1 - self->x0);
	self->h = (self->y1 - self->y0);

	return self->w * self->h;
}


static PyObject *yaf_tile_subscript_int(YafTileObject_t *self, int keynum)
{
	// Check boundaries and fill w and h
	if (keynum >= yaf_tile_length(self) || keynum < 0)
	{
		if(self->tileType == yafaray::PASS_EXT_TILE_1_GRAYSCALE) 
		{
			PyObject* groupPix = PyTuple_New(1);
			PyTuple_SET_ITEM(groupPix, 0, PyFloat_FromDouble(0.f));
			return groupPix;
		}
		else if(self->tileType == yafaray::PASS_EXT_TILE_3_RGB) 
		{
			PyObject* groupPix = PyTuple_New(3);
			PyTuple_SET_ITEM(groupPix, 0, PyFloat_FromDouble(0.f));
			PyTuple_SET_ITEM(groupPix, 1, PyFloat_FromDouble(0.f));
			PyTuple_SET_ITEM(groupPix, 2, PyFloat_FromDouble(0.f));
			return groupPix;
		}
		else
		{
			PyObject* groupPix = PyTuple_New(4);
			PyTuple_SET_ITEM(groupPix, 0, PyFloat_FromDouble(0.f));
			PyTuple_SET_ITEM(groupPix, 1, PyFloat_FromDouble(0.f));
			PyTuple_SET_ITEM(groupPix, 2, PyFloat_FromDouble(0.f));
			PyTuple_SET_ITEM(groupPix, 3, PyFloat_FromDouble(1.f));
			return groupPix;
		}
	}

	// Calc position of the tile in the image region
	int vy = keynum / self->w;
	int vx = keynum - vy * self->w;

	// Map tile position to image buffer
	vx = self->x0 + vx;
	vy = (self->y0 + self->h - 1) - vy;

	// Get pixel
	yafTilePixel_t &pix = self->mem[ self->resx * vy + vx ];

	if(self->tileType == yafaray::PASS_EXT_TILE_1_GRAYSCALE) 
	{
		PyObject* groupPix = PyTuple_New(1);
		PyTuple_SET_ITEM(groupPix, 0, PyFloat_FromDouble(pix.r));
		return groupPix;
	}
	else if(self->tileType == yafaray::PASS_EXT_TILE_3_RGB) 
	{
		PyObject* groupPix = PyTuple_New(3);
		PyTuple_SET_ITEM(groupPix, 0, PyFloat_FromDouble(pix.r));
		PyTuple_SET_ITEM(groupPix, 1, PyFloat_FromDouble(pix.g));
		PyTuple_SET_ITEM(groupPix, 2, PyFloat_FromDouble(pix.b));
		return groupPix;
	}
	else
	{
		PyObject* groupPix = PyTuple_New(4);
		PyTuple_SET_ITEM(groupPix, 0, PyFloat_FromDouble(pix.r));
		PyTuple_SET_ITEM(groupPix, 1, PyFloat_FromDouble(pix.g));
		PyTuple_SET_ITEM(groupPix, 2, PyFloat_FromDouble(pix.b));
		PyTuple_SET_ITEM(groupPix, 3, PyFloat_FromDouble(pix.a));
		return groupPix;
	}
}

static void yaf_tile_dealloc(YafTileObject_t *self)
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

PyTypeObject yafTile_Type =
{
	PyVarObject_HEAD_INIT(nullptr, 0)
	"yaf_tile",							/* tp_name */
	sizeof(YafTileObject_t),			/* tp_basicsize */
	0,									/* tp_itemsize */
	( destructor ) yaf_tile_dealloc,	/* tp_dealloc */
	nullptr,                       		/* printfunc tp_print; */
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


class pyOutput_t : public yafaray::colorOutput_t
{

public:

	pyOutput_t(int x, int y, int borderStartX, int borderStartY, bool prev, PyObject *drawAreaCallback, PyObject *flushCallback) :
	resx(x),
	resy(y),
	bsX(borderStartX),
	bsY(borderStartY),
	preview(prev),
	mDrawArea(drawAreaCallback),
	mFlush(flushCallback)
	{
	}

    virtual void initTilesPasses(int totalViews, int numExtPasses)
    { 
		SWIG_PYTHON_THREAD_BEGIN_BLOCK; 
		PyGILState_STATE gstate;
		gstate = PyGILState_Ensure();

		tilesPasses.resize(totalViews);
    
        for(size_t view = 0; view < tilesPasses.size(); ++view)
		{
			for(int idx = 0; idx < numExtPasses; ++idx)
			{
				YafTileObject_t* tile = PyObject_New(YafTileObject_t, &yafTile_Type);
				tilesPasses.at(view).push_back(tile);
				tilesPasses.at(view)[idx]->mem = new yafTilePixel_t[resx*resy];
				tilesPasses.at(view)[idx]->resx = resx;
				tilesPasses.at(view)[idx]->resy = resy;
			}
		}
		
		PyGILState_Release(gstate);
		SWIG_PYTHON_THREAD_END_BLOCK; 
    }

	virtual ~pyOutput_t()
	{
		SWIG_PYTHON_THREAD_BEGIN_BLOCK; 
		PyGILState_STATE gstate;
		gstate = PyGILState_Ensure();

		for(size_t view = 0; view < tilesPasses.size(); ++view)
		{
			for(size_t idx = 0; idx < tilesPasses.at(view).size(); ++idx)
			{
				if(tilesPasses.at(view)[idx]->mem) delete [] tilesPasses.at(view)[idx]->mem;
				//Py_XDECREF(tilesPasses.at(view)[idx]);
			}
			tilesPasses.at(view).clear();
		}
		tilesPasses.clear();
		
		PyGILState_Release(gstate);
		SWIG_PYTHON_THREAD_END_BLOCK; 
	}

	virtual bool putPixel(int numView, int x, int y, const yafaray::renderPasses_t *renderPasses, const std::vector<yafaray::colorA_t> &colExtPasses, bool alpha = true)
	{
		for(size_t idx = 0; idx < tilesPasses.at(numView).size(); ++idx)
		{
			yafTilePixel_t &pix= tilesPasses.at(numView)[idx]->mem[resx * y + x];
			pix.r = colExtPasses[idx].R;
			pix.g = colExtPasses[idx].G;
			pix.b = colExtPasses[idx].B;
			pix.a = (alpha || idx > 0) ? colExtPasses[idx].A : 1.0f;
		}
        
		return true;
	}

	virtual void flush(int numView_unused, const yafaray::renderPasses_t *renderPasses)
	{
		SWIG_PYTHON_THREAD_BEGIN_BLOCK; 
		PyGILState_STATE gstate;
		gstate = PyGILState_Ensure();
		
		PyObject* groupTile = PyTuple_New(tilesPasses.size() * tilesPasses.at(0).size());

		for(size_t view = 0; view < tilesPasses.size(); ++view)
		{
			std::string view_name = renderPasses->view_names.at(view);

			for(size_t idx = 0; idx < tilesPasses.at(view).size(); ++idx)
			{
				tilesPasses.at(view)[idx]->x0 = 0;
				tilesPasses.at(view)[idx]->x1 = resx;
				tilesPasses.at(view)[idx]->y0 = 0;
				tilesPasses.at(view)[idx]->y1 = resy;
				
				tilesPasses.at(view)[idx]->tileType = renderPasses->tileType(idx);
				
				std::stringstream extPassName;
				extPassName << renderPasses->extPassTypeStringFromIndex(idx);
				PyObject* groupItem = Py_BuildValue("ssO", view_name.c_str(), extPassName.str().c_str(), tilesPasses.at(view)[idx]);				
				PyTuple_SET_ITEM(groupTile, tilesPasses.at(view).size()*view + idx, (PyObject*) groupItem);

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

	virtual void flushArea(int numView, int x0, int y0, int x1, int y1, const yafaray::renderPasses_t *renderPasses)
	{
		SWIG_PYTHON_THREAD_BEGIN_BLOCK; 
		std::string view_name = renderPasses->view_names.at(numView);
		
		// Do nothing if we are rendering preview renders
		if(preview) return;

		int w = x1 - x0;
		int h = y1 - y0;

		PyGILState_STATE gstate;
		gstate = PyGILState_Ensure();

		PyObject* groupTile = PyTuple_New(tilesPasses.at(numView).size());

		for(size_t idx = 0; idx < tilesPasses.at(numView).size(); ++idx)
		{
			tilesPasses.at(numView)[idx]->x0 = x0 - bsX;
			tilesPasses.at(numView)[idx]->x1 = x1 - bsX;
			tilesPasses.at(numView)[idx]->y0 = y0 - bsY;
			tilesPasses.at(numView)[idx]->y1 = y1 - bsY;
			
			tilesPasses.at(numView)[idx]->tileType = renderPasses->tileType(idx);
			
			std::stringstream extPassName;
			extPassName << renderPasses->extPassTypeStringFromIndex(idx);
			PyObject* groupItem = Py_BuildValue("ssO", view_name.c_str(), extPassName.str().c_str(), tilesPasses.at(numView)[idx]);
			PyTuple_SET_ITEM(groupTile, idx, (PyObject*) groupItem);

			//std::cout << "flushArea: groupItem->ob_refcnt=" << groupItem->ob_refcnt << std::endl;
		}

		PyObject* result = PyObject_CallFunction(mDrawArea, "iiiiiO", tilesPasses.at(numView)[0]->x0, resy - tilesPasses.at(numView)[0]->y1, w, h, numView, groupTile);
		
		Py_XDECREF(result);
		Py_XDECREF(groupTile);
		//std::cout << "flushArea: result->ob_refcnt=" << result->ob_refcnt << std::endl;
		//std::cout << "flushArea: groupTile->ob_refcnt=" << groupTile->ob_refcnt << std::endl;
		
		PyGILState_Release(gstate);
		SWIG_PYTHON_THREAD_END_BLOCK; 
	}

	virtual void highliteArea(int numView, int x0, int y0, int x1, int y1)
	{
		SWIG_PYTHON_THREAD_BEGIN_BLOCK; 
		std::string view_name = "";
		
		// Do nothing if we are rendering preview renders
		if(preview) return;

		tilesPasses.at(numView)[0]->x0 = x0 - bsX;
		tilesPasses.at(numView)[0]->x1 = x1 - bsX;
		tilesPasses.at(numView)[0]->y0 = y0 - bsY;
		tilesPasses.at(numView)[0]->y1 = y1 - bsY;

		int w = x1 - x0;
		int h = y1 - y0;
		int lineL = std::min( 4, std::min( h - 1, w - 1 ) );

		drawCorner(numView, tilesPasses.at(numView)[0]->x0, tilesPasses.at(numView)[0]->y0, lineL, TL_CORNER);
		drawCorner(numView, tilesPasses.at(numView)[0]->x1, tilesPasses.at(numView)[0]->y0, lineL, TR_CORNER);
		drawCorner(numView, tilesPasses.at(numView)[0]->x0, tilesPasses.at(numView)[0]->y1, lineL, BL_CORNER);
		drawCorner(numView, tilesPasses.at(numView)[0]->x1, tilesPasses.at(numView)[0]->y1, lineL, BR_CORNER);

		PyGILState_STATE gstate;
		gstate = PyGILState_Ensure();
		

		PyObject* groupTile = PyTuple_New(1);

		tilesPasses.at(numView)[0]->tileType = yafaray::PASS_EXT_TILE_4_RGBA;
		
		PyObject* groupItem = Py_BuildValue("ssO", view_name.c_str(), "Combined", tilesPasses.at(numView)[0]);
		PyTuple_SET_ITEM(groupTile, 0, (PyObject*) groupItem);

		//std::cout << "highliteArea: groupItem->ob_refcnt=" << groupItem->ob_refcnt << std::endl;
		
		PyObject* result = PyObject_CallFunction(mDrawArea, "iiiiiO", tilesPasses.at(numView)[0]->x0, resy - tilesPasses.at(numView)[0]->y1, w, h, numView, groupTile);
		
		Py_XDECREF(result);
		Py_XDECREF(groupTile);

		//std::cout << "highliteArea: result->ob_refcnt=" << result->ob_refcnt << std::endl;
		//std::cout << "highliteArea: groupTile->ob_refcnt=" << groupTile->ob_refcnt << std::endl;

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

	void drawCorner(int numView, int x, int y, int len, corner pos)
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
			yafTilePixel_t &pix = tilesPasses.at(numView)[0]->mem[resx * y + i];
			pix.r = 0.625f;
			pix.g = 0.f;
			pix.b = 0.f;
			pix.a = 1.f;
		}

		for(int j = minY; j < maxY; ++j)
		{
			yafTilePixel_t &pix = tilesPasses.at(numView)[0]->mem[resx * j + x]; 
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
	std::vector< std::vector<YafTileObject_t*> > tilesPasses;
};

class pyProgress : public yafaray::progressBar_t
{

public:

	pyProgress(PyObject *callback) : callb(callback) {}

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
		steps_to_percent = 1.f / (float) totalSteps;
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
		PyGILState_STATE gstate;
		gstate = PyGILState_Ensure();
		PyObject* result = PyObject_CallFunction(callb, "ss", "tag", text);
		Py_XDECREF(result);
		//std::cout << "setTag: result->ob_refcnt=" << result->ob_refcnt << std::endl;
		PyGILState_Release(gstate);
	}

private:

	PyObject *callb;
	float steps_to_percent;
	int doneSteps;
};

%}

%init %{
	PyType_Ready(&yafTile_Type);
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

%extend yafaray::yafrayInterface_t
{
	void render(int x, int y, int borderStartX, int borderStartY, bool prev, PyObject *drawAreaCallBack, PyObject *flushCallBack, PyObject *progressCallback)
	{
		pyOutput_t output_wrap(x, y, borderStartX, borderStartY, prev, drawAreaCallBack, flushCallBack);
		pyProgress *pbar_wrap = new pyProgress(progressCallback);

		Py_BEGIN_ALLOW_THREADS;
		self->render(output_wrap, pbar_wrap);
		Py_END_ALLOW_THREADS;
	}
}

%exception yafaray::yafrayInterface_t::loadPlugins
{
	Py_BEGIN_ALLOW_THREADS
	$action
	Py_END_ALLOW_THREADS
}

#endif // End of python specific code

%{
#include <yafray_constants.h>
#include <interface/yafrayinterface.h>
#include <interface/xmlinterface.h>
#include <yafraycore/imageOutput.h>
#include <yafraycore/memoryIO.h>
#include <core_api/matrix4.h>
using namespace yafaray;
%}

#ifdef SWIGRUBY
%feature("director") colorOutput_t::putPixel;
%feature("director") colorOutput_t::flush;
%feature("director") colorOutput_t::flushArea;
#endif

namespace yafaray
{
	// Required abstracts

	class colorOutput_t
	{
		public:
			virtual ~colorOutput_t() {};
			virtual bool putPixel(int numView, int x, int y, const renderPasses_t *renderPasses, const std::vector<colorA_t> &colExtPasses, bool alpha = true)=0;
			virtual void flush(int numView, const renderPasses_t *renderPasses)=0;
			virtual void flushArea(int numView, int x0, int y0, int x1, int y1, const renderPasses_t *renderPasses)=0;
			virtual void highliteArea(int numView, int x0, int y0, int x1, int y1){};
	};

	class imageHandler_t
	{
		public:
			virtual void initForOutput(int width, int height, const renderPasses_t *renderPasses, bool withAlpha = false, bool multi_layer = false) = 0;
			virtual ~imageHandler_t() {};
			virtual bool loadFromFile(const std::string &name) = 0;
			virtual bool loadFromMemory(const yByte *data, size_t size) {return false; }
			virtual bool saveToFile(const std::string &name, int imagePassNumber = 0) = 0;
			virtual void putPixel(int x, int y, const colorA_t &rgba, int imagePassNumber = 0) = 0;
			virtual colorA_t getPixel(int x, int y, int imagePassNumber = 0) = 0;
			virtual int getWidth() { return m_width; }
			virtual int getHeight() { return m_height; }
			virtual bool isHDR() { return false; }
			
		protected:
			std::string handlerName;
			int m_width;
			int m_height;
			bool m_hasAlpha;
			std::vector<rgba2DImage_nw_t*> imagePasses; //!< rgba color buffers for the additional render passes
			rgba2DImage_nw_t *m_rgba;
	};

	// Outputs

	class imageOutput_t : public colorOutput_t
	{
		public:
			imageOutput_t(imageHandler_t *handle, const std::string &name, int bx, int by);
			imageOutput_t(); //!< Dummy initializer
			virtual ~imageOutput_t();
			virtual bool putPixel(int numView, int x, int y, const renderPasses_t *renderPasses, const std::vector<colorA_t> &colExtPasses, bool alpha = true);
			virtual void flush(int numView, const renderPasses_t *renderPasses);
			virtual void flushArea(int numView, int x0, int y0, int x1, int y1, const renderPasses_t *renderPasses) {}; // not used by images... yet
		private:
			imageHandler_t *image;
			std::string fname;
			float bX;
			float bY;
	};

	class memoryIO_t : public colorOutput_t
	{
		public:
			memoryIO_t(int resx, int resy, float* iMem);
			virtual bool putPixel(int numView, int x, int y, const renderPasses_t *renderPasses, const std::vector<colorA_t> &colExtPasses, bool alpha = true);
			void flush(int numView, const renderPasses_t *renderPasses);
			virtual void flushArea(int numView, int x0, int y0, int x1, int y1, const renderPasses_t *renderPasses) {}; // no tiled file format used...yet
			virtual ~memoryIO_t();
		protected:
			int sizex, sizey;
			float* imageMem;
	};

	// Utility classes

	class matrix4x4_t
	{
		public:
			matrix4x4_t() {};
			matrix4x4_t(const float init);
			matrix4x4_t(const matrix4x4_t & source);
			matrix4x4_t(const float source[4][4]);
			matrix4x4_t(const double source[4][4]);
			~matrix4x4_t() {};
			/*! attention, this function can cause the matrix to become invalid!
				unless you are sure the matrix is invertible, check invalid() afterwards! */
			matrix4x4_t & inverse();
			matrix4x4_t & transpose();
			void identity();
			void translate(float dx,float dy,float dz);
			void rotateX(float degrees);
			void rotateY(float degrees);
			void rotateZ(float degrees);
			void scale(float sx, float sy, float sz);
			int invalid() const { return _invalid; }
			// ignored by swig
			//const PFLOAT * operator [] (int i) const { return matrix[i]; }
			//PFLOAT * operator [] (int i) { return matrix[i]; }
			void setVal(int row, int col, float val)
			{
				matrix[row][col] = val;
			};

			float getVal(int row, int col)
			{
				return matrix[row][col];
			};

		protected:

			float  matrix[4][4];
			int _invalid;
	};

	// Interfaces

	class yafrayInterface_t
	{
		public:
			yafrayInterface_t();
			virtual ~yafrayInterface_t();
			// directly related to scene_t:
			virtual void loadPlugins(const char *path); //!< load plugins from path, if nullptr load from default path, if available.
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
			virtual void render(colorOutput_t &output, progressBar_t *pb = nullptr); //!< render the scene...
			virtual bool startScene(int type=0); //!< start a new scene; Must be called before any of the scene_t related callbacks!
			virtual bool setupRenderPasses(); //!< setup render passes information
			virtual void abort();
			virtual paraMap_t* getRenderParameters() { return params; }
			virtual bool getRenderedImage(int numView, colorOutput_t &output); //!< put the rendered image to output
			virtual std::vector<std::string> listImageHandlers();
			virtual std::vector<std::string> listImageHandlersFullName();
			virtual std::string getImageFormatFromFullName(const std::string &fullname);
			virtual std::string getImageFullNameFromFormat(const std::string &format);
			
			void setConsoleVerbosityLevel(const std::string &strVLevel);
			void setLogVerbosityLevel(const std::string &strVLevel);
			
			virtual void setParamsBadgePosition(const std::string &badgePosition = "none");
			virtual bool getDrawParams();

			virtual char* getVersion() const; //!< Get version to check aginst the exporters
			
			/*! Console Printing wrappers to report in color with yafaray's own console coloring */
			void printDebug(const std::string &msg);
			void printVerbose(const std::string &msg);
			void printInfo(const std::string &msg);
			void printParams(const std::string &msg);
			void printWarning(const std::string &msg);
			void printError(const std::string &msg);
			
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

	class xmlInterface_t: public yafrayInterface_t
	{
		public:
			xmlInterface_t();
			// directly related to scene_t:
			virtual void loadPlugins(const char *path);
			virtual bool setupRenderPasses(); //!< setup render passes information
			virtual bool startGeometry();
			virtual bool endGeometry();
			virtual unsigned int getNextFreeID();
			virtual bool startTriMesh(unsigned int id, int vertices, int triangles, bool hasOrco, bool hasUV=false, int type=0, int obj_pass_index=0);
			virtual bool startTriMeshPtr(unsigned int *id, int vertices, int triangles, bool hasOrco, bool hasUV=false, int type=0, int obj_pass_index=0);
			virtual bool startCurveMesh(unsigned int id, int vertices, int obj_pass_index=0);
			virtual bool endTriMesh();
			virtual bool addInstance(unsigned int baseObjectId, matrix4x4_t objToWorld);
			virtual bool endCurveMesh(const material_t *mat, float strandStart, float strandEnd, float strandShape);
			virtual int  addVertex(double x, double y, double z); //!< add vertex to mesh; returns index to be used for addTriangle
			virtual int  addVertex(double x, double y, double z, double ox, double oy, double oz); //!< add vertex with Orco to mesh; returns index to be used for addTriangle
			virtual void addNormal(double nx, double ny, double nz); //!< add vertex normal to mesh; the vertex that will be attached to is the last one inserted by addVertex method
			virtual bool addTriangle(int a, int b, int c, const material_t *mat);
			virtual bool addTriangle(int a, int b, int c, int uv_a, int uv_b, int uv_c, const material_t *mat);
			virtual int  addUV(float u, float v);
			virtual bool smoothMesh(unsigned int id, double angle);
			
			// functions directly related to renderEnvironment_t
			virtual light_t* 		createLight			(const char* name);
			virtual texture_t* 		createTexture		(const char* name);
			virtual material_t* 	createMaterial		(const char* name);
			virtual camera_t* 		createCamera		(const char* name);
			virtual background_t* 	createBackground	(const char* name);
			virtual integrator_t* 	createIntegrator	(const char* name);
			virtual VolumeRegion* 	createVolumeRegion	(const char* name);
			virtual unsigned int 	createObject		(const char* name);
			virtual void clearAll(); //!< clear the whole environment + scene, i.e. free (hopefully) all memory.
			virtual void render(colorOutput_t &output); //!< render the scene...
			virtual bool startScene(int type=0); //!< start a new scene; Must be called before any of the scene_t related callbacks!
			virtual void setOutfile(const char *fname);
			void xmlInterface_t::setXMLColorSpace(std::string color_space_string, float gammaVal);
		protected:
			void writeParamMap(const paraMap_t &pmap, int indent=1);
			void writeParamList(int indent);
			
			std::map<const material_t *, std::string> materials;
			std::ofstream xmlFile;
			std::string xmlName;
			const material_t *last_mat;
			size_t nmat;
			int n_uvs;
			unsigned int nextObj;
			float XMLGamma;
			colorSpaces_t XMLColorSpace;
	};

}
