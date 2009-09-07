
#include <interface/yafrayinterface.h>
#include <yafraycore/tga_io.h>
#include <iostream>
#include <string>
#include <cstdlib>

using namespace::yafaray;

int main()
{
	yafrayInterface_t *yi = new yafrayInterface_t();
	yi->loadPlugins(0); //you may call loadPlugins with additional paths
	
	//first of all, start scene
	std::cout << "starting scene" << std::endl;
	yi->startScene();
	
	//create a simple texture
	yi->paramsClearAll();
	std::string texName("rgb_cube1");
	yi->paramsSetString("type", "rgb_cube");
	yi->createTexture(texName.c_str());
	
	//create simple material
	yi->paramsClearAll();
	yi->paramsSetString("type", "shinydiffusemat");
	
	yi->paramsPushList();
	yi->paramsSetString("type", "texture_mapper");
	yi->paramsSetString("name", "rgbcube_mapper");
	yi->paramsSetString("texco", "uv");
	yi->paramsSetString("texture", texName.c_str() );
	yi->paramsEndList();
	
	yi->paramsSetString("diffuse_shader", "rgbcube_mapper");
	
	material_t *mat = yi->createMaterial("myMat");
	
	if(! yi->startGeometry() ) std::cout << "error occured on startGeometry\n";
	unsigned int id;
	//create a cube with UVs
	yi->startTriMesh(id, 8, 12, false, true);
	//vertices:
	const double size = 0.5;
	int a,b,c,d,e,f,g,h;
	a = yi->addVertex(size,size,size);
	b = yi->addVertex(size,-size,size);
	c = yi->addVertex(size,-size,-size);
	d = yi->addVertex(size,size,-size);
	
	e = yi->addVertex(-size,size,-size);
	f = yi->addVertex(-size,-size,-size);
	g = yi->addVertex(-size,-size,size);
	h = yi->addVertex(-size,size,size);
	//UVs:
	int ua,ub,uc,ud,ue,uf,ug,uh,ui,uj;
	ua = yi->addUV(0.0, 0.0);
	ub = yi->addUV(0.0, 0.25);
	uc = yi->addUV(0.0, 0.5);
	ud = yi->addUV(0.0, 0.75);
	ue = yi->addUV(0.0, 1.0);
	
	uf = yi->addUV(1.0, 0.0);
	ug = yi->addUV(1.0, 0.25);
	uh = yi->addUV(1.0, 0.5);
	ui = yi->addUV(1.0, 0.75);
	uj = yi->addUV(1.0, 1.0);
	//faces:
	yi->addTriangle(a, b, c, uf, ug, ub, mat);
	yi->addTriangle(c, d, a, ub, ua, uf, mat);
	// left N.set(-1,0,0);
	yi->addTriangle(e, f, g, ud, uc, uh, mat);
	yi->addTriangle(g, h, e, uh, ui, ud, mat);
	// front N.set(0,-1,0);
	yi->addTriangle(g, f, c, uh, uc, ub, mat);
	yi->addTriangle(g, c, b, uh, ub, ug, mat);
	// back N.set(0,1,0);
	yi->addTriangle(a, e, h, uj, ud, ui, mat);
	yi->addTriangle(e, a, d, ud, uj, ue, mat);
	// top N.set(0,0,1);
	yi->addTriangle(a, g, b, uf, ue, ua, mat);
	yi->addTriangle(g, a, h, ue, uf, uj, mat);
	// bottom N.set(0,0,-1);
	yi->addTriangle(d, c, f, uf, ua, ue, mat);
	yi->addTriangle(f, e, d, ue, uj, uf, mat);
	
	yi->endTriMesh();
	yi->endGeometry();
	
	// create an integrator
	yi->paramsClearAll();
	yi->paramsSetString("type", "directlighting");
	integrator_t* integrator = yi->createIntegrator("myDL");
	if(!integrator) exit(1);
	
	// create light
	yi->paramsClearAll();
	yi->paramsSetString("type", "directional");
	yi->paramsSetPoint("direction", -0.3, -0.3, 0.8 );
	yi->paramsSetColor("color", 1.0, 1.0, 0.9 );
	yi->paramsSetFloat("power", 1.0 );
	yi->createLight("myDirectional");
	
	// create background
	yi->paramsClearAll();
	yi->paramsSetString("type", "constant");
	yi->paramsSetColor("color", 0.4, 0.5, 0.9 );
	yi->createBackground("world_background");

	// create canera
	yi->paramsClearAll();
	yi->paramsSetString("type", "perspective");
	yi->paramsSetPoint("from", -1.5,-2.0,1.7 );
	yi->paramsSetPoint("to", 0,0,0.2 );
	yi->paramsSetPoint("up", -1.5,-2.0,2.7 );
	yi->paramsSetInt("resx", 400);
	yi->paramsSetInt("resy", 300);
	yi->paramsSetFloat("focal", 1.4);
	yi->createCamera("camera");
	//image film:
	
	//render
	yi->paramsClearAll();
	yi->paramsSetString("camera_name", "camera");
	yi->paramsSetString("integrator_name", "myDL");
	
	yi->paramsSetInt("AA_minsamples", 4);
	yi->paramsSetFloat("AA_pixelwidth", 1.5);
	yi->paramsSetString("filter_type", "mitchell");
	yi->paramsSetInt("width", 400);
	yi->paramsSetInt("height", 300);
	yi->paramsSetString("background_name", "world_background");
	
	
	outTga_t *output = new outTga_t(400, 300, "test.tga", false);
	yi->render(*output);
	
	yi->clearAll();
	delete yi;
	
	//save tga file:
	output->flush();
	
	return 0;
}
