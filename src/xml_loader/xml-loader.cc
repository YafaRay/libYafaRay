#include <yafray_config.h>
#include <cstdlib>
#include <cctype>
#include <algorithm>

#ifdef WIN32
	#include <windows.h>
#endif

#include <core_api/scene.h>
#include <core_api/environment.h>
#include <core_api/integrator.h>
#include <core_api/imagefilm.h>
#include <yafraycore/xmlparser.h>
#include <yaf_revision.h>
#include <utilities/console_utils.h>
#include <yafraycore/imageOutput.h>

using namespace::yafaray;

int main(int argc, char *argv[])
{
	std::string xmlLoaderVersion = "YafaRay XML loader version: " + std::string(VERSION);

	cliParser_t parse(argc, argv, 2, 1, "You need to set at least a yafaray's valid XML file.");

	parse.setAppName(xmlLoaderVersion,
	"[OPTIONS]... <input xml file> [output filename]\n<input xml file> : A valid yafaray XML file\n[output filename] : The filename of the rendered image without extension.\n*Note: If output filename is ommited the name \"yafaray\" will be used instead.");
	
	parse.setOption("pp","plugin-path", false, "Path to load plugins.");
	parse.setOption("vl","verbosity-level", false, "Set verbosity level, options are:\n                                       0 - MUTE (Prints nothing)\n                                       1 - ERROR (Prints only errors)\n                                       2 - WARNING (Prints only errors and warnings)\n                                       3 - INFO (Prints all messages)\n");
	parse.parseCommandLine();
	
#ifdef RELEASE
	std::string version = std::string(VERSION);
#else
	std::string version = std::string(YAF_SVN_REV);
#endif

	renderEnvironment_t *env = new renderEnvironment_t();
	
	// Plugin load
	std::string ppath = parse.getOptionString("pp");
	int verbLevel = parse.getOptionInteger("vl");
	
	if(verbLevel >= 0) yafLog.setMasterVerbosity(verbLevel);
	
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
	parse.setOption("dp","draw-params", true, "Enables saving the image with a settings badge.");
	parse.setOption("ndp","no-draw-params", true, "Disables saving the image with a settings badge (warning: this overrides --draw-params setting).");
	parse.setOption("cs","custom-string", false, "Sets the custom string to be used on the settings badge.");
	parse.setOption("z","z-buffer", true, "Enables the rendering of the depth map (Z-Buffer) (this flag overrides XML setting).");
	parse.setOption("nz","no-z-buffer", true, "Disables the rendering of the depth map (Z-Buffer) (this flag overrides XML setting).");
    parse.setOption("pst","partial-save-timer", false, "Sets timer in seconds for partial saving of images during render. If set to 0 (default) it will disable this feature. IMPORTANT: the more frequently partial images are saved, the slower the render will be.");
	
	bool parseOk = parse.parseCommandLine();
	
	if(parse.getFlag("h"))
	{
		parse.printUsage();
		return 0;
	}
	
	if(parse.getFlag("v"))
	{
		Y_INFO << xmlLoaderVersion << yendl << "Built with YafaRay version " << version << yendl;
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
	bool drawparams = parse.getFlag("dp");
	bool nodrawparams = parse.getFlag("ndp");
	std::string customString = parse.getOptionString("cs");
	bool zbuf = parse.getFlag("z");
	bool nozbuf = parse.getFlag("nz");
	int partial_save_timer = parse.getOptionInteger("pst");
    
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
	
	if(drawparams)
	{
		render["drawParams"] = true;
		if(!customString.empty()) render["customString"] = customString;
	}
	
	if(nodrawparams) render["drawParams"] = false;
	
	if(zbuf) render["z_channel"] = true;
	if(nozbuf) render["z_channel"] = false;
	
	bool use_zbuf = false;
	render.getParam("z_channel", use_zbuf);
	
	// create output
	colorOutput_t *out = NULL;

	bool imageFileDrawParams = false;
	render.getParam("drawParams", imageFileDrawParams);

	paraMap_t ihParams;
	ihParams["type"] = format;
	ihParams["width"] = width;
	ihParams["height"] = height;
	ihParams["alpha_channel"] = alpha;
	ihParams["z_channel"] = use_zbuf;
	ihParams["img_multilayer"] = multilayer;
	ihParams["img_draw_params"] = imageFileDrawParams;
    
	imageHandler_t *ih = env->createImageHandler("outFile", ihParams);

	if(ih)
	{
		out = new imageOutput_t(ih, outputPath, 0, 0);
		if(!out) return 1;				
	}
	else return 1;
	
	if(! env->setupScene(*scene, render, *out) ) return 1;
    
    imageFilm_t *film = scene->getImageFilm();
    film->setInteractive(false);
    film->setImageOutputPartialSaveTimeInterval((double) partial_save_timer);
	

	scene->render();
	env->clearAll();

	delete film;
	delete out;
	
	return 0;
}
