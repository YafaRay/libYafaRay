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
#include "color/color.h"
#include "color/color_layers.h"
#include "image/image.h"
#include "image/image_layers.h"
#include "common/logger.h"
#include "render/render_view.h"

BEGIN_YAFARAY

struct Tile
{
	PyObject_HEAD //Important! Python automatically reads/writes this head by position within the struct (not by name!), the class *must never* have anything located before the head or it will be overwritten in a very messy way!
	int area_x_0_ = 0, area_x_1_ = 0;
	int area_y_0_ = 0, area_y_1_ = 0;
	ImageLayer *image_layer_ = nullptr; //The Python Object header needs to know the size of the object, better to use pointers to contained objects to get a reliable size
	//Important: the PyObject_New function does *not* initialize the structure with its default values, an explicit init function needed to initialize and allocate any pointer(s)
	void init() { area_x_0_ = 0; area_x_1_ = 0; area_y_0_ = 0; area_y_1_ = 0; image_layer_ = new ImageLayer(); }
	~Tile() { delete image_layer_; }
};

class TilesLayers final : public Collection<Layer::Type, Tile>  //Actual buffer of images in the rendering process, one entry for each enabled layer.
{
	public:
		void setColor(int x, int y, const ColorLayer &color_layer);
		Rgba getColor(int x, int y, const Layer &layer);
};

inline void TilesLayers::setColor(int x, int y, const ColorLayer &color_layer)
{
	Tile *tile = find(color_layer.layer_type_);
	if(tile) tile->image_layer_->image_->setColor(x, y, color_layer.color_);
}

inline Rgba TilesLayers::getColor(int x, int y, const Layer &layer)
{
	const Tile *tile = find(layer.getType());
	if(tile) return tile->image_layer_->image_->getColor(x, y);
	else return {0.f};
}

static Py_ssize_t pythonTileSize_global(const Tile *tile)
{
	const int area_w = (tile->area_x_1_ - tile->area_x_0_);
	const int area_h = (tile->area_y_1_ - tile->area_y_0_);

	return area_w * area_h;
}


static PyObject *pythonTileSubscriptInt_global(const Tile *tile, int py_index)
{
	const int num_channels = tile->image_layer_->layer_.getNumExportedChannels();
	// Check boundaries and fill w and h
	if(py_index >= pythonTileSize_global(tile) || py_index < 0)
	{
		PyObject* groupPix = PyTuple_New(num_channels);
		for(int i = 0; i < num_channels; ++i)
		{
			PyTuple_SET_ITEM(groupPix, i, PyFloat_FromDouble((i == 3 ? 1.f : 0.f)));
		}
		return groupPix;
	}

	const int area_w = (tile->area_x_1_ - tile->area_x_0_);
	const int area_h = (tile->area_y_1_ - tile->area_y_0_);

	// Calc position of the tile in the image region
	int vy = py_index / area_w;
	int vx = py_index - vy * area_w;

	// Map tile position to image buffer
	vx = tile->area_x_0_ + vx;
	vy = (tile->area_y_0_ + area_h - 1) - vy;

	// Get pixel
	const Rgba color = tile->image_layer_->image_->getColor(vx, vy);

	PyObject* groupPix = PyTuple_New(num_channels);
	if(num_channels >= 1)
	{
		PyTuple_SET_ITEM(groupPix, 0, PyFloat_FromDouble(color.r_));
	}
	if(num_channels >= 3)
	{
		PyTuple_SET_ITEM(groupPix, 1, PyFloat_FromDouble(color.g_));
		PyTuple_SET_ITEM(groupPix, 2, PyFloat_FromDouble(color.b_));
	}
	if(num_channels == 4)
	{
		PyTuple_SET_ITEM(groupPix, 3, PyFloat_FromDouble(color.a_));
	}
	return groupPix;
}

static void pythonTileDelete_global(Tile *tile)
{
	SWIG_PYTHON_THREAD_BEGIN_BLOCK;
	PyObject_Del(tile);
	SWIG_PYTHON_THREAD_END_BLOCK;
}

PySequenceMethods sequence_methods =
{
	( lenfunc ) pythonTileSize_global,
	nullptr,
	nullptr,
	( ssizeargfunc ) pythonTileSubscriptInt_global
};

