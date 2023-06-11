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

#include "common/container.h"
#include "scene/scene.h"
#include "render/imagefilm.h"
#include "integrator/surface/integrator_surface.h"
#include "common/version_build_info.h"
#include "common/file.h"
#include <sstream>

namespace yafaray {

void Container::destroyContainedPointers()
{
	for(auto surface_integrator : surface_integrators_)
	{
		delete surface_integrator;
	}
	for(auto image_film : image_films_)
	{
		delete image_film;
	}
	for(auto scene : scenes_)
	{
		delete scene;
	}
}

Scene *Container::getScene(const std::string &name) const
{
	for(auto scene : scenes_)
	{
		if(scene->getName() == name) return scene;
	}
	return nullptr;
}

SurfaceIntegrator *Container::getSurfaceIntegrator(const std::string &name) const
{
	for(auto surface_integrator : surface_integrators_)
	{
		if(surface_integrator->getName() == name) return surface_integrator;
	}
	return nullptr;
}

ImageFilm *Container::getImageFilm(const std::string &name) const
{
	for(auto image_film : image_films_)
	{
		if(image_film->getName() == name) return image_film;
	}
	return nullptr;
}

std::string Container::exportToString(yafaray_ContainerExportType container_export_type, bool only_export_non_default_parameters) const
{
	std::stringstream ss;
	ss << createExportStartSection(container_export_type);
	for(auto scene : scenes_)
	{
		ss << scene->exportToString(1, container_export_type, only_export_non_default_parameters);
	}
	for(auto surface_integrator : surface_integrators_)
	{
		ss << surface_integrator->exportToString(1, container_export_type, only_export_non_default_parameters);
	}
	for(auto image_film : image_films_)
	{
		ss << image_film->exportToString(1, container_export_type, only_export_non_default_parameters);
	}
	ss << createExportEndSection(container_export_type);
	return ss.str();
}

yafaray_ResultFlags Container::exportToFile(yafaray_ContainerExportType container_export_type, bool only_export_non_default_parameters, const std::string &file_path) const
{
	File file{file_path};
	const bool file_open_result{file.open("w")};
	if(!file_open_result) return YAFARAY_RESULT_ERROR_WHILE_CREATING;

	file.appendText(createExportStartSection(container_export_type));
	for(auto scene : scenes_)
	{
		scene->exportToFile(file, 1, container_export_type, only_export_non_default_parameters);
	}
	for(auto surface_integrator : surface_integrators_)
	{
		file.appendText(surface_integrator->exportToString(1, container_export_type, only_export_non_default_parameters));
	}
	for(auto image_film : image_films_)
	{
		file.appendText(image_film->exportToString(1, container_export_type, only_export_non_default_parameters));
	}
	file.appendText(createExportEndSection(container_export_type));
	file.close();
	return YAFARAY_RESULT_OK;
}

std::string Container::createExportStartSection(yafaray_ContainerExportType container_export_type) const
{
	std::stringstream ss;
	if(container_export_type == YAFARAY_CONTAINER_EXPORT_C)
	{
		ss << "/* ANSI C89/C90 file generated by libYafaRay C Export */\n";
		ss << "/* To build use your favorite ANSI C compiler/linker, pointing to libYafaRay include/library files */\n";
		ss << "/* For example in Linux using GCC */\n";
		ss << "/* LD_LIBRARY_PATH=(path to folder with libyafaray libs) gcc -o libyafaray_example_executable -ansi -I(path to folder with libyafaray includes) -L(path to folder with libyafaray libs) (yafaray_scene_exported_source_file_name.c) -O0 -ggdb -lyafaray4 */\n";
		ss << "/* Note: no optimizations are needed for compiling this source file because it is libYafaRay itself which should be optimized for fastest execution. */\n";
		ss << "/*       Disabling compiler optimizations should help speeding up compilation of large scenes. */\n";
		ss << "/* To run the executable */\n";
		ss << "/* LD_LIBRARY_PATH=(path to folder with libyafaray libs) ./libyafaray_example_executable */\n\n";
		ss << "#include <yafaray_c_api.h>\n";
		ss << "void section_0(yafaray_Interface_t *yi)\n{\n";
	}
	else if(container_export_type == YAFARAY_CONTAINER_EXPORT_PYTHON)
	{
		ss << "# Python3 file generated by libYafaRay Python Export\n";
		ss << "# To run this file, execute the following line with Python v3.3 or higher (use \"python\" or \"python3\" depending on the operating system):\n";
		ss << "# python3 (yafaray_python_file_name.py)\n";
		ss << "# Alternatively if using a portable python or the \"libyafaray4_bindings\" library is not in the standard python library folder and the above does not work, try the following instead:\n";
		ss << "# LD_LIBRARY_PATH=(path to folder with python and libyafaray libs) PYTHONPATH=(path to folder with python and libyafaray libs) python3 (yafaray_python_file_name.py)\n\n";
		ss << "import libyafaray4_bindings\n\n";
		ss << "yi = libyafaray4_bindings.Interface()\n";
		ss << "yi.setConsoleVerbosityLevel(yi.logLevelFromString(\"debug\"))\n";
		ss << "yi.setLogVerbosityLevel(yi.logLevelFromString(\"debug\"))\n";
		ss << "yi.setConsoleLogColorsEnabled(True)\n";
		ss << "yi.enablePrintDateTime(True)\n\n";
	}
	else if(container_export_type == YAFARAY_CONTAINER_EXPORT_XML)
	{
		ss << std::boolalpha;
		ss << "<?xml version=\"1.0\"?>\n";
		ss << "<yafaray_container format_version=\"" << buildinfo::getVersionMajor() << "." << buildinfo::getVersionMinor() << "." << buildinfo::getVersionPatch() << "\">\n";
	}
	return ss.str();
}

std::string Container::createExportEndSection(yafaray_ContainerExportType container_export_type) const
{
	std::stringstream ss;
	if(container_export_type == YAFARAY_CONTAINER_EXPORT_C)
	{
		ss << "int main()\n";
		ss << "{\n";
		ss << "\t" << "yafaray_Interface_t *yi = yafaray_createInterface(YAFARAY_INTERFACE_FOR_RENDERING, NULL, NULL, NULL, YAFARAY_DISPLAY_CONSOLE_NORMAL);\n";
		ss << "\t" << "yafaray_setConsoleLogColorsEnabled(yi, true);\n";
		ss << "\t" << "yafaray_setConsoleVerbosityLevel(yi, YAFARAY_LOG_LEVEL_VERBOSE);\n\n";
		//FIXME ss << generateSectionsCalls();
		ss << "\n\t" << "yafaray_render(yi, NULL, NULL, YAFARAY_DISPLAY_CONSOLE_NORMAL);\n";
		ss << "\n\t" << "yafaray_destroyInterface(yi);\n\n";
		ss << "\t" << "return 0;\n";
		ss << "}\n";
	}
	else if(container_export_type == YAFARAY_CONTAINER_EXPORT_PYTHON)
	{
		ss << "del yi\n";
	}
	else if(container_export_type == YAFARAY_CONTAINER_EXPORT_XML)
	{
		ss << "</yafaray_container>\n";
	}
	return ss.str();
}

} //namespace yafaray