
#include <interface/yafrayinterface.h>
#include <core_api/environment.h>
#include <core_api/scene.h>
#include <core_api/imagefilm.h>
#include <core_api/integrator.h>

__BEGIN_YAFRAY

yafrayInterface_t::yafrayInterface_t(): scene(0), film(0), inputGamma(1.f), gcInput(false)
{
//	scene = new scene_t();
	env = new renderEnvironment_t();
//	std::string plugPath;
//	bool ok = env->getPluginPath(plugPath);
//	if(ok) env->loadPlugins(plugPath);
//	else std::cout << "error: could not get plugin path!\n";
	params = new paraMap_t;
	eparams = new std::list<paraMap_t>;
	cparams = params;
}

yafrayInterface_t::~yafrayInterface_t()
{
	std::cout << "~yafrayInterface_t()\tdelete scene...";
	if(scene) delete scene;
	std::cout << "delete environment...";
	if(env) delete env;
	std::cout << "done\n";
	if(film) delete film;
	delete params;
	delete eparams;
}

void yafrayInterface_t::loadPlugins(const char *path)
{
	if(path != 0)
	{
		std::string plugPath(path);
		if(plugPath.empty()) env->getPluginPath(plugPath);
		env->loadPlugins(plugPath);
	}
	else
	{
		std::string plugPath;
		if( env->getPluginPath(plugPath) )
		{
			env->loadPlugins(plugPath);
		}
	}
}
	

void yafrayInterface_t::clearAll()
{
	std::cout << "clearing data...\n";
	env->clearAll();
	std::cout << "delete scene...";
	if(scene) delete scene;
	std::cout << "done\n";
	scene = 0;//new scene_t();
	if(film) delete film;
	film = 0;
	params->clear();
	eparams->clear();
	cparams = params;
}

bool yafrayInterface_t::startScene(int type)
{
	if(scene) delete scene;
	scene = new scene_t();
	scene->setMode(type);
	env->setScene(scene);
	return true;
}

bool yafrayInterface_t::startGeometry() { return scene->startGeometry(); }

bool yafrayInterface_t::endGeometry() { return scene->endGeometry(); }

unsigned int yafrayInterface_t::getNextFreeID() {
	objID_t id;
	id = scene->getNextFreeID();
	return id;
}

bool yafrayInterface_t::startTriMesh(unsigned int id, int vertices, int triangles, bool hasOrco, bool hasUV, int type)
{
	bool success = scene->startTriMesh(id, vertices, triangles, hasOrco, hasUV, type);
	return success;
}

bool yafrayInterface_t::startTriMeshPtr(unsigned int *id, int vertices, int triangles, bool hasOrco, bool hasUV, int type)
{
	Y_WARNING << "This method is going to be removed, please use getNextFreeID and startTriMesh for trimesh generation" << std::endl;
	objID_t _id;
	_id = scene->getNextFreeID();
	if ( _id > 0 )
	{
		bool success = scene->startTriMesh(_id, vertices, triangles, hasOrco, hasUV, type);
		*id = _id;
		return success;
	}
	else
	{
		return false;
	}
}

bool yafrayInterface_t::endTriMesh() { return scene->endTriMesh(); }

int  yafrayInterface_t::addVertex(double x, double y, double z) { return scene->addVertex( point3d_t(x,y,z) ); }

int  yafrayInterface_t::addVertex(double x, double y, double z, double ox, double oy, double oz)
{
	return scene->addVertex( point3d_t(x,y,z), point3d_t(ox,oy,oz) );
}

bool yafrayInterface_t::addTriangle(int a, int b, int c, const material_t *mat) { return scene->addTriangle(a, b, c, mat); }

bool yafrayInterface_t::addTriangle(int a, int b, int c, int uv_a, int uv_b, int uv_c, const material_t *mat)
{
	return scene->addTriangle(a, b, c, uv_a, uv_b, uv_c, mat);
}

int yafrayInterface_t::addUV(float u, float v) { return scene->addUV(u, v); }

bool yafrayInterface_t::startVmap(int id, int type, int dimensions)
{
	return scene->startVmap(id, type, dimensions);
}
bool yafrayInterface_t::endVmap(){ return scene->endVmap(); }
bool yafrayInterface_t::addVmapValues(float *val){ return scene->addVmapValues(val); }

bool yafrayInterface_t::smoothMesh(unsigned int id, double angle) { return scene->smoothMesh(id, angle); }

// paraMap_t related functions:
void yafrayInterface_t::paramsSetPoint(const char* name, double x, double y, double z)
{
	(*cparams)[std::string(name)] = parameter_t(point3d_t(x,y,z));
}

void yafrayInterface_t::paramsSetString(const char* name, const char* s)
{
	(*cparams)[std::string(name)] = parameter_t(std::string(s));
}

void yafrayInterface_t::paramsSetBool(const char* name, bool b)
{
	(*cparams)[std::string(name)] = parameter_t(b);
}

void yafrayInterface_t::paramsSetInt(const char* name, int i)
{
	(*cparams)[std::string(name)] = parameter_t(i);
}

void yafrayInterface_t::paramsSetFloat(const char* name, double f)
{
	(*cparams)[std::string(name)] = parameter_t(f);
}

