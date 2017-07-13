
#include <interface/yafrayinterface.h>
#include <core_api/environment.h>
#include <core_api/scene.h>
#include <core_api/imagefilm.h>
#include <core_api/integrator.h>
#include <core_api/matrix4.h>
#include <signal.h>

#ifdef WIN32
	#include <windows.h>
#endif

__BEGIN_YAFRAY

scene_t *globalScene = nullptr;

#ifdef WIN32
BOOL WINAPI ctrl_c_handler(DWORD signal) {
	if(globalScene)
	{
		globalScene->abort(); 
		session.setStatusRenderAborted();
		Y_WARNING << "Interface: Render aborted by user." << yendl;
	}
	else
	{
		session.setStatusRenderAborted();
		Y_WARNING << "Interface: Render aborted by user." << yendl;
		exit(1);
	}
    return TRUE;
}
#else
void ctrl_c_handler(int signal)
{
	if(globalScene)
	{
		globalScene->abort(); 
		session.setStatusRenderAborted();
		Y_WARNING << "Interface: Render aborted by user." << yendl;
	}
	else
	{
		session.setStatusRenderAborted();
		Y_WARNING << "Interface: Render aborted by user." << yendl;
		exit(1);
	}	
}
#endif

yafrayInterface_t::yafrayInterface_t(): scene(nullptr), film(nullptr), inputGamma(1.f), inputColorSpace(RAW_MANUAL_GAMMA)
{
	//handle CTRL+C events
#ifdef WIN32
	SetConsoleCtrlHandler(ctrl_c_handler, true);
#else
	struct sigaction signalHandler;
	signalHandler.sa_handler = ctrl_c_handler;
	sigemptyset(&signalHandler.sa_mask);
	signalHandler.sa_flags = 0;
	sigaction(SIGINT, &signalHandler, nullptr);
#endif

	env = new renderEnvironment_t();
	params = new paraMap_t;
	eparams = new std::list<paraMap_t>;
	cparams = params;
}

yafrayInterface_t::~yafrayInterface_t()
{
	Y_VERBOSE << "Interface: Deleting scene..." << yendl;
	if(scene) delete scene;
	Y_VERBOSE << "Interface: Deleting environment..." << yendl;
	if(env) delete env;
	Y_INFO << "Interface: Done." << yendl;
	if(film) delete film;
	delete params;
	delete eparams;
	yafLog.clearAll();
}

void yafrayInterface_t::loadPlugins(const char *path)
{
	if(path != nullptr)
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
	Y_VERBOSE << "Interface: Cleaning environment..." << yendl;
	env->clearAll();
	Y_VERBOSE << "Interface: Deleting scene..." << yendl;
	if(scene) delete scene;
	Y_VERBOSE << "Interface: Clearing film and parameter maps scene..." << yendl;
	scene = nullptr;//new scene_t();
	if(film) delete film;
	film = nullptr;
	params->clear();
	eparams->clear();
	cparams = params;
	Y_VERBOSE << "Interface: Cleanup done." << yendl;
}

bool yafrayInterface_t::startScene(int type)
{
	if(scene) delete scene;
	scene = new scene_t(env);
	globalScene = scene;	//for the CTRL+C handler
	scene->setMode(type);
	env->setScene(scene);
	return true;
}

bool yafrayInterface_t::setLoggingAndBadgeSettings()
{
	env->setupLoggingAndBadge(*params);
	return true;
}

bool yafrayInterface_t::setupRenderPasses()
{
	env->setupRenderPasses(*params);
	return true;
}

bool yafrayInterface_t::setInteractive(bool interactive)
{
	session.setInteractive(interactive);
	return true;
}

bool yafrayInterface_t::startGeometry() { return scene->startGeometry(); }

bool yafrayInterface_t::endGeometry() { return scene->endGeometry(); }

unsigned int yafrayInterface_t::getNextFreeID() {
	objID_t id;
	id = scene->getNextFreeID();
	return id;
}

