%{
#include <yafraycore/monitor.h>
#include <core_api/output.h>
#include <interface/yafrayinterface.h>

typedef struct {
	PyObject_HEAD
	int resx, resy;
	int x0, x1, y0, y1;
	float *mem;
} YafTileObject_t;

static Py_ssize_t yaf_tile_length(YafTileObject_t *self) {
	return (self->x1 - self->x0) * (self->y1 - self->y0);
}
static PyObject *yaf_tile_subscript_int(YafTileObject_t *self, int keynum) {
	if (keynum >= yaf_tile_length(self))
		return NULL;
		
	int width = (self->x1 - self->x0);
	int height = (self->y1 - self->y0);
	
	int line = (self->y0 + height - 1) - (keynum/width);
	int index_on_line = keynum - (keynum/width)*width;
	int index = (self->resx*line + self->x0 + index_on_line) * 4;
	
	return Py_BuildValue("ffff", 
		self->mem[index],
		self->mem[index+1],
		self->mem[index+2],
		self->mem[index+3]);
}
static void yaf_tile_dealloc(YafTileObject_t *self) {
	PyObject_Del(self);
}

PySequenceMethods sequence_methods = {
	( lenfunc ) yaf_tile_length,
	NULL, NULL,
	( ssizeargfunc ) yaf_tile_subscript_int
};

PyTypeObject yafTile_Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"yaf_tile",			/* tp_name */
	sizeof(YafTileObject_t),	/* tp_basicsize */
	0,			/* tp_itemsize */
	( destructor ) yaf_tile_dealloc,			/* tp_dealloc */
	NULL,                       /* printfunc tp_print; */
	NULL,						/* getattrfunc tp_getattr; */
	NULL,						/* setattrfunc tp_setattr; */
	NULL,						/* tp_compare */ /* DEPRECATED in python 3.0! */
	NULL,	/* tp_repr */
	NULL,                       /* PyNumberMethods *tp_as_number; */
	&sequence_methods,	/* PySequenceMethods *tp_as_sequence; */
	NULL,	/* PyMappingMethods *tp_as_mapping; */
	NULL,	/* hashfunc tp_hash; */
	NULL,						/* ternaryfunc tp_call; */
	NULL,                       /* reprfunc tp_str; */
	NULL,	/* getattrofunc tp_getattro; */
	NULL,	/* setattrofunc tp_setattro; */
	NULL,                       /* PyBufferProcs *tp_as_buffer; */
	Py_TPFLAGS_DEFAULT,         /* long tp_flags; */
};


class pyOutput_t : public yafaray::colorOutput_t {
public:
	pyOutput_t(int x, int y, PyObject *callback) : resx(x), resy(y), callb(callback) {
		tile = PyObject_NEW(YafTileObject_t, &yafTile_Type);
		tile->mem = new float[x*y*4];
	}
	
	virtual ~pyOutput_t() {
		delete[] tile->mem;
		Py_DECREF(tile);
	}
	
	virtual bool putPixel(int x, int y, const float *c, bool alpha = true, bool depth = false, float z = 0.f) {
		tile->mem[(resx*y+x)*4+0] = c[0];
		tile->mem[(resx*y+x)*4+1] = c[1];
		tile->mem[(resx*y+x)*4+2] = c[2];
		tile->mem[(resx*y+x)*4+3] = alpha ? c[3] : 1.0f;
		return true;
	}
	
	virtual void flush() {
	}
	virtual void flushArea(int x0, int y0, int x1, int y1) {
		tile->x0 = x0;
		tile->x1 = x1;
		tile->y0 = y0;
		tile->y1 = y1;
		tile->resx = resx;
		tile->resy = resy;
		
		PyGILState_STATE gstate;
		gstate = PyGILState_Ensure();
		PyEval_CallObject(callb, Py_BuildValue("siiiiO", "flushArea", 
			x0, resy - y1, x1, resy - y0, tile));
		PyGILState_Release(gstate);
	}
	virtual void highliteArea(int x0, int y0, int x1, int y1) {
	}
	
private:
	PyObject *callb;
	int resx, resy;
	int tilex, tiley;
	YafTileObject_t *tile;
};

class pyProgress : public yafaray::progressBar_t {
public:
	pyProgress(PyObject *callback) : callb(callback) {}
	
	void report_progress(float percent) {
		PyGILState_STATE gstate;
		gstate = PyGILState_Ensure();
		PyEval_CallObject(callb, Py_BuildValue("sf", "progress", percent));
		PyGILState_Release(gstate);
	}
	
	virtual void init(int totalSteps) {
		steps_to_percent = 1.f*100/totalSteps;
		doneSteps = 0;
		report_progress(0.f);
	}
	virtual void update(int steps = 1) {
		doneSteps += steps;
		report_progress(doneSteps * steps_to_percent);		
	}
	virtual void done() {
		report_progress(100.f);
	}
	virtual void setTag(const char* text) {
		PyGILState_STATE gstate;
		gstate = PyGILState_Ensure();
		PyEval_CallObject(callb, Py_BuildValue("ss", "tag", text));
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

%typemap(in) PyObject *pyfunc {
  if (!PyCallable_Check($input)) {
      PyErr_SetString(PyExc_TypeError, "Need a callback method.");
      return NULL;
  }
  $1 = $input;
}

%extend yafaray::yafrayInterface_t {
	void render(int x, int y, PyObject *pyfunc1, PyObject *pyfunc2) {
  		pyOutput_t output_wrap(x, y, pyfunc1);
  		pyProgress *pbar_wrap = new pyProgress(pyfunc2);
  		Py_BEGIN_ALLOW_THREADS;
  		self->render(output_wrap, pbar_wrap);
		Py_END_ALLOW_THREADS;
	}
}

%exception yafaray::yafrayInterface_t::loadPlugins {
	Py_BEGIN_ALLOW_THREADS
	$action
	Py_END_ALLOW_THREADS
}