void yafrayInterface_t::paramsSetColor(const char* name, float r, float g, float b, float a)
{
	colorA_t col(r,g,b,a);
	if(gcInput) col.gammaAdjust(inputGamma);
	(*cparams)[std::string(name)] = parameter_t(col);
}

void yafrayInterface_t::paramsSetColor(const char* name, float *rgb, bool with_alpha)
{
	colorA_t col(rgb[0],rgb[1],rgb[2], (with_alpha ? rgb[3] : 1.f));
	if(gcInput) col.gammaAdjust(inputGamma);
	(*cparams)[std::string(name)] = parameter_t(col);
}

void yafrayInterface_t::paramsSetMatrix(const char* name, float m[4][4], bool transpose)
{
	if(transpose)	cparams->setMatrix(std::string(name), matrix4x4_t(m).transpose());
	else		cparams->setMatrix(std::string(name), matrix4x4_t(m));
}

void yafrayInterface_t::paramsSetMatrix(const char* name, double m[4][4], bool transpose)
{
	if(transpose)	cparams->setMatrix(std::string(name), matrix4x4_t(m).transpose());
	else		cparams->setMatrix(std::string(name), matrix4x4_t(m));
}

void yafrayInterface_t::paramsSetMemMatrix(const char* name, float* matrix, bool transpose)
{
	float mat[4][4];
	int i,j;
	for(i= 0; i < 4; i++)
		for(j= 0; j < 4; j++)
			mat[i][j] = *(matrix+i*4+j);
	paramsSetMatrix(name, mat, transpose);
}

void yafrayInterface_t::paramsSetMemMatrix(const char* name, double* matrix, bool transpose)
{
	double mat[4][4];
	int i,j;
	for(i= 0; i < 4; i++)
		for(j= 0; j < 4; j++)
			mat[i][j] = *(matrix+i*4+j);
	paramsSetMatrix(name, mat, transpose);
}

void yafrayInterface_t::setInputGamma(float gammaVal, bool enable)
{
	gcInput = enable;
	if(gammaVal > 0) inputGamma = gammaVal;
}

void yafrayInterface_t::paramsClearAll()
{
	params->clear();
	eparams->clear();
	cparams = params;
}

void yafrayInterface_t::paramsStartList()
{
	if(!eparams->empty()) eparams->push_back(paraMap_t());
	else std::cout << "warning, appending to existing list!\n";
	cparams = &eparams->back();
}
void yafrayInterface_t::paramsPushList()
{
	eparams->push_back(paraMap_t());
	cparams = &eparams->back();
}

void yafrayInterface_t::paramsEndList()
{
	cparams = params;
}

light_t* 		yafrayInterface_t::createLight(const char* name)
{
	light_t* light = env->createLight(name, *params);
	if(light) scene->addLight(light);
	return light;
}

texture_t* 		yafrayInterface_t::createTexture(const char* name) { return env->createTexture(name, *params); }
material_t* 	yafrayInterface_t::createMaterial(const char* name) { return env->createMaterial(name, *params, *eparams); }
camera_t* 		yafrayInterface_t::createCamera(const char* name) { return env->createCamera(name, *params); }
background_t* 	yafrayInterface_t::createBackground(const char* name) { return env->createBackground(name, *params); }
integrator_t* 	yafrayInterface_t::createIntegrator(const char* name) { return env->createIntegrator(name, *params); }
VolumeRegion* 	yafrayInterface_t::createVolumeRegion(const char* name) { 
	//return env->createVolumeRegion(name, *params);
	VolumeRegion* vr = env->createVolumeRegion(name, *params);
	if (!vr) return 0;
	scene->addVolumeRegion(vr);
	return 0;
}

unsigned int 	yafrayInterface_t::createObject	(const char* name)
{
	object3d_t *object = env->createObject(name, *params);
	if(!object) return 0;
	objID_t id;
	if( scene->addObject(object, id) ) return id;
	return 0;
}

void yafrayInterface_t::addToParamsString(const char* params) { env->addToParamsString(params);  }

void yafrayInterface_t::clearParamsString() { env->clearParamsString(); }

void yafrayInterface_t::setDrawParams(bool b) { env->setDrawParams(b); }

void yafrayInterface_t::abort(){ if(scene) scene->abort(); }

bool yafrayInterface_t::getRenderedImage(colorOutput_t &output)
{
	if(!film) return false;
	film->flush(IF_ALL, &output);
	return true;
}

char* yafrayInterface_t::getVersion() const
{
#ifdef RELEASE
	return (char*)std::string(VERSION).c_str();
#else
	return (char*)std::string(YAF_SVN_REV).c_str();
#endif
}

void yafrayInterface_t::render(colorOutput_t &output, progressBar_t *pb)
{
	if(! env->setupScene(*scene, *params, output, pb) ) return;
	scene->render();
	film = scene->getImageFilm();
	//delete film;
}

// export "factory"...

extern "C"
{
	YAFRAYPLUGIN_EXPORT yafrayInterface_t* getYafray()
	{
		return new yafrayInterface_t();
	}
}

__END_YAFRAY

