#include <yafray_config.h>
#include <cstdlib>
#include <cctype>
#include <algorithm>
#include <signal.h>

#ifdef WIN32
	#include <windows.h>
#endif

#include <boost/filesystem.hpp>

#include <core_api/scene.h>
#include <core_api/environment.h>
#include <core_api/integrator.h>
#include <core_api/imagefilm.h>
#include <yafraycore/xmlparser.h>
#include <utilities/console_utils.h>
#include <yafraycore/imageOutput.h>

using namespace::yafaray;

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

int main(int argc, char *argv[])
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

	session.setPathYafaRayXml(boost::filesystem::system_complete(argv[0]).parent_path().string());

	cliParser_t parse(argc, argv, 2, 1, "You need to set at least a yafaray's valid XML file.");

	parse.setAppName("YafaRay XML loader",
	"[OPTIONS]... <input xml file> [output filename]\n<input xml file> : A valid yafaray XML file\n[output filename] : The filename of the rendered image without extension.\n*Note: If output filename is ommited the name \"yafaray\" will be used instead.");
	
	parse.setOption("pp","plugin-path", false, "Path to load plugins.");
	parse.setOption("vl","verbosity-level", false, "Set console verbosity level, options are:\n                                       \"mute\" (Prints nothing)\n                                       \"error\" (Prints only errors)\n                                       \"warning\" (Prints also warnings)\n                                       \"params\" (Prints also render param messages)\n                                       \"info\" (Prints also basi info messages)\n                                       \"verbose\" (Prints additional info messages)\n                                       \"debug\" (Prints debug messages if any)\n");
	parse.setOption("lvl","log-verbosity-level", false, "Set log/HTML files verbosity level, options are:\n                                       \"mute\" (Prints nothing)\n                                       \"error\" (Prints only errors)\n                                       \"warning\" (Prints also warnings)\n                                       \"params\" (Prints also render param messages)\n                                       \"info\" (Prints also basic info messages)\n                                       \"verbose\" (Prints additional info messages)\n                                       \"debug\" (Prints debug messages if any)\n");
	parse.parseCommandLine();
	
	renderEnvironment_t *env = new renderEnvironment_t();
	
	// Plugin load
	std::string ppath = parse.getOptionString("pp");
	std::string verbLevel = parse.getOptionString("vl");
	std::string logVerbLevel = parse.getOptionString("lvl");
	
	if(verbLevel.empty()) yafLog.setConsoleMasterVerbosity("info");
	else yafLog.setConsoleMasterVerbosity(verbLevel);

	if(logVerbLevel.empty()) yafLog.setLogMasterVerbosity("verbose");
	else yafLog.setLogMasterVerbosity(logVerbLevel);


	if(ppath.empty()) env->getPluginPath(ppath);
	
	if (!ppath.empty())
	{
		Y_VERBOSE << "The plugin path is: " << ppath << yendl;
		env->loadPlugins(ppath);
	}
	else
	{
		Y_ERROR << "Getting plugin path from render environment failed!" << yendl;
		return 1;
	}
	
	std::vector<std::string> formats = env->listImageHandlers();
	
	std::string formatString = "";
	for(size_t i = 0; i < formats.size(); i++)
	{
		formatString.append("                                       " + formats[i]);
		if(i < formats.size() - 1) formatString.append("\n");
	}

	parse.setOption("v","version", true, "Displays this program's version.");
	parse.setOption("h","help", true, "Displays this help text.");
	parse.setOption("op","output-path", false, "Uses the path in <value> as rendered image output path.");
	parse.setOption("ics","input-color-space", false, "Sets color space for input color values.\n                                       This does not affect textures, as they have individual color\n                                       space parameters in the XML file.\n                                       Available options:\n\n                                       LinearRGB (default)\n                                       sRGB\n                                       XYZ (experimental)\n");
	parse.setOption("f","format", false, "Sets the output image format, available formats are:\n\n" + formatString + "\n                                       Default: tga.\n");
    parse.setOption("ml","multilayer", true, "Enables multi-layer image output (only in certain formats as EXR)");
	parse.setOption("t","threads", false, "Overrides threads setting on the XML file, for auto selection use -1.");
	parse.setOption("a","with-alpha", true, "Enables saving the image with alpha channel.");
	parse.setOption("pbp","params_badge_position", false, "Sets position of the params badge: \"none\", \"top\" or \"bottom\".");
	parse.setOption("l","log-file-output", false, "Enable log file output(s): \"none\", \"txt\", \"html\" or \"txt+html\". Log file name will be same as selected image name,");
	parse.setOption("z","z-buffer", true, "Enables the rendering of the depth map (Z-Buffer) (this flag overrides XML setting).");
	parse.setOption("nz","no-z-buffer", true, "Disables the rendering of the depth map (Z-Buffer) (this flag overrides XML setting).");
	
	bool parseOk = parse.parseCommandLine();
	
	if(parse.getFlag("h"))
	{
		parse.printUsage();
		return 0;
	}
	
	if(parse.getFlag("v"))
	{
		Y_INFO << "YafaRay XML loader" << yendl << "Built with YafaRay Core version " << session.getYafaRayCoreVersion() << yendl;
		return 0;
	}
	
	if(!parseOk)
	{
		parse.printError();
		parse.printUsage();
		return 0;
	}
	
	bool alpha = parse.getFlag("a");
	std::string format = parse.getOptionString("f");
    bool multilayer = parse.getFlag("ml");
    
	std::string outputPath = parse.getOptionString("op");
	std::string input_color_space_string = parse.getOptionString("ics");	
	if(input_color_space_string.empty()) input_color_space_string = "LinearRGB";
	float input_gamma = 1.f;	//TODO: there is no parse.getOptionFloat available for now, so no way to have the additional option of entering an arbitrary manual input gamma yet. Maybe in the future...
	int threads = parse.getOptionInteger("t");
	bool zbuf = parse.getFlag("z");
	bool nozbuf = parse.getFlag("nz");
    
	if(format.empty()) format = "tga";
	bool formatValid = false;
	
	for(size_t i = 0; i < formats.size(); i++)
	{
		if(formats[i].find(format) != std::string::npos) formatValid = true;
	}
	
	if(!formatValid)
	{
		Y_ERROR << "Couldn't find any valid image format, image handlers missing?" << yendl;
		return 1;
	}
	
	const std::vector<std::string> files = parse.getCleanArgs();
	
	if(files.size() == 0)
	{
		return 0;
	}
	
	std::string outName = "yafray." + format;
	
	if(files.size() > 1) outName = files[1] + "." + format;
	
	std::string xmlFile = files[0];
	
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
	
	scene_t *scene = new scene_t(env);
	
	globalScene = scene;	//for the CTRL+C handler
	
	env->setScene(scene);
	paraMap_t render;
	
	bool success = parse_xml_file(xmlFile.c_str(), scene, env, render, input_color_space_string, input_gamma);
	if(!success) exit(1);
	
	int width=320, height=240;
	int bx = 0, by = 0;
	render.getParam("width", width); // width of rendered image
	render.getParam("height", height); // height of rendered image
	render.getParam("xstart", bx); // border render x start
	render.getParam("ystart", by); // border render y start
	
	if(threads >= -1) render["threads"] = threads;

	std::string logFileTypes = parse.getOptionString("l");
	if(logFileTypes == "none")
	{
		render["logging_saveLog"] = false;
		render["logging_saveHTML"] = false;
	}
	if(logFileTypes == "txt")
	{
		render["logging_saveLog"] = true;
		render["logging_saveHTML"] = false;
	}
	if(logFileTypes == "html")
	{
		render["logging_saveLog"] = false;
		render["logging_saveHTML"] = true;
	}
	if(logFileTypes == "txt+html")
	{
		render["logging_saveLog"] = true;
		render["logging_saveHTML"] = true;
	}

	std::string params_badge_position = parse.getOptionString("pbp");
	if(!params_badge_position.empty())
	{
		render["logging_paramsBadgePosition"] = params_badge_position;
		yafLog.setParamsBadgePosition(params_badge_position);
	}
	
	if(zbuf) render["z_channel"] = true;
	if(nozbuf) render["z_channel"] = false;
	
	bool use_zbuf = false;
	render.getParam("z_channel", use_zbuf);
	
	// create output
	colorOutput_t *out = nullptr;

	paraMap_t ihParams;
	ihParams["type"] = format;
	ihParams["width"] = width;
	ihParams["height"] = height;
	ihParams["alpha_channel"] = alpha;
	ihParams["z_channel"] = use_zbuf;
	ihParams["img_multilayer"] = multilayer;
    
	imageHandler_t *ih = env->createImageHandler("outFile", ihParams);

	if(ih)
	{
		out = new imageOutput_t(ih, outputPath, 0, 0);
		if(!out) return 1;				
	}
	else return 1;
	
	if(! env->setupScene(*scene, render, *out) ) return 1;
    imageFilm_t *film = scene->getImageFilm();
    session.setInteractive(false);
	session.setStatusRenderStarted();
	scene->render();
	
	env->clearAll();

	delete film;
	delete out;
	
	return 0;
}
