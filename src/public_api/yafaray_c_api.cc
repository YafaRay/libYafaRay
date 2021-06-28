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

#include "public_api/yafaray_c_api.h"
#include "interface/interface.h"
#include "interface/export/export_xml.h"
#include "geometry/matrix4.h"
#include "render/progress_bar.h"
#include "image/image.h"
#include "common/logger.h"
#include "color/color.h"
#include "common/version_build_info.h"
#include <cstring>

yafaray_Interface_t *yafaray_createInterface(yafaray_Interface_Type_t interface_type, const char *exported_file_path, const yafaray_LoggerCallback_t logger_callback, void *callback_user_data, yafaray_DisplayConsole_t display_console)
{
	yafaray::Interface *interface;
	if(interface_type == YAFARAY_INTERFACE_EXPORT_XML) interface = new yafaray::XmlExport(exported_file_path, logger_callback, callback_user_data, display_console);
	else interface = new yafaray::Interface(logger_callback, callback_user_data, display_console);
	return reinterpret_cast<yafaray_Interface *>(interface);
}

void yafaray_destroyInterface(yafaray_Interface_t *interface)
{
	delete reinterpret_cast<yafaray::Interface *>(interface);
}

void yafaray_setLoggingCallback(yafaray_Interface_t *interface, const yafaray_LoggerCallback_t logger_callback, void *callback_user_data)
{
	reinterpret_cast<yafaray::Interface *>(interface)->setLoggingCallback(logger_callback, callback_user_data);
}

void yafaray_createScene(yafaray_Interface_t *interface)
{
	reinterpret_cast<yafaray::Interface *>(interface)->createScene();
}

yafaray_bool_t yafaray_startGeometry(yafaray_Interface_t *interface) //!< call before creating geometry; only meshes and vmaps can be created in this state
{
	return static_cast<yafaray_bool_t>(reinterpret_cast<yafaray::Interface *>(interface)->startGeometry());
}
yafaray_bool_t yafaray_endGeometry(yafaray_Interface_t *interface) //!< call after creating geometry;
{
	return static_cast<yafaray_bool_t>(reinterpret_cast<yafaray::Interface *>(interface)->endGeometry());
}

unsigned int yafaray_getNextFreeId(yafaray_Interface_t *interface)
{
	return reinterpret_cast<yafaray::Interface *>(interface)->getNextFreeId();
}

yafaray_bool_t yafaray_endObject(yafaray_Interface_t *interface) //!< end current mesh and return to geometry state
{
	return static_cast<yafaray_bool_t>(reinterpret_cast<yafaray::Interface *>(interface)->endObject());
}

int  yafaray_addVertex(yafaray_Interface_t *interface, double x, double y, double z) //!< add vertex to mesh; returns index to be used for addTriangle
{
	return reinterpret_cast<yafaray::Interface *>(interface)->addVertex(x, y, z);
}

int  yafaray_addVertexWithOrco(yafaray_Interface_t *interface, double x, double y, double z, double ox, double oy, double oz) //!< add vertex with Orco to mesh; returns index to be used for addTriangle
{
	return reinterpret_cast<yafaray::Interface *>(interface)->addVertex(x, y, z, ox, oy, oz);
}

void yafaray_addNormal(yafaray_Interface_t *interface, double nx, double ny, double nz) //!< add vertex normal to mesh; the vertex that will be attached to is the last one inserted by addVertex method
{
	reinterpret_cast<yafaray::Interface *>(interface)->addNormal(nx, ny, nz);
}

yafaray_bool_t yafaray_addTriangle(yafaray_Interface_t *interface, int a, int b, int c) //!< add a triangle given vertex indices and material pointer
{
	return static_cast<yafaray_bool_t>(reinterpret_cast<yafaray::Interface *>(interface)->addFace(a, b, c));
}

yafaray_bool_t yafaray_addTriangleWithUv(yafaray_Interface_t *interface, int a, int b, int c, int uv_a, int uv_b, int uv_c) //!< add a triangle given vertex and uv indices and material pointer
{
	return static_cast<yafaray_bool_t>(reinterpret_cast<yafaray::Interface *>(interface)->addFace(a, b, c, uv_a, uv_b, uv_c));
}

int  yafaray_addUv(yafaray_Interface_t *interface, float u, float v) //!< add a UV coordinate pair; returns index to be used for addTriangle
{
	return reinterpret_cast<yafaray::Interface *>(interface)->addUv(u, v);
}

yafaray_bool_t yafaray_smoothMesh(yafaray_Interface_t *interface, const char *name, double angle) //!< smooth vertex normals of mesh with given ID and angle (in degrees)
{
	return static_cast<yafaray_bool_t>(reinterpret_cast<yafaray::Interface *>(interface)->smoothMesh(name, angle));
}