PyTypeObject python_tile_type_global =
{
	PyVarObject_HEAD_INIT(nullptr, 0)
	"yaf_tile",							/* tp_name */
	sizeof(Tile),			/* tp_basicsize */
	0,									/* tp_itemsize */
	( destructor ) pythonTileDelete_global,	/* tp_dealloc */
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


class PythonOutput : public ColorOutput
{

public:

	PythonOutput(int width, int height, int border_x, int border_y, bool preview, const std::string &color_space, float gamma, bool with_alpha, bool alpha_premultiply) :
	ColorOutput("blender_out", Rgb::colorSpaceFromName(color_space), gamma, with_alpha, alpha_premultiply),
	width_(width), height_(height), border_x_(border_x), border_y_(border_y), preview_(preview)
	{
	}

	void setRenderCallbacks(PyObject *py_draw_area_callback, PyObject *py_flush_callback)
	{
		//Y_DEBUG PRTEXT(setRenderCallbacks b4) PR(py_draw_area_callback_) PR(py_flush_callback_) PREND;
		py_draw_area_callback_ = py_draw_area_callback;
		py_flush_callback_ = py_flush_callback;
		//Y_DEBUG PRTEXT(setRenderCallbacks after) PR(py_draw_area_callback_) PR(py_flush_callback_) PREND;
	}

    virtual void init(int width, int height, const Layers *layers, const std::map<std::string, RenderView *> *render_views) override
    { 
		SWIG_PYTHON_THREAD_BEGIN_BLOCK; 
		PyGILState_STATE gstate;
		gstate = PyGILState_Ensure();

		//Y_DEBUG PRTEXT(PythonOutputInit) PR(py_draw_area_callback_) PR(py_flush_callback_) PREND;

		ColorOutput::init(width, height, layers, render_views);
		for(const auto &render_view : *render_views)
		{
			//FIXME clearImageLayers();
			TilesLayers *tiles_layers = new TilesLayers();
			const Layers layers_exported = layers_->getLayersWithExportedImages();
			for(const auto &it : layers_exported)
			{
				Image::Type image_type = it.second.getImageType();
				std::unique_ptr<Image> image = Image::factory(width, height, image_type, Image::Optimization::None);
				Tile *tile = PyObject_New(Tile, &python_tile_type_global);
				tile->init();
				tile->image_layer_->image_ = std::move(image);
				tile->image_layer_->layer_ = it.second;
				tiles_layers->set(it.first, *tile);
			}
			tiles_views_[render_view.first] = tiles_layers;
		}
		PyGILState_Release(gstate);
		SWIG_PYTHON_THREAD_END_BLOCK; 
    }

	virtual ~PythonOutput() override
	{
		SWIG_PYTHON_THREAD_BEGIN_BLOCK; 
		PyGILState_STATE gstate;
		gstate = PyGILState_Ensure();

		for(size_t view = 0; view < tiles_views_.size(); ++view)
		{
//			for(size_t idx = 0; idx < tiles_views_.at(current_render_view_->getName())->size(); ++idx)
//			{
//				if(tile.second.color_vector_) delete [] tile.second.color_vector_;
//				//Py_XDECREF(tiles_views_.at(current_render_view_->getName())->[idx]);
//			}
			//FIXME clearImageLayers();
		}
		tiles_views_.clear();
		
		PyGILState_Release(gstate);
		SWIG_PYTHON_THREAD_END_BLOCK; 
	}

	virtual bool putPixel(int x, int y, const ColorLayer &color_layer) override
	{
		tiles_views_.at(current_render_view_->getName())->setColor(x, y, color_layer);
		return true;
	}

	virtual bool isPreview() const override { return preview_; }

	virtual void flush(const RenderControl &render_control) override
	{
		//Y_DEBUG PRTEXT(flush) PREND;
		if(!py_flush_callback_) return;

		SWIG_PYTHON_THREAD_BEGIN_BLOCK;
		PyGILState_STATE gstate;
		gstate = PyGILState_Ensure();
		const std::string view_name = current_render_view_->getName();
		TilesLayers *tiles_layers = tiles_views_.at(view_name);
		const size_t num_tiles_layers = tiles_layers->size();
		PyObject* groupTile = PyTuple_New(num_tiles_layers);
		//Y_DEBUG PR(groupTile) PREND; if(groupTile) //Y_DEBUG PR(groupTile->ob_refcnt) PREND;
		size_t tuple_index = 0;
		for(auto &tile : *tiles_layers)
		{
			tile.second.area_x_0_ = 0;
			tile.second.area_x_1_ = width_;
			tile.second.area_y_0_ = 0;
			tile.second.area_y_1_ = height_;
			std::string layer_name = tile.second.image_layer_->layer_.getExportedImageName();
			if(layer_name.empty()) layer_name = tile.second.image_layer_->layer_.getTypeName();
			//Y_DEBUG PR(view_name) PR(tuple_index) PR(tiles_layers->size()) PR(Layer::getTypeName(tile.first)) PR(tiles_layers) PR(&tile.second) PREND;
			PyObject* groupItem = Py_BuildValue("sO", layer_name.c_str(), &tile.second);
			//Y_DEBUG PR(groupItem) PREND; if(groupItem) //Y_DEBUG PR(groupItem->ob_refcnt) PREND;
			PyTuple_SET_ITEM(groupTile, tuple_index, groupItem);
			++tuple_index;
		}
		PyObject* result = PyObject_CallFunction(py_flush_callback_, "iisO", width_, height_, view_name.c_str(), groupTile);
		//Y_DEBUG PR(result) PREND; if(result) //Y_DEBUG PR(result->ob_refcnt) PREND;

		Py_XDECREF(result);
		Py_XDECREF(groupTile);

		PyGILState_Release(gstate);
		SWIG_PYTHON_THREAD_END_BLOCK; 
	}

	virtual void flushArea(int x0, int y0, int x1, int y1) override
	{
		//Y_DEBUG PRTEXT(flushArea) PREND;
		// Do nothing if we are rendering preview_ renders
		if(preview_ || !py_draw_area_callback_) return;

		SWIG_PYTHON_THREAD_BEGIN_BLOCK;
		PyGILState_STATE gstate;
		gstate = PyGILState_Ensure();

		const std::string view_name = current_render_view_->getName();
		TilesLayers *tiles_layers = tiles_views_.at(view_name);
		const size_t num_tiles_layers = tiles_layers->size();
		PyObject* groupTile = PyTuple_New(num_tiles_layers);
		//Y_DEBUG PR(groupTile) PREND; if(groupTile) //Y_DEBUG PR(groupTile->ob_refcnt) PREND;
		size_t tuple_index = 0;
		for(auto &tile : *tiles_layers)
		{
			tile.second.area_x_0_ = x0 - border_x_;
			tile.second.area_x_1_ = x1 - border_x_;
			tile.second.area_y_0_ = y0 - border_y_;
			tile.second.area_y_1_ = y1 - border_y_;
			std::string layer_name = tile.second.image_layer_->layer_.getExportedImageName();
			if(layer_name.empty()) layer_name = tile.second.image_layer_->layer_.getTypeName();
			//Y_DEBUG PR(view_name) PR(tuple_index) PR(tiles_layers->size()) PR(Layer::getTypeName(tile.first)) PR(tiles_layers) PR(&tile.second) PR(tile.second.getTypeName()) PREND;
			PyObject* groupItem = Py_BuildValue("sO", layer_name.c_str(), &tile.second);
			//Y_DEBUG PR(groupItem) PREND; if(groupItem) //Y_DEBUG PR(groupItem->ob_refcnt) PREND;
			PyTuple_SET_ITEM(groupTile, tuple_index, groupItem);
			++tuple_index;
		}

		const int area_width = x1 - x0;
		const int area_height = y1 - y0;
		PyObject* result = PyObject_CallFunction(py_draw_area_callback_, "iiiisO", x0 - border_x_, height_ - (y1 - border_y_), area_width, area_height, view_name.c_str(), groupTile);
		//Y_DEBUG PR(result) PREND; if(result) //Y_DEBUG PR(result->ob_refcnt) PREND;

		Py_XDECREF(result);
		Py_XDECREF(groupTile);
		
		PyGILState_Release(gstate);
		SWIG_PYTHON_THREAD_END_BLOCK; 
	}

	virtual void highlightArea(int x0, int y0, int x1, int y1) override
	{
		//Y_DEBUG PRTEXT(highlightArea) PREND;
		// Do nothing if we are rendering preview_ renders
		if(preview_ || !py_draw_area_callback_) return;

		SWIG_PYTHON_THREAD_BEGIN_BLOCK;
		const std::string view_name = current_render_view_->getName();
		TilesLayers *tiles_layers = tiles_views_.at(view_name);
		Tile *tile = tiles_layers->find(Layer::Combined);

		tile->area_x_0_ = x0 - border_x_;
		tile->area_x_1_ = x1 - border_x_;
		tile->area_y_0_ = y0 - border_y_;
		tile->area_y_1_ = y1 - border_y_;

		int w = x1 - x0;
		int h = y1 - y0;
		int lineL = std::min( 4, std::min( h - 1, w - 1 ) );

		drawCorner(tile, tile->area_x_0_, tile->area_y_0_, lineL, TL_CORNER);
		drawCorner(tile, tile->area_x_1_, tile->area_y_0_, lineL, TR_CORNER);
		drawCorner(tile, tile->area_x_0_, tile->area_y_1_, lineL, BL_CORNER);
		drawCorner(tile, tile->area_x_1_, tile->area_y_1_, lineL, BR_CORNER);

		PyGILState_STATE gstate;
		gstate = PyGILState_Ensure();

		PyObject* groupTile = PyTuple_New(1);
		//Y_DEBUG PR(groupTile) PREND; if(groupTile) //Y_DEBUG PR(groupTile->ob_refcnt) PREND;

		std::string layer_name = tile->image_layer_->layer_.getExportedImageName();
		if(layer_name.empty()) layer_name = tile->image_layer_->layer_.getTypeName();
		//Y_DEBUG PR(view_name) PR(tiles_layers->size()) PR(Layer::getTypeName(Layer::Combined)) PR(tiles_layers) PR(tile) PREND;
		PyObject* groupItem = Py_BuildValue("sO", layer_name.c_str(), tile);
		//Y_DEBUG PR(groupItem) PREND; if(groupItem) //Y_DEBUG PR(groupItem->ob_refcnt) PREND;
		PyTuple_SET_ITEM(groupTile, 0, groupItem);
		
		PyObject* result = PyObject_CallFunction(py_draw_area_callback_, "iiiisO", tile->area_x_0_, height_ - tile->area_y_1_, w, h, view_name.c_str(), groupTile);
		//Y_DEBUG PR(result) PREND; if(result) //Y_DEBUG PR(result->ob_refcnt) PREND;
		
		Py_XDECREF(result);
		Py_XDECREF(groupTile);

		PyGILState_Release(gstate);
		SWIG_PYTHON_THREAD_END_BLOCK; 
	}

private:
	enum corner { TL_CORNER, TR_CORNER, BL_CORNER, BR_CORNER };
	void drawCorner(Tile *tile, int x, int y, int len, corner pos)
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
			tile->image_layer_->image_->setColor(i, y, {0.625f, 0.f, 0.f, 1.f});
		}

		for(int j = minY; j < maxY; ++j)
		{
			tile->image_layer_->image_->setColor(x, j, {0.625f, 0.f, 0.f, 1.f});
		}
	}

	int width_ = 0, height_ = 0;
	int border_x_ = 0, border_y_ = 0;
	bool preview_ = false;
	PyObject *py_draw_area_callback_ = nullptr;
	PyObject *py_flush_callback_ = nullptr;
	std::map<std::string, TilesLayers *> tiles_views_;
};