bool yafrayInterface_t::startTriMesh(unsigned int id, int vertices, int triangles, bool hasOrco, bool hasUV, int type, int obj_pass_index)
{
	bool success = scene->startTriMesh(id, vertices, triangles, hasOrco, hasUV, type, obj_pass_index);
	return success;
}

bool yafrayInterface_t::startCurveMesh(unsigned int id, int vertices, int obj_pass_index)
{
        bool success = scene->startCurveMesh(id, vertices, obj_pass_index);
        return success;
}


bool yafrayInterface_t::startTriMeshPtr(unsigned int *id, int vertices, int triangles, bool hasOrco, bool hasUV, int type, int obj_pass_index)
{
	Y_WARNING << "Interface: This method is going to be removed, please use getNextFreeID() and startTriMesh() for trimesh generation" << yendl;
	objID_t _id;
	_id = scene->getNextFreeID();
	if ( _id > 0 )
	{
		bool success = scene->startTriMesh(_id, vertices, triangles, hasOrco, hasUV, type, obj_pass_index);
		*id = _id;
		return success;
	}
	else
	{
		return false;
	}
}

bool yafrayInterface_t::endTriMesh() { return scene->endTriMesh(); }
bool yafrayInterface_t::endCurveMesh(const material_t *mat, float strandStart, float strandEnd, float strandShape) { return scene->endCurveMesh(mat, strandStart, strandEnd, strandShape); }

int  yafrayInterface_t::addVertex(double x, double y, double z) { return scene->addVertex( point3d_t(x,y,z) ); }

int  yafrayInterface_t::addVertex(double x, double y, double z, double ox, double oy, double oz)
{
	return scene->addVertex(point3d_t(x,y,z), point3d_t(ox,oy,oz));
}

void yafrayInterface_t::addNormal(double x, double y, double z)
{
	scene->addNormal( normal_t(x,y,z) );
}

bool yafrayInterface_t::addTriangle(int a, int b, int c, const material_t *mat) { return scene->addTriangle(a, b, c, mat); }

bool yafrayInterface_t::addTriangle(int a, int b, int c, int uv_a, int uv_b, int uv_c, const material_t *mat)
{
	return scene->addTriangle(a, b, c, uv_a, uv_b, uv_c, mat);
}

int yafrayInterface_t::addUV(float u, float v) { return scene->addUV(u, v); }

bool yafrayInterface_t::smoothMesh(unsigned int id, double angle) { return scene->smoothMesh(id, angle); }

bool yafrayInterface_t::addInstance(unsigned int baseObjectId, matrix4x4_t objToWorld)
{
	return scene->addInstance(baseObjectId, objToWorld);
}
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
	col.linearRGB_from_ColorSpace(inputColorSpace, inputGamma);
	(*cparams)[std::string(name)] = parameter_t(col);
}

