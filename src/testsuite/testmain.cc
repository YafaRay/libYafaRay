
#include <yafray_config.h>
#include <iostream>
#include <cstdlib>

#include <core_api/scene.h>
#include <core_api/environment.h>
#include <core_api/surface.h>
#include <core_api/texture.h>
#include <core_api/integrator.h>
#include <core_api/imagefilm.h>
#include <yafraycore/builtincameras.h>
#include <yafraycore/tga_io.h>

#include <testsuite/simplemat.h>

using namespace::yafray;
//__BEGIN_YAFRAY

class trivialIntegrator_t: public surfaceIntegrator_t
{
	public:
	trivialIntegrator_t();
	trivialIntegrator_t(scene_t &s):dupliScene(&s) {}
	trivialIntegrator_t(scene_t *s):dupliScene(s) {}
	virtual colorA_t integrate(renderState_t &state, ray_t &ray/*, sampler_t &sam*/) const;
	scene_t *dupliScene;
};

colorA_t trivialIntegrator_t::integrate(renderState_t &state, ray_t &ray/*, sampler_t &sam*/) const
{
	surfacePoint_t sp;
//	std::cout << "integrate(): ray from (" << ray.from.x <<", "<<ray.from.y<<", "<<ray.from.z<<")\n"<<
//		 "\t to(" << ray.dir.x <<", "<<ray.dir.y<<", "<<ray.dir.z<<")\n";
	if(dupliScene->intersect(ray, sp))
	{
//		std::cout << "intersection! returning white\n";
		return colorA_t(1.0, 1.0, 1.0, 1.0);
	}
	else
	{
//		std::cout << "no intersection! returning dark\n";
		return colorA_t(0.1, 0.1, 0.1, 0.0);
	}
}
/*namespace yafray{
class directLighting_t: public surfaceIntegrator_t
{
	public:
	directLighting_t(bool transpShad=false, int shadowDepth=4);
	virtual void preprocess() { }
	virtual colorA_t integrate(renderState_t &state, ray_t &ray) const;
	static integrator_t* factory(paraMap_t &params, renderEnvironment_t &render);
	protected:
	background_t *background;
	bool trShad;
	int sDepth, rDepth;
};
}*/
static void cube(scene_t &scene, const point3d_t &center, yafray::PFLOAT size, material_t *mat)
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

//cube with UVs
static void cubeUV(scene_t &scene, const point3d_t &center, yafray::PFLOAT size, material_t *mat)
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
	
	scene.addUV(0.0, 0.0);
	scene.addUV(0.0, 0.25);
	scene.addUV(0.0, 0.5);
	scene.addUV(0.0, 0.75);
	scene.addUV(0.0, 1.0);
	
	scene.addUV(1.0, 0.0);
	scene.addUV(1.0, 0.25);
	scene.addUV(1.0, 0.5);
	scene.addUV(1.0, 0.75);
	scene.addUV(1.0, 1.0);
	// right vector3d_t N(1,0,0);
	scene.addTriangle(a, b, c, 5, 6, 1, mat);
	scene.addTriangle(c, d, a, 1, 0, 5, mat);
	// left N.set(-1,0,0);
	scene.addTriangle(e, f, g, 3, 2, 7, mat);
	scene.addTriangle(g, h, e, 7, 8, 3, mat);
	// front N.set(0,-1,0);
	scene.addTriangle(g, f, c, 7, 2, 1, mat);
	scene.addTriangle(g, c, b, 7, 1, 6, mat);
	// back N.set(0,1,0);
	scene.addTriangle(a, e, h, 9, 3, 8, mat);
	scene.addTriangle(e, a, d, 3, 9, 4, mat);
	// top N.set(0,0,1);
	scene.addTriangle(a, g, b, 5, 4, 0, mat);
	scene.addTriangle(g, a, h, 4, 5, 9, mat);
	// bottom N.set(0,0,-1);
	scene.addTriangle(d, c, f, 5, 0, 4, mat);
	scene.addTriangle(f, e, d, 4, 9, 5, mat);
}


bool loadPly(scene_t *s, material_t *mat, const char *plyfile, double scale);