yafaray_bool_t yafaray_addInstance(yafaray_Interface_t *interface, const char *base_object_name, const float obj_to_world[4][4])
// functions to build paramMaps instead of passing them from Blender
// (decouling implementation details of STL containers, paraMap_t etc. as much as possible)
{
	return static_cast<yafaray_bool_t>(reinterpret_cast<yafaray::Interface *>(interface)->addInstance(base_object_name, obj_to_world));
}

void yafaray_paramsSetVector(yafaray_Interface_t *interface, const char *name, double x, double y, double z)
{
	reinterpret_cast<yafaray::Interface *>(interface)->paramsSetVector(name, x, y, z);
}

void yafaray_paramsSetString(yafaray_Interface_t *interface, const char *name, const char *s)
{
	reinterpret_cast<yafaray::Interface *>(interface)->paramsSetString(name, s);
}

void yafaray_paramsSetBool(yafaray_Interface_t *interface, const char *name, yafaray_bool_t b)
{
	reinterpret_cast<yafaray::Interface *>(interface)->paramsSetBool(name, b);
}

void yafaray_paramsSetInt(yafaray_Interface_t *interface, const char *name, int i)
{
	reinterpret_cast<yafaray::Interface *>(interface)->paramsSetInt(name, i);
}

void yafaray_paramsSetFloat(yafaray_Interface_t *interface, const char *name, double f)
{
	reinterpret_cast<yafaray::Interface *>(interface)->paramsSetFloat(name, f);
}

void yafaray_paramsSetColor(yafaray_Interface_t *interface, const char *name, float r, float g, float b, float a)
{
	reinterpret_cast<yafaray::Interface *>(interface)->paramsSetColor(name, r, g, b, a);
}

void yafaray_paramsSetMatrix(yafaray_Interface_t *interface, const char *name, const float *m, yafaray_bool_t transpose)
{
	reinterpret_cast<yafaray::Interface *>(interface)->paramsSetMemMatrix(name, m, transpose);
}

void yafaray_paramsClearAll(yafaray_Interface_t *interface) 	//!< clear the paramMap and paramList
{
	reinterpret_cast<yafaray::Interface *>(interface)->paramsClearAll();
}

void yafaray_paramsPushList(yafaray_Interface_t *interface) 	//!< push new list item in paramList (e.g. new shader node description)
{
	reinterpret_cast<yafaray::Interface *>(interface)->paramsPushList();
}

void yafaray_paramsEndList(yafaray_Interface_t *interface) 	//!< revert to writing to normal paramMap
{
	reinterpret_cast<yafaray::Interface *>(interface)->paramsEndList();
}

void yafaray_setCurrentMaterial(yafaray_Interface_t *interface, const char *name)
{
	reinterpret_cast<yafaray::Interface *>(interface)->setCurrentMaterial(name);
}

yafaray_bool_t yafaray_createObject(yafaray_Interface_t *interface, const char *name)
{
	return static_cast<yafaray_bool_t>(reinterpret_cast<yafaray::Interface *>(interface)->createObject(name) != nullptr);
}

yafaray_bool_t yafaray_createLight(yafaray_Interface_t *interface, const char *name)
{
	return static_cast<yafaray_bool_t>(reinterpret_cast<yafaray::Interface *>(interface)->createLight(name) != nullptr);
}

yafaray_bool_t yafaray_createTexture(yafaray_Interface_t *interface, const char *name)
{
	return static_cast<yafaray_bool_t>(reinterpret_cast<yafaray::Interface *>(interface)->createTexture(name) != nullptr);
}

yafaray_bool_t yafaray_createMaterial(yafaray_Interface_t *interface, const char *name)
{
	return static_cast<yafaray_bool_t>(reinterpret_cast<yafaray::Interface *>(interface)->createMaterial(name) != nullptr);
}

yafaray_bool_t yafaray_createCamera(yafaray_Interface_t *interface, const char *name)
{
	return static_cast<yafaray_bool_t>(reinterpret_cast<yafaray::Interface *>(interface)->createCamera(name) != nullptr);
}

yafaray_bool_t yafaray_createBackground(yafaray_Interface_t *interface, const char *name)
{
	return static_cast<yafaray_bool_t>(reinterpret_cast<yafaray::Interface *>(interface)->createBackground(name) != nullptr);
}

yafaray_bool_t yafaray_createIntegrator(yafaray_Interface_t *interface, const char *name)
{
	return static_cast<yafaray_bool_t>(reinterpret_cast<yafaray::Interface *>(interface)->createIntegrator(name) != nullptr);
}

yafaray_bool_t yafaray_createVolumeRegion(yafaray_Interface_t *interface, const char *name)
{
	return static_cast<yafaray_bool_t>(reinterpret_cast<yafaray::Interface *>(interface)->createVolumeRegion(name) != nullptr);
}

