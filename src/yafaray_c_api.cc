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

#include "yafaray_c_api.h"
#include "interface/interface.h"
#include "interface/export/export_xml.h"
#include "geometry/matrix4.h"
#include "render/progress_bar.h"
#include "image/image.h"
#include <cstring>

yafaray4_Interface_t *yafaray4_createInterface(yafaray4_Interface_Type_t interface_type, const char *exported_file_path, const yafaray4_LoggerCallback_t logger_callback, void *callback_user_data, yafaray4_DisplayConsole_t display_console)
{
	yafaray4::Interface *interface;
	if(interface_type == YAFARAY_INTERFACE_EXPORT_XML) interface = new yafaray4::XmlExport(exported_file_path);
	else interface = new yafaray4::Interface(logger_callback, callback_user_data, display_console);
	return reinterpret_cast<yafaray4_Interface *>(interface);
}

void yafaray4_destroyInterface(yafaray4_Interface_t *interface)
{
	delete reinterpret_cast<yafaray4::Interface *>(interface);
}

void yafaray4_createScene(yafaray4_Interface_t *interface)
{
	reinterpret_cast<yafaray4::Interface *>(interface)->createScene();
}

yafaray4_bool_t yafaray4_startGeometry(yafaray4_Interface_t *interface) //!< call before creating geometry; only meshes and vmaps can be created in this state
{
	return static_cast<yafaray4_bool_t>(reinterpret_cast<yafaray4::Interface *>(interface)->startGeometry());
}
yafaray4_bool_t yafaray4_endGeometry(yafaray4_Interface_t *interface) //!< call after creating geometry;
{
	return static_cast<yafaray4_bool_t>(reinterpret_cast<yafaray4::Interface *>(interface)->endGeometry());
}

unsigned int yafaray4_getNextFreeId(yafaray4_Interface_t *interface)
{
	return reinterpret_cast<yafaray4::Interface *>(interface)->getNextFreeId();
}

yafaray4_bool_t yafaray4_endObject(yafaray4_Interface_t *interface) //!< end current mesh and return to geometry state
{
	return static_cast<yafaray4_bool_t>(reinterpret_cast<yafaray4::Interface *>(interface)->endObject());
}

int  yafaray4_addVertex(yafaray4_Interface_t *interface, double x, double y, double z) //!< add vertex to mesh; returns index to be used for addTriangle
{
	return reinterpret_cast<yafaray4::Interface *>(interface)->addVertex(x, y, z);
}

int  yafaray4_addVertexWithOrco(yafaray4_Interface_t *interface, double x, double y, double z, double ox, double oy, double oz) //!< add vertex with Orco to mesh; returns index to be used for addTriangle
{
	return reinterpret_cast<yafaray4::Interface *>(interface)->addVertex(x, y, z, ox, oy, oz);
}

void yafaray4_addNormal(yafaray4_Interface_t *interface, double nx, double ny, double nz) //!< add vertex normal to mesh; the vertex that will be attached to is the last one inserted by addVertex method
{
	reinterpret_cast<yafaray4::Interface *>(interface)->addNormal(nx, ny, nz);
}

yafaray4_bool_t yafaray4_addTriangle(yafaray4_Interface_t *interface, int a, int b, int c) //!< add a triangle given vertex indices and material pointer
{
	return static_cast<yafaray4_bool_t>(reinterpret_cast<yafaray4::Interface *>(interface)->addFace(a, b, c));
}

yafaray4_bool_t yafaray4_addFaceWithUv(yafaray4_Interface_t *interface, int a, int b, int c, int uv_a, int uv_b, int uv_c) //!< add a triangle given vertex and uv indices and material pointer
{
	return static_cast<yafaray4_bool_t>(reinterpret_cast<yafaray4::Interface *>(interface)->addFace(a, b, c, uv_a, uv_b, uv_c));
}

int  yafaray4_addUv(yafaray4_Interface_t *interface, float u, float v) //!< add a UV coordinate pair; returns index to be used for addTriangle
{
	return reinterpret_cast<yafaray4::Interface *>(interface)->addUv(u, v);
}

yafaray4_bool_t yafaray4_smoothMesh(yafaray4_Interface_t *interface, const char *name, double angle) //!< smooth vertex normals of mesh with given ID and angle (in degrees)
{
	return static_cast<yafaray4_bool_t>(reinterpret_cast<yafaray4::Interface *>(interface)->smoothMesh(name, angle));
}