void yafrayInterface_t::paramsSetColor(const char* name, float *rgb, bool with_alpha)
{
	colorA_t col(rgb[0],rgb[1],rgb[2], (with_alpha ? rgb[3] : 1.f));
	col.linearRGB_from_ColorSpace(inputColorSpace, inputGamma);
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

void yafrayInterface_t::setInputColorSpace(std::string color_space_string, float gammaVal)
{
	if(color_space_string == "sRGB") inputColorSpace = SRGB;
	else if(color_space_string == "XYZ") inputColorSpace = XYZ_D65;
	else if(color_space_string == "LinearRGB") inputColorSpace = LINEAR_RGB;
	else if(color_space_string == "Raw_Manual_Gamma") inputColorSpace = RAW_MANUAL_GAMMA;
	else inputColorSpace = SRGB;
	
	inputGamma = gammaVal;
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
	else Y_WARNING << "Interface: Appending to existing list!" << yendl;
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

light_t* yafrayInterface_t::createLight(const char* name)
{
	light_t* light = env->createLight(name, *params);
	if(light) scene->addLight(light);
	return light;
}

texture_t* 		yafrayInterface_t::createTexture(const char* name) { return env->createTexture(name, *params); }
material_t* 	yafrayInterface_t::createMaterial(const char* name) { return env->createMaterial(name, *params, *eparams); }
camera_t* 		yafrayInterface_t::createCamera(const char* name)
{
	camera_t *camera = env->createCamera(name, *params);
	return camera;
}
background_t* 	yafrayInterface_t::createBackground(const char* name) { return env->createBackground(name, *params); }
integrator_t* 	yafrayInterface_t::createIntegrator(const char* name) { return env->createIntegrator(name, *params); }
imageHandler_t* yafrayInterface_t::createImageHandler(const char* name, bool addToTable) { return env->createImageHandler(name, *params, addToTable); }

VolumeRegion* 	yafrayInterface_t::createVolumeRegion(const char* name)
{
	VolumeRegion* vr = env->createVolumeRegion(name, *params);
	if (!vr) return nullptr;
	scene->addVolumeRegion(vr);
	return nullptr;
}

unsigned int yafrayInterface_t::createObject(const char* name)
{
	object3d_t *object = env->createObject(name, *params);
	if(!object) return 0;
	objID_t id;
	if( scene->addObject(object, id) ) return id;
	return 0;
}

void yafrayInterface_t::abort()
{
	if(scene) scene->abort(); 
	session.setStatusRenderAborted();
	Y_WARNING << "Interface: Render aborted by user." << yendl;
}

bool yafrayInterface_t::getRenderedImage(int numView, colorOutput_t &output)
{
	if(!film) return false;
	film->flush(numView, IF_ALL, &output);
	return true;
}

std::vector<std::string> yafrayInterface_t::listImageHandlers()
{
	return env->listImageHandlers();
}

std::vector<std::string> yafrayInterface_t::listImageHandlersFullName()
{
	return env->listImageHandlersFullName();
}

std::string yafrayInterface_t::getImageFormatFromFullName(const std::string &fullname)
{
	return env->getImageFormatFromFullName(fullname);
}

std::string yafrayInterface_t::getImageFullNameFromFormat(const std::string &format)
{
	return env->getImageFullNameFromFormat(format);
}

char* yafrayInterface_t::getVersion() const
{
	return (char*)session.getYafaRayCoreVersion().c_str();
}

void yafrayInterface_t::printDebug(const std::string &msg)
{
	Y_DEBUG << msg << yendl;
}

void yafrayInterface_t::printVerbose(const std::string &msg)
{
	Y_VERBOSE << msg << yendl;
}

void yafrayInterface_t::printInfo(const std::string &msg)
{
	Y_INFO << msg << yendl;
}

void yafrayInterface_t::printParams(const std::string &msg)
{
	Y_PARAMS << msg << yendl;
}

void yafrayInterface_t::printWarning(const std::string &msg)
{
	Y_WARNING << msg << yendl;
}

void yafrayInterface_t::printError(const std::string &msg)
{
	Y_ERROR << msg << yendl;
}

void yafrayInterface_t::render(colorOutput_t &output, progressBar_t *pb)
{
	if(! env->setupScene(*scene, *params, output, pb) ) return;
	session.setStatusRenderStarted();
	scene->render();
	film = scene->getImageFilm();
	//delete film;
}

void yafrayInterface_t::setParamsBadgePosition(const std::string &badgePosition)
{
	yafLog.setParamsBadgePosition(badgePosition);
}

bool yafrayInterface_t::getDrawParams()
{
	bool dp = false;
	
	dp = yafLog.getUseParamsBadge();
	
	return dp;
}

void yafrayInterface_t::setOutput2(colorOutput_t *out2)
{
	if(env) env->setOutput2(out2);
}

void yafrayInterface_t::setConsoleVerbosityLevel(const std::string &strVLevel)
{
	yafLog.setConsoleMasterVerbosity(strVLevel);
}

void yafrayInterface_t::setLogVerbosityLevel(const std::string &strVLevel)
{
	yafLog.setLogMasterVerbosity(strVLevel);
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

