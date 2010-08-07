#include <yafray_config.h>
#include <cstdlib>
#include <iostream>
#include <cctype>
#include <algorithm>

#ifdef WIN32
	#ifndef __MINGW32__
		#define NOMINMAX
	#endif
	#include <windows.h>
	#include <utilities/winunistd.h>
#else
	#include<unistd.h>
#endif

#include <core_api/scene.h>
#include <core_api/environment.h>
#include <core_api/integrator.h>
#include <core_api/imagefilm.h>
#include <yafraycore/tga_io.h>
#include <yafraycore/EXR_io.h>
#include <yafraycore/xmlparser.h>
#include <yaf_revision.h>

using namespace::yafaray;

void printInstructions()
{
	std::cout << "USAGE: yafaray-xml [OPTION]... FILE\n"
			  << "OPTIONS:\n"
			  << "\t-h: display this help\n"
			  << "\t-f FORMAT: 'exr' outputs EXR format instead of TGA\n"
			  << "\t-p PATH: use alternative path for loading plugins\n"
			  << "\t-o PATH: path for the output file\n"
			  << "\t-d LEVEL: set debug verbosity level\n"
			  << "\t-v: display the version\n";
}

int main(int argc, char *argv[])
{
	std::cout << "Starting YafaRay XML loader..." << std::endl;
	std::string format;
	std::string plugPath;
	std::string outputPath;
#ifdef RELEASE
	std::string version = std::string(VERSION);
#else
	std::string version = std::string(YAF_SVN_REV);
#endif
	int c=0;
	int debug=0;
	while(c != -1)
	{
		c=getopt(argc,argv,"vhf:c:p:s:o:d:");
		switch(c)
		{
			case -1: break;
			case 'v': std::cout << "XML loader, version 0.1\nBuilt with YafaRay version " << version << std::endl;
					return 0;
			case 'h': printInstructions(); return 0;
			case 'p': plugPath = optarg; break;
			case 'o': outputPath = optarg; break;
			case 'f': format = optarg;
					  for(unsigned int i=0; i<format.size(); ++i) format[i] = std::tolower(format[i]); break;
			case 'd': debug = (int) atol(optarg); break;
			default: Y_WARNING << "Option ignored! For a list of options, use '-h' option" << std::endl;
		}
	}
	std::string ppath, xmlFile;
	
	if(optind < argc) xmlFile = argv[optind];
	else
	{
		Y_ERROR << "No input file given." << std::endl;
		printInstructions();
		return 0;
	}
	renderEnvironment_t *env = new renderEnvironment_t();

	env->Debug = debug;

	if (outputPath.empty())
	{
		outputPath = std::string("yafaray.tga");
	}
	else if (outputPath.at(outputPath.length() - 1) == '/')
	{
		outputPath += std::string("yafaray.tga");
	}
	else if (outputPath.at(outputPath.length() - 1) != '/')
	{
		outputPath += std::string("/yafaray.tga");
	}
	
	if(plugPath.empty())
	{
		if (env->getPluginPath(ppath))
		{
			Y_DEBUG(1) << "The plugin path is: " << ppath << std::endl;
			env->loadPlugins(ppath);
		}
		else Y_ERROR << "Getting plugin path from render environment failed!" << std::endl;
	}
	else
	{
		Y_DEBUG(1) << "The plugin path is: " << plugPath << std::endl;
		env->loadPlugins(plugPath);
	}
	scene_t *scene = new scene_t();
	env->setScene(scene);
	paraMap_t render;
	
	bool success = parse_xml_file(xmlFile.c_str(), scene, env, render);
	if(!success) exit(1);
	
	// render scene:
	/* paraMap_t *params = &render;
	const std::string *name=0; */
	int width=320, height=240;
	render.getParam("width", width); // width of rendered image
	render.getParam("height", height); // height of rendered image
	
	// create output
	colorOutput_t *out=0;
#if HAVE_EXR
	if(format=="exr") out = new outEXR_t(width, height, "yafaray.exr", "");
	else out = new outTga_t(width, height, outputPath.c_str(), false);
#else
	out = new outTga_t(width, height, outputPath.c_str(), false);
#endif
	
	std::cout << "setting up scene..." << std::flush;
	if(! env->setupScene(*scene, render, *out) ) return 1;
	std::cout << "done!" << std::endl;
	
	scene->render();
	env->clearAll();
	Y_DEBUG(1) << "Free imageFilm..." << std::endl;
	imageFilm_t *film = scene->getImageFilm();
	delete film;
	delete out;
	
	return 0;
}