yafaray4_bool_t yafaray4_addInstance(yafaray4_Interface_t *interface, const char *base_object_name, const float obj_to_world[4][4])
// functions to build paramMaps instead of passing them from Blender
// (decouling implementation details of STL containers, paraMap_t etc. as much as possible)
{
	return static_cast<yafaray4_bool_t>(reinterpret_cast<yafaray4::Interface *>(interface)->addInstance(base_object_name, obj_to_world));
}

void yafaray4_paramsSetVector(yafaray4_Interface_t *interface, const char *name, double x, double y, double z)
{
	reinterpret_cast<yafaray4::Interface *>(interface)->paramsSetVector(name, x, y, z);
}

void yafaray4_paramsSetString(yafaray4_Interface_t *interface, const char *name, const char *s)
{
	reinterpret_cast<yafaray4::Interface *>(interface)->paramsSetString(name, s);
}

void yafaray4_paramsSetBool(yafaray4_Interface_t *interface, const char *name, yafaray4_bool_t b)
{
	reinterpret_cast<yafaray4::Interface *>(interface)->paramsSetBool(name, b);
}

void yafaray4_paramsSetInt(yafaray4_Interface_t *interface, const char *name, int i)
{
	reinterpret_cast<yafaray4::Interface *>(interface)->paramsSetInt(name, i);
}

void yafaray4_paramsSetFloat(yafaray4_Interface_t *interface, const char *name, double f)
{
	reinterpret_cast<yafaray4::Interface *>(interface)->paramsSetFloat(name, f);
}

void yafaray4_paramsSetColor(yafaray4_Interface_t *interface, const char *name, float r, float g, float b, float a)
{
	reinterpret_cast<yafaray4::Interface *>(interface)->paramsSetColor(name, r, g, b, a);
}

void yafaray4_paramsSetMatrix(yafaray4_Interface_t *interface, const char *name, const float m[4][4], yafaray4_bool_t transpose)
{
	reinterpret_cast<yafaray4::Interface *>(interface)->paramsSetMatrix(name, m, transpose);
}

void yafaray4_paramsClearAll(yafaray4_Interface_t *interface) 	//!< clear the paramMap and paramList
{
	reinterpret_cast<yafaray4::Interface *>(interface)->paramsClearAll();
}

void yafaray4_paramsPushList(yafaray4_Interface_t *interface) 	//!< push new list item in paramList (e.g. new shader node description)
{
	reinterpret_cast<yafaray4::Interface *>(interface)->paramsPushList();
}

void yafaray4_paramsEndList(yafaray4_Interface_t *interface) 	//!< revert to writing to normal paramMap
{
	reinterpret_cast<yafaray4::Interface *>(interface)->paramsEndList();
}

void yafaray4_setCurrentMaterial(yafaray4_Interface_t *interface, const char *name)
{
	reinterpret_cast<yafaray4::Interface *>(interface)->setCurrentMaterial(name);
}

yafaray4_bool_t yafaray4_createObject(yafaray4_Interface_t *interface, const char *name)
{
	return static_cast<yafaray4_bool_t>(reinterpret_cast<yafaray4::Interface *>(interface)->createObject(name) != nullptr);
}

yafaray4_bool_t yafaray4_createLight(yafaray4_Interface_t *interface, const char *name)
{
	return static_cast<yafaray4_bool_t>(reinterpret_cast<yafaray4::Interface *>(interface)->createLight(name) != nullptr);
}

yafaray4_bool_t yafaray4_createTexture(yafaray4_Interface_t *interface, const char *name)
{
	return static_cast<yafaray4_bool_t>(reinterpret_cast<yafaray4::Interface *>(interface)->createTexture(name) != nullptr);
}

yafaray4_bool_t yafaray4_createMaterial(yafaray4_Interface_t *interface, const char *name)
{
	return static_cast<yafaray4_bool_t>(reinterpret_cast<yafaray4::Interface *>(interface)->createMaterial(name) != nullptr);
}

yafaray4_bool_t yafaray4_createCamera(yafaray4_Interface_t *interface, const char *name)
{
	return static_cast<yafaray4_bool_t>(reinterpret_cast<yafaray4::Interface *>(interface)->createCamera(name) != nullptr);
}