int main(int argc, char *argv[])
{
	paraMap_t params;
	std::cout << "creating YafaRay render environment;\n";
	renderEnvironment_t *env = new renderEnvironment_t();
	std::string ppath;
	if (env->getPluginPath(ppath))
	{
		std::cout << "the plugin path is:\n" << ppath << "\n";
		env->loadPlugins(ppath);
	}
	else std::cout << "getting plugin path from render environment failed!\n";
	
	std::cout << "creating TGA output;\n";
	outTga_t *out = new outTga_t(400, 300, "test.tga", false);
	outTga_t *out2 = new outTga_t(400, 300, "test_cmp.tga", false);
	std::cout << "creating scene instance;\n";
	scene_t *scene = new scene_t();
	scene->setAntialiasing(8, 1, 1, 0.05);
	//camera:
	std::cout << "adding camera;\n";
	perspectiveCam_t *camera = new perspectiveCam_t(point3d_t(0,-2.0,0.7), point3d_t(0,0,0.2), point3d_t(0,-2.0,1), 400,300, 1, 1.4);
	scene->setCamera(camera);
	//image film:
	imageFilm_t *myFilm = new imageFilm_t(400, 300, 0, 0, *out2, 1.5);
	myFilm->setClamp(false);
	scene->setImageFilm(myFilm);
	//create a texutre:
	std::string texName1("rgb_cube1");
	params.clear();
	params["type"] = parameter_t(std::string("rgb_cube"));
	texture_t *myTex = env->createTexture(texName1, params);
	params.clear();
	//std::string texName1("my_clouds");
//	params["type"] = parameter_t(std::string("clouds"));
//	params["color1"] = parameter_t( color_t(0.2, 0.3, 0.8));
//	params["size"] = parameter_t( 0.03f );
//	myTex = env->createTexture("my_clouds", params);
	params["type"] = parameter_t(std::string("marble"));
	params["color1"] = parameter_t( color_t(0.3, 0.5, 1.0));
	params["size"] = parameter_t( 5.f );
	params["turbulence"] = parameter_t(5.f);
	params["depth"] = parameter_t(4);
	params["noise_type"] = parameter_t(std::string("voronoi_f1"));
	myTex = env->createTexture("my_clouds", params);
	
	/////////////////
	// some materials:
	params.clear();
//	params["type"] = parameter_t(std::string("glass"));
	params["type"] = parameter_t(std::string("mirror"));
	params["reflect"] = parameter_t(0.8f);
	params["IOR"] = parameter_t( 1.4f );
	std::cout << "creating a simplemat;\n";
	std::list<paraMap_t> dummy;
	material_t *glassMat = env->createMaterial("myGlass", params, dummy);
	simplemat_t *mat = new simplemat_t(color_t(0.8, 0.85, 1.0));
	//simplemat_t *mat2 = new simplemat_t(color_t(1.0, 0.15, 0.1), 0.8);
	simplemat_t *mat2 = new simplemat_t(color_t(1.0, 0.15, 0.1), 0.f, 0.f, myTex);
	
	params.clear();
	params["type"] = parameter_t(std::string("blendermat"));
	params["diffuse_shader"] = parameter_t(std::string("mixer"));
	std::list<paraMap_t> eparams;
	eparams.push_back(paraMap_t());
	paraMap_t &node = eparams.back();
	node["type"] = parameter_t(std::string("texture_mapper"));
	node["name"] = parameter_t(std::string("rgbcube_mapper"));
	node["texco"] = parameter_t(std::string("uv"));
	node["texture"] = parameter_t(texName1);
	node["do_scalar"] = parameter_t( bool(false));
	eparams.push_back(paraMap_t());
	paraMap_t &node2 = eparams.back();
	node2["type"] = parameter_t(std::string("texture_mapper"));
	node2["name"] = parameter_t(std::string("cloud_mapper"));
	node2["texco"] = parameter_t(std::string("uv"));
	node2["texture"] = parameter_t(std::string("my_clouds"));
	eparams.push_back(paraMap_t());
	paraMap_t &node3 = eparams.back();
	node3["type"] = parameter_t(std::string("value"));
	node3["name"] = parameter_t(std::string("blue"));
	node3["color"] = parameter_t(color_t(0.3, 0.4, 1.0));
	eparams.push_back(paraMap_t());
	paraMap_t &node4 = eparams.back();
	node4["type"] = parameter_t(std::string("mix"));
	node4["name"] = parameter_t(std::string("mixer"));
	node4["input1"] = parameter_t(std::string("rgbcube_mapper"));
	node4["input2"] = parameter_t(std::string("blue"));
	node4["factor"] = parameter_t(std::string("cloud_mapper"));
	material_t *nodeMat = env->createMaterial("myBlendermat", params, eparams);
	
	
	// some integrator:
//	trivialIntegrator_t* integrator = new trivialIntegrator_t(scene);
	params.clear();
	params["type"] = parameter_t(std::string("directlighting"));
	//params["type"] = parameter_t(std::string("photonmapping"));
	params["transpShad"] = parameter_t(true);
	params["photons"] = parameter_t(2600000);
	surfaceIntegrator_t* integrator = (surfaceIntegrator_t*)env->createIntegrator("myDL", params);
//	directLighting_t* integrator = new directLighting_t(true, 4);
	if(!integrator) exit(1);
	std::cout << "adding integrator to scene;\n";
	scene->setSurfIntegrator(integrator);
	//a bit geometry:
	std::cout << "adding geometry;\n";
	if(!scene->startGeometry()){ std::cout << "error on startGeometry!\n"; return 1; }
	objID_t id;
	if(!scene->startTriMesh(id,8,12,false,false)){ std::cout << "error on startTriMesh!\n"; return 1; }
//	std::cout << "ading a cube...\n";
	cube(*scene, point3d_t(0.0, 0.0, -1.0), 1.10, mat);
//	cube(*scene, point3d_t(0.0, 0.0, 0.0), 0.35, mat);
	if(!scene->endTriMesh()){ std::cout << "error on endTriMesh!\n"; return 1; }
	if(!scene->startTriMesh(id,8,12,false,true)){ std::cout << "error on startTriMesh!\n"; return 1; }
//	cubeUV(*scene, point3d_t(-0.5, -0.2, 0.21), 0.1, mat2);
	cubeUV(*scene, point3d_t(-0.5, -0.2, 0.21), 0.1, nodeMat);
	if(!scene->endTriMesh()){ std::cout << "error on endTriMesh!\n"; return 1; }

	//try to load a ply file:
	std::cout << "trying to load a ply...";
//	bool success = loadPly(scene, mat, "D:\\xyzrgb_statuette.ply", 0.0015);
//	bool success = loadPly(scene, mat, "D:\\dragon.ply", 3.0);
	bool success = loadPly(scene, glassMat, "/media/hda6/bunny.ply", 4.0);
	scene->smoothMesh(2, 180.1);
	if(success) std::cout << "done!\n";
	else std::cout << "failed!\n";
	
	std::cout << "finishing geometry;\n";
	if(!scene->endGeometry()){ std::cout << "error on endGeometry!\n"; return 1; }
	
	//////////////////
	// create some lights:
	params.clear();
	params["type"] = parameter_t(std::string("pointlight"));
	params["from"] = parameter_t( point3d_t(-5.0, -6.0, 6.0) );
	params["color"] = parameter_t( colorA_t(1.0, 1.0, 0.9) );
//	params["power"] = parameter_t( 10.f );
	params["power"] = parameter_t( 60.f );
	light_t* myLight = env->createLight("myLight", params);
	std::cout << "light pointer:" << (void *)myLight << "\n";
	scene->addLight(myLight);
/*	
	// create an area light:
	params.clear();
	params["type"] = parameter_t(std::string("arealight"));
	params["corner"] = parameter_t( point3d_t(-5.0, 6.0, 6.0) );
	params["point1"] = parameter_t( point3d_t(-3.0, 7.0, 6.0) );
	params["point2"] = parameter_t( point3d_t(-5.0, 5.0, 8.0) );
	params["color"] = parameter_t( colorA_t(1.0, 1.0, 0.9) );
	params["power"] = parameter_t( 20.f );
	params["samples"] = parameter_t( 4 );
	myLight = env->createLight("myAreaLight", params);
	std::cout << "light pointer:" << (void *)myLight << "\n";
	scene->addLight(myLight);

	// create a directional light:
	params.clear();
	params["type"] = parameter_t(std::string("directional"));
	params["position"] = parameter_t( point3d_t(-0.4, -0.2, 0.21) );
	params["direction"] = parameter_t( point3d_t(0.0, 0.0, 1.0) );
	params["radius"] = parameter_t( 0.1f );
	params["color"] = parameter_t( colorA_t(1.0, 1.0, 0.9) );
	params["power"] = parameter_t( 0.5f );
	params["infinite"] = parameter_t( bool(false) );
	myLight = env->createLight("myDirectional", params);
	scene->addLight(myLight);
*/	
	// create a sunlight:
	params.clear();
	params["type"] = parameter_t(std::string("sunlight"));
	params["direction"] = parameter_t( point3d_t(-0.5, 0.6, 0.3) );
	params["angle"] = parameter_t( 0.35f );
	params["color"] = parameter_t( colorA_t(1.0, 1.0, 0.9) );
	params["power"] = parameter_t( 0.4f );
	params["samples"] = parameter_t( 2 );

	myLight = env->createLight("mySunlight", params);
	scene->addLight(myLight);
	
	//create a texutre:
	std::string texName("kitchen");
	params.clear();
	params["type"] = parameter_t(std::string("HDRtex"));
	params["filename"] = parameter_t(std::string("/media/hda6/Programme/YafRay_wip/Kitchen_LL2.hdr"));
	params["exposure_adjust"] = parameter_t(-0.4f);
	myTex = env->createTexture(texName, params);
	
	//create background:
	params.clear();
	params["type"] = parameter_t(std::string("textureback"));
	params["texture"] = parameter_t(texName);
	params["ibl"] = parameter_t(bool(false));
	params["ibl_samples"] = parameter_t(16);
	background_t *myBack = env->createBackground("myKitchen", params);
	scene->setBackground(myBack);

//	params.clear();
//	params["type"] = parameter_t(std::string("sunsky"));
//	params["from"] = parameter_t( point3d_t(1.0, 1.0, 1.0) );
//	background_t *myBack = env->createBackground("mySunsky", params);
//	scene->setBackground(myBack);
	
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
