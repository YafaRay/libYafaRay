/****************************************************************************
 *   This is part of the libYafaRay package
 *
 *   This library is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Lesser General Public
 *   License as published by the Free Software Foundation; either
 *   version 2.1 of the License, or (at your option) any later version.
 *
 *   This library is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *   Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation,Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "public_api/yafaray_c_api.h"
#include "public_api/yafaray_c_api_utils.h"
#include "common/version_build_info.h"

/*yafaray_Scene *yafaray_createInterface(yafaray_InterfaceType interface_type, const char *exported_file_path, const yafaray_LoggerCallback logger_callback, void *callback_data, yafaray_DisplayConsole display_console)
{
	yafaray::Interface *interface;
	if(interface_type == YAFARAY_INTERFACE_EXPORT_XML) interface = new yafaray::ExportXml(exported_file_path, logger_callback, callback_data, display_console);
	else if(interface_type == YAFARAY_INTERFACE_EXPORT_C) interface = new yafaray::ExportC(exported_file_path, logger_callback, callback_data, display_console);
	else if(interface_type == YAFARAY_INTERFACE_EXPORT_PYTHON) interface = new yafaray::ExportPython(exported_file_path, logger_callback, callback_data, display_console);
	else interface = new yafaray::Interface(logger_callback, callback_data, display_console);
	return reinterpret_cast<yafaray_Scene *>(interface);
}

void yafaray_destroyInterface(yafaray_Scene *scene)
{
	delete reinterpret_cast<yafaray::Scene *>(scene);
}*/

int yafaray_getVersionMajor() { return yafaray::buildinfo::getVersionMajor(); }
int yafaray_getVersionMinor() { return yafaray::buildinfo::getVersionMinor(); }
int yafaray_getVersionPatch() { return yafaray::buildinfo::getVersionPatch(); }

char *yafaray_getVersionString()
{
	return createCharString(yafaray::buildinfo::getVersionString());
}

void yafaray_destroyCharString(char *string)
{
	delete[] string;
}