yafaray_bool_t yafaray_createRenderView(yafaray_Interface_t *interface, const char *name)
{
	return static_cast<yafaray_bool_t>(reinterpret_cast<yafaray::Interface *>(interface)->createRenderView(name) != nullptr);
}

yafaray_bool_t yafaray_createOutput(yafaray_Interface_t *interface, const char *name, yafaray_bool_t auto_delete) //!< ColorOutput creation, usually for internally-owned outputs that are destroyed when the scene is deleted or when libYafaRay instance is closed. If the client wants to keep ownership, it can set the "auto_delete" to false.
{
	return static_cast<yafaray_bool_t>(reinterpret_cast<yafaray::Interface *>(interface)->createOutput(name, auto_delete) != nullptr);
}
void yafaray_setOutputPutPixelCallback(yafaray_Interface_t *interface, const char *output_name, yafaray_OutputPutpixelCallback_t putpixel_callback, void *putpixel_callback_user_data)
{
	reinterpret_cast<yafaray::Interface *>(interface)->setOutputPutPixelCallback(output_name, putpixel_callback, putpixel_callback_user_data);
}

void yafaray_setOutputFlushAreaCallback(yafaray_Interface_t *interface, const char *output_name, yafaray_OutputFlushAreaCallback_t flush_area_callback, void *flush_area_callback_user_data)
{
	reinterpret_cast<yafaray::Interface *>(interface)->setOutputFlushAreaCallback(output_name, flush_area_callback, flush_area_callback_user_data);
}

void yafaray_setOutputFlushCallback(yafaray_Interface_t *interface, const char *output_name, yafaray_OutputFlushCallback_t flush_callback, void *flush_callback_user_data)
{
	reinterpret_cast<yafaray::Interface *>(interface)->setOutputFlushCallback(output_name, flush_callback, flush_callback_user_data);
}

void yafaray_setOutputHighlightCallback(yafaray_Interface_t *interface, const char *output_name, yafaray_OutputHighlightCallback_t highlight_callback, void *highlight_callback_user_data)
{
	reinterpret_cast<yafaray::Interface *>(interface)->setOutputHighlightCallback(output_name, highlight_callback, highlight_callback_user_data);
}

yafaray_bool_t yafaray_removeOutput(yafaray_Interface_t *interface, const char *name)
{
	return static_cast<yafaray_bool_t>(reinterpret_cast<yafaray::Interface *>(interface)->removeOutput(name));
}

void yafaray_clearOutputs(yafaray_Interface_t *interface)
{
	reinterpret_cast<yafaray::Interface *>(interface)->clearOutputs();
}

void yafaray_clearAll(yafaray_Interface_t *interface)
{
	reinterpret_cast<yafaray::Interface *>(interface)->clearAll();
}

void yafaray_setupRender(yafaray_Interface_t *interface)
{
	reinterpret_cast<yafaray::Interface *>(interface)->setupRender();
}

void yafaray_render(yafaray_Interface_t *interface, yafaray_ProgressBarCallback_t monitor_callback, void *callback_user_data, yafaray_DisplayConsole_t progress_bar_display_console) //!< render the scene...
{
	yafaray::ProgressBar *progress_bar;
	if(progress_bar_display_console == YAFARAY_DISPLAY_CONSOLE_NORMAL) progress_bar = new yafaray::ConsoleProgressBar(80, monitor_callback, callback_user_data);
	else progress_bar = new yafaray::ProgressBar(monitor_callback, callback_user_data);
	reinterpret_cast<yafaray::Interface *>(interface)->render(progress_bar, true, progress_bar_display_console);
}

void yafaray_defineLayer(yafaray_Interface_t *interface)
{
	reinterpret_cast<yafaray::Interface *>(interface)->defineLayer();
}

yafaray_bool_t yafaray_setupLayersParameters(yafaray_Interface_t *interface)
{
	return static_cast<yafaray_bool_t>(reinterpret_cast<yafaray::Interface *>(interface)->setupLayersParameters());
}

void yafaray_cancel(yafaray_Interface_t *interface)
{
	reinterpret_cast<yafaray::Interface *>(interface)->cancel();
}

yafaray_bool_t yafaray_setInteractive(yafaray_Interface_t *interface, yafaray_bool_t interactive)
{
	return static_cast<yafaray_bool_t>(reinterpret_cast<yafaray::Interface *>(interface)->setInteractive(interactive));
}

void yafaray_enablePrintDateTime(yafaray_Interface_t *interface, yafaray_bool_t value)
{
	reinterpret_cast<yafaray::Interface *>(interface)->enablePrintDateTime(value);
}

void yafaray_setConsoleVerbosityLevel(yafaray_Interface_t *interface, yafaray_LogLevel_t log_level)
{
	reinterpret_cast<yafaray::Interface *>(interface)->setConsoleVerbosityLevel(log_level);
}