class YafPyProgress : public ProgressBar
{
public:
	YafPyProgress(PyObject *callback) : py_callback_(callback) {}

	void report_progress(float percent)
	{
		PyGILState_STATE gstate;
		gstate = PyGILState_Ensure();
		PyObject* result = PyObject_CallFunction(py_callback_, "sf", "progress", percent);
		Py_XDECREF(result);
		PyGILState_Release(gstate);
	}

	virtual void init(int total_steps) override
	{
		num_steps_ = total_steps;
		steps_to_percent_ = 1.f / static_cast<float>(num_steps_);
		done_steps_ = 0;
		report_progress(0.f);
	}

	virtual void update(int steps = 1) override
	{
		done_steps_ += steps;
		report_progress(done_steps_ * steps_to_percent_);
	}

	virtual void done() override
	{
		report_progress(1.f);
	}

	virtual void setTag(const char* text) override
	{
		tag_ = std::string(text);
		PyGILState_STATE gstate;
		gstate = PyGILState_Ensure();
		PyObject* result = PyObject_CallFunction(py_callback_, "ss", "tag", text);
		Py_XDECREF(result);
		PyGILState_Release(gstate);
	}

	virtual void setTag(std::string text) override
	{
		tag_ = text;
		PyGILState_STATE gstate;
		gstate = PyGILState_Ensure();
		PyObject* result = PyObject_CallFunction(py_callback_, "ss", "tag", text.c_str());
		Py_XDECREF(result);
		PyGILState_Release(gstate);
	}
	
