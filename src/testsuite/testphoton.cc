
#include <yafray_config.h>
#include <iostream>

#include <core_api/scene.h>
#include <core_api/environment.h>
#include <core_api/surface.h>
#include <core_api/integrator.h>
#include <core_api/imagefilm.h>
#include <yafraycore/builtincameras.h>
#include <yafraycore/tga_io.h>

#include <testsuite/simplemat.h>

using namespace::yafaray;
//__BEGIN_YAFRAY

static void cube(scene_t &scene, const point3d_t &center, yafaray::PFLOAT size, material_t *mat)
{
	int a,b,c,d,e,f,g,h;

	a = scene.addVertex(center + vector3d_t(size,size,size) );
	b = scene.addVertex(center + vector3d_t(size,-size,size) );
	c = scene.addVertex(center + vector3d_t(size,-size,-size) );
	d = scene.addVertex(center + vector3d_t(size,size,-size) );
	
	e = scene.addVertex(center + vector3d_t(-size,size,-size) );
	f = scene.addVertex(center + vector3d_t(-size,-size,-size) );
	g = scene.addVertex(center + vector3d_t(-size,-size,size) );
	h = scene.addVertex(center + vector3d_t(-size,size,size) );
	
//	if(a!=0 || b!=1 || c!=2 || d!=3) std::cout << "cube():: wrong index!\n";

	vector3d_t N(1,0,0);
	scene.addTriangle(a, b, c, mat);
	scene.addTriangle(c, d, a, mat);
	N.set(-1,0,0);
	scene.addTriangle(e, f, g, mat);
	scene.addTriangle(g, h, e, mat);
	N.set(0,-1,0);
	scene.addTriangle(g, f, c, mat);
	scene.addTriangle(g, c, b, mat);
	N.set(0,1,0);
	scene.addTriangle(a, e, h, mat);
	scene.addTriangle(e, a, d, mat);
	N.set(0,0,1);
	scene.addTriangle(a, g, b, mat);
	scene.addTriangle(g, a, h, mat);
	N.set(0,0,-1);
	scene.addTriangle(d, c, f, mat);
	scene.addTriangle(f, e, d, mat);
}

static void room(scene_t &scene, material_t *base_mat, material_t *left_wall, material_t *right_wall, material_t *floor)
{
	int a,b,c,d,e,f,g,h;
	a = scene.addVertex(point3d_t(2.1, 2.1, 2.1) );
	b = scene.addVertex(point3d_t(2.1,-4.1, 2.1) );
	c = scene.addVertex(point3d_t(2.1,-4.1,-2.1) );
	d = scene.addVertex(point3d_t(2.1, 2.1,-2.1) );
	
	e = scene.addVertex(point3d_t(-2.1, 2.1,-2.1) );
	f = scene.addVertex(point3d_t(-2.1,-4.1,-2.1) );
	g = scene.addVertex(point3d_t(-2.1,-4.1, 2.1) );
	h = scene.addVertex(point3d_t(-2.1, 2.1, 2.1) );
	//right wall (when viewing in positive y-dir):
	scene.addTriangle(a, c, b, right_wall);
	scene.addTriangle(c, a, d, right_wall);
	//left wall
	scene.addTriangle(e, g, f, left_wall);
	scene.addTriangle(g, e, h, left_wall);
	//front wall (behind cam)
	scene.addTriangle(g, c, f, base_mat);
	scene.addTriangle(g, b, c, base_mat);
	//back wall
	scene.addTriangle(a, h, e, base_mat);
	scene.addTriangle(a, e, d, base_mat);
	//ceiling
	scene.addTriangle(a, b, g, base_mat);
	scene.addTriangle(a, g, h, base_mat);
	// floor
	scene.addTriangle(d, f, c, floor);
	scene.addTriangle(d, e, f, floor);
	
	//
	simplemat_t *dark = new simplemat_t(color_t(0.0, 0.0, 0.0));
	a = scene.addVertex(point3d_t(-0.5, -0.5, 2.02) );
	b = scene.addVertex(point3d_t(-0.5,  0.5, 2.02) );
	c = scene.addVertex(point3d_t( 0.5,  0.5, 2.02) );
	d = scene.addVertex(point3d_t( 0.5, -0.5, 2.02) );
	scene.addTriangle(a, b, c, dark);
	scene.addTriangle(a, c, d, dark);
}

