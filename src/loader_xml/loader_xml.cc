/****************************************************************************
 *      This is part of the libYafaRay package
 *
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public
 *      License as published by the Free Software Foundation; either
 *      version 2.1 of the License, or (at your option) any later version.
 *
 *      This library is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      Lesser General Public License for more details.
 *
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library; if not, write to the Free Software
 *      Foundation,Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "constants.h"
#include "yafaray_config.h"
#include "common/param.h"
#include "common/session.h"
#include "common/file.h"
#include "common/scene.h"
#include "common/imagefilm.h"
#include "common/import_xml.h"
#include "utility/util_console.h"
#include "output/output_image.h"
#include <signal.h>

#ifdef WIN32
#include <windows.h>
#endif

using namespace::yafaray4;

Scene *global_scene__ = nullptr;

#ifdef WIN32
BOOL WINAPI ctrlCHandler__(DWORD signal)
{
	if(global_scene__)
	{
		global_scene__->abort();
		session__.setStatusRenderAborted();
		Y_WARNING << "Interface: Render aborted by user." << YENDL;
	}
	else
	{
		session__.setStatusRenderAborted();
		Y_WARNING << "Interface: Render aborted by user." << YENDL;
		exit(1);
	}
	return TRUE;
}
#else
void ctrlCHandler__(int signal)
{
	if(global_scene__)
	{
		global_scene__->abort();
		session__.setStatusRenderAborted();
		Y_WARNING << "Interface: Render aborted by user." << YENDL;
	}
	else
	{
		session__.setStatusRenderAborted();
		Y_WARNING << "Interface: Render aborted by user." << YENDL;
		exit(1);
	}
}
#endif

int main(int argc, char *argv[])
{
	//handle CTRL+C events
#ifdef WIN32
	SetConsoleCtrlHandler(ctrlCHandler__, true);
#else
	struct ::sigaction signal_handler;
	signal_handler.sa_handler = ctrlCHandler__;
	sigemptyset(&signal_handler.sa_mask);
	signal_handler.sa_flags = 0;
	sigaction(SIGINT, &signal_handler, nullptr);
#endif

	//FIXME DAVID: get absolute path from relative? session.setPathYafaRayXml(bsst::filesystem::system_complete(argv[0]).parent_path().string());
	session__.setPathYafaRayXml(Path::getParent(argv[0]));

	CliParser parse(argc, argv, 2, 1, "You need to set at least a yafaray's valid XML file.");

	parse.setAppName("YafaRay XML loader",
	                 "[OPTIONS]... <input xml file> [output filename]\n<input xml file> : A valid yafaray XML file\n[output filename] : The filename of the rendered image without extension.\n*Note: If output filename is ommited the name \"yafaray\" will be used instead.");

	parse.setOption("pp", "plugin-path", false, "Path to load plugins.");
	parse.setOption("vl", "verbosity-level", false, "Set console verbosity level, options are:\n                                       \"mute\" (Prints nothing)\n                                       \"error\" (Prints only errors)\n                                       \"warning\" (Prints also warnings)\n                                       \"params\" (Prints also render param messages)\n                                       \"info\" (Prints also basi info messages)\n                                       \"verbose\" (Prints additional info messages)\n                                       \"debug\" (Prints debug messages if any)\n");
	parse.setOption("lvl", "log-verbosity-level", false, "Set log/HTML files verbosity level, options are:\n                                       \"mute\" (Prints nothing)\n                                       \"error\" (Prints only errors)\n                                       \"warning\" (Prints also warnings)\n                                       \"params\" (Prints also render param messages)\n                                       \"info\" (Prints also basic info messages)\n                                       \"verbose\" (Prints additional info messages)\n                                       \"debug\" (Prints debug messages if any)\n");
	parse.setOption("ccd", "console-colors-disabled", true, "If specified, disables the Console colors ANSI codes, useful for some 3rd party software that cannot handle ANSI codes well.");

	parse.parseCommandLine();

	bool console_colors_disabled = parse.getFlag("ccd");

	if(console_colors_disabled) logger__.setConsoleLogColorsEnabled(false);
	else logger__.setConsoleLogColorsEnabled(true);

	Scene *scene = new Scene();

	// Plugin load
	std::string ppath = parse.getOptionString("pp");
	std::string verb_level = parse.getOptionString("vl");
	std::string log_verb_level = parse.getOptionString("lvl");

	if(verb_level.empty()) logger__.setConsoleMasterVerbosity("info");
	else logger__.setConsoleMasterVerbosity(verb_level);

	if(log_verb_level.empty()) logger__.setLogMasterVerbosity("verbose");
	else logger__.setLogMasterVerbosity(log_verb_level);

/*	FIXME std::vector<std::string> formats = env->listImageHandlers();

	std::string format_string = "";
	for(size_t i = 0; i < formats.size(); i++)
	{
		format_string.append("                                       " + formats[i]);
		if(i < formats.size() - 1) format_string.append("\n");
	}
*/
	parse.setOption("v", "version", true, "Displays this program's version.");
	parse.setOption("h", "help", true, "Displays this help text.");
	parse.setOption("op", "output-path", false, "Uses the path in <value> as rendered image output path.");
	parse.setOption("ics", "input-color-space", false, "Sets color space for input color values.\n                                       This does not affect textures, as they have individual color\n                                       space parameters in the XML file.\n                                       Available options:\n\n                                       LinearRGB (default)\n                                       sRGB\n                                       XYZ (experimental)\n");