	virtual std::string getTag() const override { return tag_; }
	virtual float getPercent() const override { return 100.f * std::min(1.f, static_cast<float>(done_steps_) * steps_to_percent_); }
	virtual float getTotalSteps() const override { return num_steps_; }

private:
	PyObject *py_callback_ = nullptr;
	float steps_to_percent_;
	int done_steps_, num_steps_;
	std::string tag_;
};

END_YAFARAY

%}

%init %{
	PyType_Ready(&python_tile_type_global);
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
	void render(PyObject *py_progress_callback)
	{
		YafPyProgress *pbar_wrap = new YafPyProgress(py_progress_callback);
		//Y_DEBUG PR(py_progress_callback) PREND;
		Py_BEGIN_ALLOW_THREADS;
		self->render(pbar_wrap);
		Py_END_ALLOW_THREADS;
	}
}

#endif // SWIGPYTHON  // End of python specific code

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
%feature("director") ColorOutput::putPixel;
%feature("director") ColorOutput::flush;
%feature("director") ColorOutput::flushArea;
#endif

namespace yafaray4
{
	// Required abstracts

	class ColorOutput
	{
		public:
		static ColorOutput *factory(const ParamMap &params, const Scene &scene);
		void setLoggingParams(const ParamMap &params);
		void setBadgeParams(const ParamMap &params);
		virtual ~ColorOutput() = default;
		virtual bool putPixel(int x, int y, const ColorLayer &color_layer) = 0;
		virtual void flush(const RenderControl &render_control) = 0;
		virtual void flushArea(int x_0, int y_0, int x_1, int y_1) { }
	};

#ifdef SWIGPYTHON  // Begining of python specific code
	class PythonOutput : public ColorOutput
	{
		public:
		PythonOutput(int width, int height, int border_x, int border_y, bool preview, const std::string &color_space, float gamma, bool with_alpha, bool alpha_premultiply);
		void setRenderCallbacks(PyObject *py_draw_area_callback, PyObject *py_flush_callback);
		void setLoggingParams(const ParamMap &params) override;
		void setBadgeParams(const ParamMap &params) override;
		virtual ~PythonOutput() override;
		virtual bool putPixel(int x, int y, const ColorLayer &color_layer) override;
		virtual void flush(const RenderControl &render_control) override;
		virtual void flushArea(int x_0, int y_0, int x_1, int y_1) override;
	};
#endif //SWIGPYTHON  // Begining of python specific code

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
		virtual void paramsSetColor(const char *name, float *rgb, bool with_alpha = false);
		virtual void paramsSetMatrix(const char *name, float m[4][4], bool transpose = false);
		virtual void paramsSetMatrix(const char *name, double m[4][4], bool transpose = false);
		virtual void paramsSetMemMatrix(const char *name, float *matrix, bool transpose = false);
		virtual void paramsSetMemMatrix(const char *name, double *matrix, bool transpose = false);
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
		virtual ColorOutput *createOutput(const char *name); //We do *NOT* have ownership of the outputs, do *NOT* delete them!
		virtual ColorOutput *createOutput(const std::string &name, ColorOutput *output); //We do *NOT* have ownership of the outputs, do *NOT* delete them!
		bool removeOutput(const std::string &name); //Caution: this will delete outputs, only to be called by the client on demand, we do *NOT* have ownership of the outputs
		virtual void clearOutputs(); //Caution: this will delete outputs, only to be called by the client on demand, we do *NOT* have ownership of the outputs
		virtual void clearAll(); //!< clear the whole environment + scene, i.e. free (hopefully) all memory.
		virtual void render(ProgressBar *pb = nullptr); //!< render the scene...
		virtual void defineLayer(const std::string &layer_type_name, const std::string &exported_image_type_name, const std::string &exported_image_name, const std::string &image_type_name = "");
		virtual bool setupLayersParameters();
		virtual void abort();
		virtual ParamMap *getRenderParameters() { return params_; }

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
	};

	class XmlExport: public Interface
	{
		public:
		XmlExport(const char *fname);
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
		virtual ColorOutput *createOutput(const char *name) override;
		virtual void clearAll() override; //!< clear the whole environment + scene, i.e. free (hopefully) all memory.
		virtual void clearOutputs() override { }
		virtual void render(ProgressBar *pb = nullptr) override; //!< render the scene...
		void setXmlColorSpace(std::string color_space_string, float gamma_val);
	};

}