yafaray4_bool_t yafaray4_createBackground(yafaray4_Interface_t *interface, const char *name)
{
	return static_cast<yafaray4_bool_t>(reinterpret_cast<yafaray4::Interface *>(interface)->createBackground(name) != nullptr);
}

yafaray4_bool_t yafaray4_createIntegrator(yafaray4_Interface_t *interface, const char *name)
{
	return static_cast<yafaray4_bool_t>(reinterpret_cast<yafaray4::Interface *>(interface)->createIntegrator(name) != nullptr);
}

yafaray4_bool_t yafaray4_createVolumeRegion(yafaray4_Interface_t *interface, const char *name)
{
	return static_cast<yafaray4_bool_t>(reinterpret_cast<yafaray4::Interface *>(interface)->createVolumeRegion(name) != nullptr);
}

yafaray4_bool_t yafaray4_createRenderView(yafaray4_Interface_t *interface, const char *name)
{
	return static_cast<yafaray4_bool_t>(reinterpret_cast<yafaray4::Interface *>(interface)->createRenderView(name) != nullptr);
}

yafaray4_bool_t yafaray4_createOutput(yafaray4_Interface_t *interface, const char *name, yafaray4_bool_t auto_delete, yafaray4_OutputPutpixelCallback_t output_putpixel_callback, yafaray4_OutputFlushAreaCallback_t output_flush_area_callback, yafaray4_OutputFlushCallback_t output_flush_callback, void *callback_user_data) //!< ColorOutput creation, usually for internally-owned outputs that are destroyed when the scene is deleted or when libYafaRay instance is closed. If the client wants to keep ownership, it can set the "auto_delete" to false.
{
	return static_cast<yafaray4_bool_t>(reinterpret_cast<yafaray4::Interface *>(interface)->createOutput(name, auto_delete, callback_user_data, output_putpixel_callback, output_flush_area_callback, output_flush_callback) != nullptr);
}

yafaray4_bool_t yafaray4_removeOutput(yafaray4_Interface_t *interface, const char *name)
{
	return static_cast<yafaray4_bool_t>(reinterpret_cast<yafaray4::Interface *>(interface)->removeOutput(name));
}

void yafaray4_clearOutputs(yafaray4_Interface_t *interface)
{
	reinterpret_cast<yafaray4::Interface *>(interface)->clearOutputs();
}

void yafaray4_clearAll(yafaray4_Interface_t *interface)
{
	reinterpret_cast<yafaray4::Interface *>(interface)->clearAll();
}

void yafaray4_render(yafaray4_Interface_t *interface, yafaray4_ProgressBarCallback_t monitor_callback, void *callback_user_data, yafaray4_DisplayConsole_t progress_bar_display_console) //!< render the scene...
{
	yafaray4::ProgressBar *progress_bar;
	if(progress_bar_display_console == YAFARAY_DISPLAY_CONSOLE_NORMAL) progress_bar = new yafaray4::ConsoleProgressBar(80, monitor_callback, callback_user_data);
	else progress_bar = new yafaray4::ProgressBar(monitor_callback, callback_user_data);
	reinterpret_cast<yafaray4::Interface *>(interface)->render(progress_bar, true, progress_bar_display_console);
}

void yafaray4_defineLayer(yafaray4_Interface_t *interface, const char *layer_type_name, const char *exported_image_type_name, const char *exported_image_name, const char *image_type_name)
{
	reinterpret_cast<yafaray4::Interface *>(interface)->defineLayer(layer_type_name, exported_image_type_name, exported_image_name, image_type_name);
}

yafaray4_bool_t yafaray4_setupLayersParameters(yafaray4_Interface_t *interface)
{
	return static_cast<yafaray4_bool_t>(reinterpret_cast<yafaray4::Interface *>(interface)->setupLayersParameters());
}

void yafaray4_cancel(yafaray4_Interface_t *interface)
{
	reinterpret_cast<yafaray4::Interface *>(interface)->cancel();
}

yafaray4_bool_t yafaray4_setInteractive(yafaray4_Interface_t *interface, yafaray4_bool_t interactive)
{
	return static_cast<yafaray4_bool_t>(reinterpret_cast<yafaray4::Interface *>(interface)->setInteractive(interactive));
}

void yafaray4_enablePrintDateTime(yafaray4_Interface_t *interface, yafaray4_bool_t value)
{
	reinterpret_cast<yafaray4::Interface *>(interface)->enablePrintDateTime(value);
}