/* FIXME	parse.setOption("f", "format", false, "Sets the output image format, available formats are:\n\n" + format_string + "\n                                       Default: tga.\n"); */
	parse.setOption("ml", "multilayer", true, "Enables multi-layer image output (only in certain formats as EXR)");
	parse.setOption("t", "threads", false, "Overrides threads setting on the XML file, for auto selection use -1.");
	parse.setOption("a", "with-alpha", true, "Enables saving the image with alpha channel.");
	parse.setOption("pbp", "params_badge_position", false, "Sets position of the params badge: \"none\", \"top\" or \"bottom\".");
	parse.setOption("l", "log-file-output", false, "Enable log file output(s): \"none\", \"txt\", \"html\" or \"txt+html\". Log file name will be same as selected image name,");
	parse.setOption("z", "z-buffer", true, "Enables the rendering of the depth map (Z-Buffer) (this flag overrides XML setting).");
	parse.setOption("nz", "no-z-buffer", true, "Disables the rendering of the depth map (Z-Buffer) (this flag overrides XML setting).");

	bool parse_ok = parse.parseCommandLine();

	if(parse.getFlag("h"))
	{
		parse.printUsage();
		return 0;
	}

	if(parse.getFlag("v"))
	{
		Y_INFO << "YafaRay XML loader" << YENDL << "Built with YafaRay Core version " << YAFARAY_BUILD_VERSION << YENDL;
		return 0;
	}

	if(!parse_ok)
	{
		parse.printError();
		parse.printUsage();
		return 0;
	}

	bool alpha = parse.getFlag("a");
	std::string format = parse.getOptionString("f");
	bool multilayer = parse.getFlag("ml");

	std::string output_path = parse.getOptionString("op");
	std::string input_color_space_string = parse.getOptionString("ics");
	if(input_color_space_string.empty()) input_color_space_string = "LinearRGB";
	float input_gamma = 1.f;	//TODO: there is no parse.getOptionFloat available for now, so no way to have the additional option of entering an arbitrary manual input gamma yet. Maybe in the future...
	int threads = parse.getOptionInteger("t");
	bool zbuf = parse.getFlag("z");
	bool nozbuf = parse.getFlag("nz");

	if(format.empty()) format = "tga";
	bool format_valid = false;

