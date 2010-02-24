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
#endif

#include <core_api/scene.h>
#include <core_api/environment.h>
#include <core_api/integrator.h>
#include <core_api/imagefilm.h>
#include <yafraycore/tga_io.h>
#include <yafraycore/EXR_io.h>
#include <yafraycore/xmlparser.h>
#include <yaf_revision.h>
#include <utilities/console_utils.h>

using namespace::yafaray;

int main(int argc, char *argv[])
{
	cliParser_t parse(argc, argv, 2, 1, "You need to set at least a yafaray's valid XML file.");
	
#ifdef RELEASE
	std::string version = std::string(VERSION);
#else
	std::string version = std::string(YAF_SVN_REV);
#endif

	std::string xmlLoaderVersion = "YafaRay XML loader version 0.2";

	parse.setAppName(xmlLoaderVersion,
	"[OPTIONS]... <input xml file> [output filename]\n\
	<input xml file> : A valid yafaray XML file\n\
	[output filename] : The filename of the rendered image without extension.\n\
	                    If output filename is ommited the name \"yafaray\" will be used instead.");
	// Configuration of valid flags
	parse.setOption("v","version", true, "Displays this program's version.");
	parse.setOption("h","help", true, "Displays this help text.");
	parse.setOption("pp","plugin-path", false, "Uses the path in <value> to load the plugins.");
	parse.setOption("op","output-path", false, "Uses the path in <value> as rendered image output path.");
	parse.setOption("f","format", false, "Sets the output image format (tga and exr are the only options by now).\n\tIf no format is set or an unknown format is used it defaults to tga.");
	parse.setOption("t","threads", false, "Overrides threads setting on the XML file, for auto selection use -1.");
	parse.setOption("a","with-alpha", true, "Enables saving the image with alpha channel.");
	parse.setOption("dp","draw-params", true, "Enables saving the image with a settings badge.");
	parse.setOption("ndp","no-draw-params", true, "Disables saving the image with a settings badge (warning: this overrides --draw-params setting).");
	parse.setOption("cs","custom-string", false, "Sets the custom string to be used on the settings badge.");
	
	if(!parse.parseCommandLine())
	{
		parse.printUsage();
		return 0;
	}
	
	if(parse.getFlag("h"))
	{
		parse.printUsage();
		return 0;
	}
	else if(parse.getFlag("v"))
	{
		Y_INFO << xmlLoaderVersion << std::endl << "Built with YafaRay version " << version << std::endl;
		return 0;
	}
	
	bool alpha = parse.getFlag("a");
	std::string format = parse.getOptionString("f");
	std::string plugPath = parse.getOptionString("pp");
	std::string outputPath = parse.getOptionString("op");
	int threads = parse.getOptionInteger("t");
	bool drawparams = parse.getFlag("dp");
	bool nodrawparams = parse.getFlag("ndp");
	std::string customString = parse.getOptionString("cs");
	
	if(format.empty()) format = "tga";
	else if(format != "tga" && format != "exr") format = "tga"; // <<<< Fix this shit after making file formats proper plugins
	
	const std::vector<std::string> files = parse.getCleanArgs();
	
	if(files.size() == 0)
	{
		return 0;
	}
	
	std::string outName = "yafray." + format;
	
	if(files.size() > 1) outName = files[1] + "." + format;
	
	std::string xmlFile = files[0];
	std::string ppath = "";
	
	//env->Debug = debug; //disabled until proper debugging messages are set throughout the core

	// Set the full output path with filename
	if (outputPath.empty())
	{
		outputPath = outName;
	}
	else if (outputPath.at(outputPath.length() - 1) == '/')
	{
		outputPath += outName;
	}
	else if (outputPath.at(outputPath.length() - 1) != '/')
	{
		outputPath += "/" + outName;
	}
	
	renderEnvironment_t *env = new renderEnvironment_t();
	
	// Plugin load
	
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
	
	int width=320, height=240;
	render.getParam("width", width); // width of rendered image
	render.getParam("height", height); // height of rendered image
	
	if(threads >= -1) render["threads"] = threads;
	
	if(drawparams)
	{
		render["drawParams"] = true;
		if(!customString.empty()) render["customString"] = customString;
	}
	
	if(nodrawparams) render["drawParams"] = false;
	
	// create output
	colorOutput_t *out=0;
#if HAVE_EXR
	if(format=="exr") out = new outEXR_t(width, height, outputPath.c_str(), "");
	else out = new outTga_t(width, height, outputPath.c_str(), alpha);
#else
	out = new outTga_t(width, height, outputPath.c_str(), alpha);
#endif
	
	if(! env->setupScene(*scene, render, *out) ) return 1;
	
	scene->render();
	env->clearAll();

	imageFilm_t *film = scene->getImageFilm();

	delete film;
	delete out;
	
	return 0;
}