void yafaray4_setConsoleVerbosityLevel(yafaray4_Interface_t *interface, yafaray4_LogLevel_t log_level)
{
	reinterpret_cast<yafaray4::Interface *>(interface)->setConsoleVerbosityLevel(log_level);
}

void yafaray4_setLogVerbosityLevel(yafaray4_Interface_t *interface, yafaray4_LogLevel_t log_level)
{
	reinterpret_cast<yafaray4::Interface *>(interface)->setLogVerbosityLevel(log_level);
}

void yafaray4_getVersion(yafaray4_Interface_t *interface, char *dest_string, size_t dest_string_size) //!< Get version to check against the exporters
{
	if(dest_string) strncpy(dest_string, reinterpret_cast<yafaray4::Interface *>(interface)->getVersion().c_str(), dest_string_size);
}

/*! Console Printing wrappers to report in color with yafaray's own console coloring */
void yafaray4_printDebug(yafaray4_Interface_t *interface, const char *msg)
{
	reinterpret_cast<yafaray4::Interface *>(interface)->printDebug(msg);
}

void yafaray4_printVerbose(yafaray4_Interface_t *interface, const char *msg)
{
	reinterpret_cast<yafaray4::Interface *>(interface)->printVerbose(msg);
}

void yafaray4_printInfo(yafaray4_Interface_t *interface, const char *msg)
{
	reinterpret_cast<yafaray4::Interface *>(interface)->printInfo(msg);
}

void yafaray4_printParams(yafaray4_Interface_t *interface, const char *msg)
{
	reinterpret_cast<yafaray4::Interface *>(interface)->printParams(msg);
}

void yafaray4_printWarning(yafaray4_Interface_t *interface, const char *msg)
{
	reinterpret_cast<yafaray4::Interface *>(interface)->printWarning(msg);
}

void yafaray4_printError(yafaray4_Interface_t *interface, const char *msg)
{
	reinterpret_cast<yafaray4::Interface *>(interface)->printError(msg);
}

void yafaray4_setInputColorSpace(yafaray4_Interface_t *interface, const char *color_space_string, float gamma_val)
{
	reinterpret_cast<yafaray4::Interface *>(interface)->setInputColorSpace(color_space_string, gamma_val);
}

yafaray4_Image_t *yafaray4_createImage(yafaray4_Interface_t *interface, const char *name)
{
	return reinterpret_cast<yafaray4_Image_t *>(reinterpret_cast<yafaray4::Interface *>(interface)->createImage(name));
}

yafaray4_bool_t yafaray4_setImageColor(yafaray4_Image_t *image, int x, int y, float red, float green, float blue, float alpha)
{
	yafaray4::Image *yaf_image = reinterpret_cast<yafaray4::Image *>(image);
	if(x < 0 || x >= yaf_image->getWidth()) return YAFARAY_BOOL_FALSE;
	else if(y < 0 || y >= yaf_image->getHeight()) return YAFARAY_BOOL_FALSE;
	yaf_image->setColor(x, y, {red, green, blue, alpha});
	return YAFARAY_BOOL_TRUE;
}

yafaray4_bool_t yafaray4_getImageColor(const yafaray4_Image_t *image, int x, int y, float *red, float *green, float *blue, float *alpha)
{
	const yafaray4::Image *yaf_image = reinterpret_cast<const yafaray4::Image *>(image);
	if(x < 0 || x >= yaf_image->getWidth()) return YAFARAY_BOOL_FALSE;
	else if(y < 0 || y >= yaf_image->getHeight()) return YAFARAY_BOOL_FALSE;
	const yafaray4::Rgba color = yaf_image->getColor(x, y);
	*red = color.r_;
	*green = color.g_;
	*blue = color.b_;
	*alpha = color.a_;
	return YAFARAY_BOOL_TRUE;
}

void yafaray4_cancelRendering(yafaray4_Interface_t *interface)
{
	reinterpret_cast<yafaray4::Interface *>(interface)->cancel();
}

void yafaray4_setConsoleLogColorsEnabled(yafaray4_Interface_t *interface, yafaray4_bool_t colors_enabled)
{
	reinterpret_cast<yafaray4::Interface *>(interface)->setConsoleLogColorsEnabled(colors_enabled);
}