/* FIXME	for(size_t i = 0; i < formats.size(); i++)
	{
		if(formats[i].find(format) != std::string::npos) format_valid = true;
	}

	if(!format_valid)
	{
		Y_ERROR << "Couldn't find any valid image format, image handlers missing?" << YENDL;
		return 1;
	}
*/
	const std::vector<std::string> files = parse.getCleanArgs();

	if(files.size() == 0)
	{
		return 0;
	}

	std::string out_name = "yafray." + format;

	if(files.size() > 1) out_name = files[1] + "." + format;

	std::string xml_file = files[0];

	// Set the full output path with filename
	if(output_path.empty())
	{
		output_path = out_name;
	}
	else if(output_path.at(output_path.length() - 1) == '/')
	{
		output_path += out_name;
	}
	else if(output_path.at(output_path.length() - 1) != '/')
	{
		output_path += "/" + out_name;
	}

	global_scene__ = scene;	//for the CTRL+C handler

	ParamMap params;

	bool success = parseXmlFile__(xml_file.c_str(), scene, params, input_color_space_string, input_gamma);
	if(!success) exit(1);

	int width = 320, height = 240;
	int bx = 0, by = 0;
	params.getParam("width", width); // width of rendered image
	params.getParam("height", height); // height of rendered image
	params.getParam("xstart", bx); // border render x start
	params.getParam("ystart", by); // border render y start

	//image output denoise options
	bool denoise_enabled = false;
	int denoise_h_col = 5, denoise_h_lum = 5;
	float denoise_mix = 0.8;
	params.getParam("denoiseEnabled", denoise_enabled);
	params.getParam("denoiseHCol", denoise_h_col);
	params.getParam("denoiseHLum", denoise_h_lum);
	params.getParam("denoiseMix", denoise_mix);

	if(threads >= -1) params["threads"] = threads;

	std::string log_file_types = parse.getOptionString("l");
	if(log_file_types == "none")
	{
		params["logging_saveLog"] = false;
		params["logging_saveHTML"] = false;
	}
	if(log_file_types == "txt")
	{
		params["logging_saveLog"] = true;
		params["logging_saveHTML"] = false;
	}
	if(log_file_types == "html")
	{
		params["logging_saveLog"] = false;
		params["logging_saveHTML"] = true;
	}
	if(log_file_types == "txt+html")
	{
		params["logging_saveLog"] = true;
		params["logging_saveHTML"] = true;
	}

	std::string params_badge_position = parse.getOptionString("pbp");
	if(!params_badge_position.empty())
	{
		params["logging_paramsBadgePosition"] = params_badge_position;
		logger__.setParamsBadgePosition(params_badge_position);
	}

	if(zbuf) params["z_channel"] = true;
	if(nozbuf) params["z_channel"] = false;

	bool use_zbuf = false;
	params.getParam("z_channel", use_zbuf);

	// create output
	ColorOutput *out = nullptr;

	ParamMap ih_params;
	ih_params["type"] = format;
	ih_params["width"] = width;
	ih_params["height"] = height;
	ih_params["alpha_channel"] = alpha;
	ih_params["z_channel"] = use_zbuf;
	ih_params["img_multilayer"] = multilayer;
	ih_params["denoiseEnabled"] = denoise_enabled;
	ih_params["denoiseHCol"] = denoise_h_col;
	ih_params["denoiseHLum"] = denoise_h_lum;
	ih_params["denoiseMix"] = denoise_mix;

	ImageHandler *ih = scene->createImageHandler("outFile", ih_params);

	if(ih)
	{
		out = new ImageOutput(ih, output_path, 0, 0);
		if(!out) return 1;
	}
	else return 1;

	if(! scene->setupScene(*scene, params, *out)) return 1;
	ImageFilm *film = scene->getImageFilm();
	session__.setInteractive(false);
	session__.setStatusRenderStarted();
	scene->render();

	scene->clearAll();

	delete film;
	delete out;

	return 0;
}
