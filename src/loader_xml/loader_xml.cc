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

#include "yafaray_config.h"
#include "common/param.h"
#include "common/session.h"
#include "scene/scene.h"
#include "render/imagefilm.h"
#include "import/import_xml.h"
#include "common/console.h"
#include "output/output_image.h"
#include <signal.h>

#ifdef WIN32
#include <windows.h>
#endif

using namespace::yafaray4;

RenderControl *global_render_control_global = nullptr;

#ifdef WIN32
BOOL WINAPI ctrlCHandler_global(DWORD signal)
{
	Y_WARNING << "Interface: Render aborted by user." << YENDL;
	if(global_render_control_global)
	{
		global_render_control_global->setAborted();
		return TRUE;
	}
	else exit(1);
}
#else
void ctrlCHandler_global(int signal)
{
	Y_WARNING << "Interface: Render aborted by user." << YENDL;
	if(global_render_control_global) global_render_control_global->setAborted();
	else exit(1);
}
#endif

int main(int argc, char *argv[])
{
	//handle CTRL+C events
#ifdef WIN32
	SetConsoleCtrlHandler(ctrlCHandler_global, true);
#else
	struct ::sigaction signal_handler;
	signal_handler.sa_handler = ctrlCHandler_global;
	sigemptyset(&signal_handler.sa_mask);
	signal_handler.sa_flags = 0;
	sigaction(SIGINT, &signal_handler, nullptr);
#endif

	CliParser parse(argc, argv, 2, 1, "You need to set at least a yafaray's valid XML file.");

	parse.setAppName("YafaRay XML loader",
					 "[OPTIONS]... <input xml file>\n<input xml file> : A valid yafaray XML file\n*Note: the output file name(s) and parameters are defined in the XML file, in the <output> tags.");

	parse.setOption("vl", "verbosity-level", false, "Set console verbosity level, options are:\n                                       \"mute\" (Prints nothing)\n                                       \"error\" (Prints only errors)\n                                       \"warning\" (Prints also warnings)\n                                       \"params\" (Prints also render param messages)\n                                       \"info\" (Prints also basi info messages)\n                                       \"verbose\" (Prints additional info messages)\n                                       \"debug\" (Prints debug messages if any)\n");
	parse.setOption("lvl", "log-verbosity-level", false, "Set log/HTML files verbosity level, options are the same as for the \"verbosity-level\" parameter\n");
	parse.setOption("nodt", "no-date-time", true, "If specified, disables the logging of the date/time in the screen and file logs");
	parse.setOption("ccd", "console-colors-disabled", true, "If specified, disables the Console colors ANSI codes, useful for some 3rd party software that cannot handle ANSI codes well.");

	parse.parseCommandLine();

	bool no_date_time = parse.getFlag("nodt");
	bool console_colors_disabled = parse.getFlag("ccd");

	if(no_date_time) logger_global.enablePrintDateTime(false);
	if(console_colors_disabled) logger_global.setConsoleLogColorsEnabled(false);
	else logger_global.setConsoleLogColorsEnabled(true);

	std::string verb_level = parse.getOptionString("vl");
	std::string log_verb_level = parse.getOptionString("lvl");

	if(verb_level.empty()) logger_global.setConsoleMasterVerbosity("info");
	else logger_global.setConsoleMasterVerbosity(verb_level);

	if(log_verb_level.empty()) logger_global.setLogMasterVerbosity("verbose");
	else logger_global.setLogMasterVerbosity(log_verb_level);

	parse.setOption("v", "version", true, "Displays this program's version.");
	parse.setOption("h", "help", true, "Displays this help text.");
	parse.setOption("ics", "input-color-space", false, "Sets color space for input color values.\n                                       This does not affect textures, as they have individual color space parameters in the XML file.\n                                       Available options:\n                                       LinearRGB (default)\n                                       sRGB\n                                       XYZ (experimental)\n");
	parse.setOption("t", "threads", false, "Overrides threads setting on the XML file, for auto selection use -1.");
	parse.setOption("pbp", "params_badge_position", false, "Sets position of the params badge: \"none\", \"top\" or \"bottom\".");
	parse.setOption("l", "log-file-output", false, "Enable log file output(s): \"none\", \"txt\", \"html\" or \"txt+html\". Log file name will be same as selected image name,");

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

	std::string input_color_space_string = parse.getOptionString("ics");
	if(input_color_space_string.empty()) input_color_space_string = "LinearRGB";
	float input_gamma = 1.f;	//TODO: there is no parse.getOptionFloat available for now, so no way to have the additional option of entering an arbitrary manual input gamma yet. Maybe in the future...
	int threads = parse.getOptionInteger("t");
	const std::vector<std::string> files = parse.getCleanArgs();

	std::string output_file_path = "yafaray.tga";
	if(files.size() == 0) return 0;

	std::string xml_file_path = files.at(0);
	if(files.size() > 1) output_file_path = files.at(1);

	ParamMap params;
	Scene *scene =parseXmlFile_global(xml_file_path.c_str(), params, input_color_space_string, input_gamma);
	if(!scene) exit(1);
	global_render_control_global = &scene->getRenderControl();	//for the CTRL+C handler

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
	params.getParam("denoise_enabled", denoise_enabled);
	params.getParam("denoise_h_col", denoise_h_col);
	params.getParam("denoise_h_lum", denoise_h_lum);
	params.getParam("denoise_mix", denoise_mix);

	if(threads >= -1) params["threads"] = threads;

	std::string log_file_types = parse.getOptionString("l");
	if(log_file_types == "none")
	{
		params["logging_save_txt"] = false;
		params["logging_save_html"] = false;
	}
	if(log_file_types == "txt")
	{
		params["logging_save_txt"] = true;
		params["logging_save_html"] = false;
	}
	if(log_file_types == "html")
	{
		params["logging_save_txt"] = false;
		params["logging_save_html"] = true;
	}
	if(log_file_types == "txt+html")
	{
		params["logging_save_txt"] = true;
		params["logging_save_html"] = true;
	}

	std::string params_badge_position = parse.getOptionString("pbp");
	if(!params_badge_position.empty()) params["badge_position"] = params_badge_position;

	if(! scene->setupScene(*scene, params)) return 1;
	session_global.setInteractive(false);
	scene->render();
	auto outputs = scene->getOutputs();
	for(auto &output : outputs) delete output.second;
	scene->clearAll();
	delete scene;
	return 0;
}