void yafaray_setLogVerbosityLevel(yafaray_Interface_t *interface, yafaray_LogLevel_t log_level)
{
	reinterpret_cast<yafaray::Interface *>(interface)->setLogVerbosityLevel(log_level);
}

void yafaray_getVersionString(char *dest_string, unsigned int dest_string_size)
{
	if(!dest_string || dest_string_size == 0) return;
	const std::string version_string = yafaray::buildinfo::getVersionString();
	const unsigned int copy_length = std::min(dest_string_size - 1, static_cast<unsigned int>(version_string.size()));
	strncpy(dest_string, version_string.c_str(), copy_length);
	*(dest_string + copy_length) = 0x00; //Make sure that the destination string gets null terminated
}

int yafaray_getVersionMajor() { return yafaray::buildinfo::getVersionMajor(); }
int yafaray_getVersionMinor() { return yafaray::buildinfo::getVersionMinor(); }
int yafaray_getVersionPatch() { return yafaray::buildinfo::getVersionPatch(); }

/*! Console Printing wrappers to report in color with yafaray's own console coloring */
void yafaray_printDebug(yafaray_Interface_t *interface, const char *msg)
{
	reinterpret_cast<yafaray::Interface *>(interface)->printDebug(msg);
}

void yafaray_printVerbose(yafaray_Interface_t *interface, const char *msg)
{
	reinterpret_cast<yafaray::Interface *>(interface)->printVerbose(msg);
}

void yafaray_printInfo(yafaray_Interface_t *interface, const char *msg)
{
	reinterpret_cast<yafaray::Interface *>(interface)->printInfo(msg);
}

void yafaray_printParams(yafaray_Interface_t *interface, const char *msg)
{
	reinterpret_cast<yafaray::Interface *>(interface)->printParams(msg);
}

void yafaray_printWarning(yafaray_Interface_t *interface, const char *msg)
{
	reinterpret_cast<yafaray::Interface *>(interface)->printWarning(msg);
}

void yafaray_printError(yafaray_Interface_t *interface, const char *msg)
{
	reinterpret_cast<yafaray::Interface *>(interface)->printError(msg);
}

void yafaray_setInputColorSpace(yafaray_Interface_t *interface, const char *color_space_string, float gamma_val)
{
	reinterpret_cast<yafaray::Interface *>(interface)->setInputColorSpace(color_space_string, gamma_val);
}

yafaray_Image_t *yafaray_createImage(yafaray_Interface_t *interface, const char *name)
{
	return reinterpret_cast<yafaray_Image_t *>(reinterpret_cast<yafaray::Interface *>(interface)->createImage(name));
}

yafaray_bool_t yafaray_setImageColor(yafaray_Image_t *image, int x, int y, float red, float green, float blue, float alpha)
{
	yafaray::Image *yaf_image = reinterpret_cast<yafaray::Image *>(image);
	if(!yaf_image) return YAFARAY_BOOL_FALSE;
	if(x < 0 || x >= yaf_image->getWidth()) return YAFARAY_BOOL_FALSE;
	else if(y < 0 || y >= yaf_image->getHeight()) return YAFARAY_BOOL_FALSE;
	yaf_image->setColor(x, y, {red, green, blue, alpha});
	return YAFARAY_BOOL_TRUE;
}

yafaray_bool_t yafaray_getImageColor(const yafaray_Image_t *image, int x, int y, float *red, float *green, float *blue, float *alpha)
{
	const yafaray::Image *yaf_image = reinterpret_cast<const yafaray::Image *>(image);
	if(!yaf_image) return YAFARAY_BOOL_FALSE;
	if(x < 0 || x >= yaf_image->getWidth()) return YAFARAY_BOOL_FALSE;
	else if(y < 0 || y >= yaf_image->getHeight()) return YAFARAY_BOOL_FALSE;
	const yafaray::Rgba color = yaf_image->getColor(x, y);
	*red = color.r_;
	*green = color.g_;
	*blue = color.b_;
	*alpha = color.a_;
	return YAFARAY_BOOL_TRUE;
}

void yafaray_cancelRendering(yafaray_Interface_t *interface)
{
	reinterpret_cast<yafaray::Interface *>(interface)->cancel();
}

void yafaray_setConsoleLogColorsEnabled(yafaray_Interface_t *interface, yafaray_bool_t colors_enabled)
{
	reinterpret_cast<yafaray::Interface *>(interface)->setConsoleLogColorsEnabled(colors_enabled);
}

yafaray_LogLevel_t yafaray_logLevelFromString(const char *log_level_string)
{
	return static_cast<yafaray_LogLevel_t>(yafaray::Logger::vlevelFromString(log_level_string));
}