static void cuboid(scene_t &scene, const point3d_t &p1, const point3d_t &p2, material_t *mat)
{
	int a,b,c,d,e,f,g,h;

	a = scene.addVertex(point3d_t(p2.x, p2.y, p2.z) );
	b = scene.addVertex(point3d_t(p2.x, p1.y, p2.z) );
	c = scene.addVertex(point3d_t(p2.x, p1.y, p1.z) );
	d = scene.addVertex(point3d_t(p2.x, p2.y, p1.z) );
	
	e = scene.addVertex(point3d_t(p1.x, p2.y, p1.z) );
	f = scene.addVertex(point3d_t(p1.x, p1.y, p1.z) );
	g = scene.addVertex(point3d_t(p1.x, p1.y, p2.z) );
	h = scene.addVertex(point3d_t(p1.x, p2.y, p2.z) );
	

	scene.addTriangle(a, b, c, mat);
	scene.addTriangle(c, d, a, mat);

	scene.addTriangle(e, f, g, mat);
	scene.addTriangle(g, h, e, mat);

	scene.addTriangle(g, f, c, mat);
	scene.addTriangle(g, c, b, mat);

	scene.addTriangle(a, e, h, mat);
	scene.addTriangle(e, a, d, mat);

	scene.addTriangle(a, g, b, mat);
	scene.addTriangle(g, a, h, mat);

	scene.addTriangle(d, c, f, mat);
	scene.addTriangle(f, e, d, mat);
}

bool loadPly(scene_t *s, material_t *mat, const char *plyfile, double scale);

int main()
{
	paraMap_t params;
	renderEnvironment_t *env = new renderEnvironment_t();
	std::string ppath;
	if (env->getPluginPath(ppath))
	{
		std::cout << "the plugin path is:\n" << ppath << "\n";
		env->loadPlugins(ppath);
	}
	else std::cout << "getting plugin path from render environment failed!\n";
	
	std::cout << "creating TGA output;\n";
	outTga_t *out = new outTga_t(400, 400, "photon_bounce.tga", false);
	outTga_t *out2 = new outTga_t(400, 400, "test_cmp.tga", false);
	std::cout << "creating scene instance;\n";
	scene_t *scene = new scene_t();
	scene->setAntialiasing(1, 1, 1, 0.05);
	//camera:
	std::cout << "adding camera;\n";
	perspectiveCam_t *camera = new perspectiveCam_t(point3d_t(0,-3.0,-0.5), point3d_t(0,0,-0.2), point3d_t(0,-3.0,1), 400,400, 1, 1.0);
//	perspectiveCam_t *camera = new perspectiveCam_t(point3d_t(0,-3.0,1.0), point3d_t(0,0,0.5), point3d_t(0,-3.0,2.0), 400,400, 1, 1.0);
	scene->setCamera(camera);
	//image film:
	imageFilm_t *myFilm = new imageFilm_t(400, 400, 0, 0, *out, 1.5);
	scene->setImageFilm(myFilm);
	//some materials:
//	params["type"] = parameter_t(std::string("glass"));
//	params["IOR"] = parameter_t( 1.4f );
//	std::cout << "creating a simplemat;\n";
	std::list<paraMap_t> dummy;
//	material_t *glassMat = env->createMaterial("myGlass", params, dummy);
	simplemat_t *mat = new simplemat_t(color_t(0.66, 0.66, 0.66));
	simplemat_t *mat2 = new simplemat_t(color_t(1.0, 0.15, 0.1), 0.8);
	simplemat_t *blueMat = new simplemat_t(color_t(0.15, 0.15, 0.75));
	simplemat_t *redMat = new simplemat_t(color_t(0.75, 0.15, 0.15));
	//a trivial integrator:
	params.clear();
//	params["type"] = parameter_t(std::string("directlighting"));
	params["type"] = parameter_t(std::string("photonmapping"));
//	params["type"] = parameter_t(std::string("pathtracing"));
	params["transpShad"] = parameter_t(true);
	params["photons"] = parameter_t(100000);
	params["search"] = parameter_t(75);
	params["diffuseRadius"] = parameter_t(0.05f);
	surfaceIntegrator_t* integrator = (surfaceIntegrator_t*)env->createIntegrator("myDL", params);
	if(!integrator) exit(1);
	std::cout << "adding integrator to scene;\n";
	scene->setSurfIntegrator(integrator);
	//a bit geometry:
	std::cout << "adding geometry;\n";
	if(!scene->startGeometry()){ std::cout << "error on startGeometry!\n"; return 1; }
	objID_t id;
	if(!scene->startTriMesh(id,8,12,false,false)){ std::cout << "error on startTriMesh!\n"; return 1; }
//	std::cout << "ading a cube...\n";
	room(*scene, mat, blueMat, redMat, mat);
	cuboid(*scene, point3d_t(0, -2.0, 0.1), point3d_t(2.0, 2.1, 0.2), mat);
//	cube(*scene, point3d_t(0.0, 0.0, 0.0), 2.10, mat);
//	cube(*scene, point3d_t(0.0, 0.0, 0.0), 0.35, mat);
	cube(*scene, point3d_t(-0.5, -0.2, 0.21), 0.1, mat2);
	if(!scene->endTriMesh()){ std::cout << "error on endTriMesh!\n"; return 1; }

	//try to load a ply file:
//	std::cout << "trying to load a ply...";
//	bool success = loadPly(scene, mat, "D:\\xyzrgb_statuette.ply", 0.0015);
//	bool success = loadPly(scene, mat, "D:\\dragon.ply", 3.0);
//	bool success = loadPly(scene, glassMat, "D:\\bunny.ply", 4.0);
//	scene->smoothMesh(2, 180.1);
//	if(success) std::cout << "done!\n";
//	else std::cout << "failed!\n";
	
	std::cout << "finishing geometry;\n";
	if(!scene->endGeometry()){ std::cout << "error on endGeometry!\n"; return 1; }
	
	light_t* myLight=0;
	// create a light:
/*	params.clear();
	params["type"] = parameter_t(std::string("pointlight"));
	params["from"] = parameter_t( point3d_t(0.0, 0.0, 1.5) );
	params["color"] = parameter_t( colorA_t(1.0, 1.0, 0.9) );
	params["power"] = parameter_t( 10.f );
	myLight = env->createLight("myLight", params);
	std::cout << "light pointer:" << (void *)myLight << "\n";
	scene->addLight(myLight);
*/	
	// create an area light:
	params.clear();
	params["type"] = parameter_t(std::string("arealight"));
	params["corner"] = parameter_t( point3d_t(-0.5, -0.5, 2.0) );
	params["point1"] = parameter_t( point3d_t(-0.5, 0.5, 2.0) );
	params["point2"] = parameter_t( point3d_t(0.5, -0.5, 2.0) );
	params["color"] = parameter_t( colorA_t(1.0, 1.0, 1.0) );
	params["power"] = parameter_t( 15.f );
	params["samples"] = parameter_t( 8 );
	myLight = env->createLight("myAreaLight", params);
	std::cout << "light pointer:" << (void *)myLight << "\n";
	scene->addLight(myLight);
	
	//create background:
/*	params.clear();
	params["type"] = parameter_t(std::string("sunsky"));
	params["from"] = parameter_t( point3d_t(1.0, 1.0, 1.0) );
	background_t *myBack = env->createBackground("mySunsky", params);
	scene->setBackground(myBack);
*/	
	// update scene manually (probably done automatically later when rendering)
	if(!scene->update()){ std::cout << "error on update!\n"; return 1; }

	// render the scene
	std::cout << "rendering scene!\n";
	scene->render();

	// save the tga file:
	out->flush();
	
	//clean up...although the only effect could be a crash due to destructor errors...
	delete mat;
	delete camera;
	delete scene;
	return 0;
}

//__END_YAFRAY
