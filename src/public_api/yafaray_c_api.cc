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
#include "color/color.h"
#include "common/logger.h"
#include "common/version_build_info.h"
#include "geometry/matrix.h"
#include "image/image.h"
#include "interface/export/export_c.h"
#include "interface/export/export_python.h"
#include "interface/export/export_xml.h"
#include "interface/interface.h"
#include "render/progress_bar.h"
#include <cstring>

yafaray_Interface_t *yafaray_createInterface(yafaray_Interface_Type_t interface_type, const char *exported_file_path, const yafaray_LoggerCallback_t logger_callback, void *callback_data, yafaray_DisplayConsole_t display_console)
{
	yafaray::Interface *interface;
	if(interface_type == YAFARAY_INTERFACE_EXPORT_XML) interface = new yafaray::ExportXml(exported_file_path, logger_callback, callback_data, display_console);
	else if(interface_type == YAFARAY_INTERFACE_EXPORT_C) interface = new yafaray::ExportC(exported_file_path, logger_callback, callback_data, display_console);
	else if(interface_type == YAFARAY_INTERFACE_EXPORT_PYTHON) interface = new yafaray::ExportPython(exported_file_path, logger_callback, callback_data, display_console);
	else interface = new yafaray::Interface(logger_callback, callback_data, display_console);
	return reinterpret_cast<yafaray_Interface *>(interface);
}

void yafaray_destroyInterface(yafaray_Interface_t *interface)
{
	delete reinterpret_cast<yafaray::Interface *>(interface);
}

void yafaray_setLoggingCallback(yafaray_Interface_t *interface, const yafaray_LoggerCallback_t logger_callback, void *callback_data)
{
	reinterpret_cast<yafaray::Interface *>(interface)->setLoggingCallback(logger_callback, callback_data);
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

int  yafaray_addVertex(yafaray_Interface_t *interface, float x, float y, float z) //!< add vertex to mesh; returns index to be used for addTriangle/addQuad
{
	return reinterpret_cast<yafaray::Interface *>(interface)->addVertex({{x, y, z}}, 0);
}

int  yafaray_addVertexTimeStep(yafaray_Interface_t *interface, float x, float y, float z, int time_step) //!< add vertex to mesh; returns index to be used for addTriangle/addQuad
{
	return reinterpret_cast<yafaray::Interface *>(interface)->addVertex({{x, y, z}}, time_step);
}

int  yafaray_addVertexWithOrco(yafaray_Interface_t *interface, float x, float y, float z, float ox, float oy, float oz) //!< add vertex with Orco to mesh; returns index to be used for addTriangle/addQuad
{
	return reinterpret_cast<yafaray::Interface *>(interface)->addVertex({{x, y, z}}, {{ox, oy, oz}}, 0);
}

int  yafaray_addVertexWithOrcoTimeStep(yafaray_Interface_t *interface, float x, float y, float z, float ox, float oy, float oz, int time_step) //!< add vertex with Orco to mesh; returns index to be used for addTriangle/addQuad
{
	return reinterpret_cast<yafaray::Interface *>(interface)->addVertex({{x, y, z}}, {{ox, oy, oz}}, time_step);
}

void yafaray_addNormal(yafaray_Interface_t *interface, float nx, float ny, float nz) //!< add vertex normal to mesh; the vertex that will be attached to is the last one inserted by addVertex method
{
	reinterpret_cast<yafaray::Interface *>(interface)->addVertexNormal({{nx, ny, nz}}, 0);
}

void yafaray_addNormalTimeStep(yafaray_Interface_t *interface, float nx, float ny, float nz, int time_step) //!< add vertex normal to mesh; the vertex that will be attached to is the last one inserted by addVertex method
{
	reinterpret_cast<yafaray::Interface *>(interface)->addVertexNormal({{nx, ny, nz}}, time_step);
}

yafaray_bool_t yafaray_addTriangle(yafaray_Interface_t *interface, int a, int b, int c)
{
	return static_cast<yafaray_bool_t>(reinterpret_cast<yafaray::Interface *>(interface)->addFace({a, b, c}, {}));
}

yafaray_bool_t yafaray_addTriangleWithUv(yafaray_Interface_t *interface, int a, int b, int c, int uv_a, int uv_b, int uv_c)
{
	return static_cast<yafaray_bool_t>(reinterpret_cast<yafaray::Interface *>(interface)->addFace({a, b, c}, {uv_a, uv_b, uv_c}));
}

yafaray_bool_t yafaray_addQuad(yafaray_Interface_t *interface, int a, int b, int c, int d)
{
	return static_cast<yafaray_bool_t>(reinterpret_cast<yafaray::Interface *>(interface)->addFace({a, b, c, d}, {}));
}

yafaray_bool_t yafaray_addQuadWithUv(yafaray_Interface_t *interface, int a, int b, int c, int d, int uv_a, int uv_b, int uv_c, int uv_d)
{
	return static_cast<yafaray_bool_t>(reinterpret_cast<yafaray::Interface *>(interface)->addFace({a, b, c, d}, {uv_a, uv_b, uv_c, uv_d}));
}

int yafaray_addUv(yafaray_Interface_t *interface, float u, float v) //!< add a UV coordinate pair; returns index to be used for addTriangle/addQuad
{
	return reinterpret_cast<yafaray::Interface *>(interface)->addUv({u, v});
}

yafaray_bool_t yafaray_smoothMesh(yafaray_Interface_t *interface, const char *name, float angle) //!< smooth vertex normals of mesh with given ID and angle (in degrees)
{
	return static_cast<yafaray_bool_t>(reinterpret_cast<yafaray::Interface *>(interface)->smoothVerticesNormals(name, angle));
}

int yafaray_createInstance(yafaray_Interface_t *interface)
{
	return reinterpret_cast<yafaray::Interface *>(interface)->createInstance();
}

yafaray_bool_t yafaray_addInstanceObject(yafaray_Interface_t *interface, int instance_id, const char *base_object_name)
{
	return static_cast<yafaray_bool_t>(reinterpret_cast<yafaray::Interface *>(interface)->addInstanceObject(instance_id, base_object_name));
}

yafaray_bool_t yafaray_addInstanceOfInstance(yafaray_Interface_t *interface, int instance_id, int base_instance_id)
{
	return static_cast<yafaray_bool_t>(reinterpret_cast<yafaray::Interface *>(interface)->addInstanceOfInstance(instance_id, base_instance_id));
}

yafaray_bool_t yafaray_addInstanceMatrix(yafaray_Interface_t *interface, int instance_id, float m_00, float m_01, float m_02, float m_03, float m_10, float m_11, float m_12, float m_13, float m_20, float m_21, float m_22, float m_23, float m_30, float m_31, float m_32, float m_33, float time)
{
	return static_cast<yafaray_bool_t>(reinterpret_cast<yafaray::Interface *>(interface)->addInstanceMatrix(instance_id, yafaray::Matrix4f{std::array<std::array<float, 4>, 4>{{{m_00, m_01, m_02, m_03}, {m_10, m_11, m_12, m_13}, {m_20, m_21, m_22, m_23}, {m_30, m_31, m_32, m_33}}}}, time));
}

yafaray_bool_t yafaray_addInstanceMatrixArray(yafaray_Interface_t *interface, int instance_id, const float *obj_to_world, float time)
{
	return static_cast<yafaray_bool_t>(reinterpret_cast<yafaray::Interface *>(interface)->addInstanceMatrix(instance_id, yafaray::Matrix4f{obj_to_world}, time));
}

void yafaray_paramsSetVector(yafaray_Interface_t *interface, const char *name, float x, float y, float z)
{
	reinterpret_cast<yafaray::Interface *>(interface)->paramsSetVector(name, {{x, y, z}});
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

void yafaray_paramsSetFloat(yafaray_Interface_t *interface, const char *name, float f)
{
	reinterpret_cast<yafaray::Interface *>(interface)->paramsSetFloat(name, f);
}

void yafaray_paramsSetColor(yafaray_Interface_t *interface, const char *name, float r, float g, float b, float a)
{
	reinterpret_cast<yafaray::Interface *>(interface)->paramsSetColor(name, yafaray::Rgba{r, g, b, a});
}

void yafaray_paramsSetMatrix(yafaray_Interface_t *interface, const char *name, float m_00, float m_01, float m_02, float m_03, float m_10, float m_11, float m_12, float m_13, float m_20, float m_21, float m_22, float m_23, float m_30, float m_31, float m_32, float m_33, yafaray_bool_t transpose)
{
	reinterpret_cast<yafaray::Interface *>(interface)->paramsSetMatrix(name, yafaray::Matrix4f{std::array<std::array<float, 4>, 4>{{{m_00, m_01, m_02, m_03}, {m_10, m_11, m_12, m_13}, {m_20, m_21, m_22, m_23}, {m_30, m_31, m_32, m_33}}}}, transpose);
}

void yafaray_paramsSetMatrixArray(yafaray_Interface_t *interface, const char *name, const float *matrix, yafaray_bool_t transpose)
{
	reinterpret_cast<yafaray::Interface *>(interface)->paramsSetMatrix(name, yafaray::Matrix4f{matrix}, transpose);
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
	return static_cast<yafaray_bool_t>(reinterpret_cast<yafaray::Interface *>(interface)->createObject(name).second.isOk());
}

yafaray_bool_t yafaray_createLight(yafaray_Interface_t *interface, const char *name)
{
	return static_cast<yafaray_bool_t>(reinterpret_cast<yafaray::Interface *>(interface)->createLight(name).second.isOk());
}

yafaray_bool_t yafaray_createTexture(yafaray_Interface_t *interface, const char *name)
{
	return static_cast<yafaray_bool_t>(reinterpret_cast<yafaray::Interface *>(interface)->createTexture(name).second.isOk());
}

yafaray_bool_t yafaray_createMaterial(yafaray_Interface_t *interface, const char *name)
{
	return static_cast<yafaray_bool_t>(reinterpret_cast<yafaray::Interface *>(interface)->createMaterial(name).second.isOk());
}

yafaray_bool_t yafaray_createCamera(yafaray_Interface_t *interface, const char *name)
{
	return static_cast<yafaray_bool_t>(reinterpret_cast<yafaray::Interface *>(interface)->createCamera(name).second.isOk());
}

yafaray_bool_t yafaray_defineBackground(yafaray_Interface_t *interface)
{
	return static_cast<yafaray_bool_t>(reinterpret_cast<yafaray::Interface *>(interface)->defineBackground().isOk());
}

yafaray_bool_t yafaray_defineSurfaceIntegrator(yafaray_Interface_t *interface)
{
	return static_cast<yafaray_bool_t>(reinterpret_cast<yafaray::Interface *>(interface)->defineSurfaceIntegrator().isOk());
}

yafaray_bool_t yafaray_defineVolumeIntegrator(yafaray_Interface_t *interface)
{
	return static_cast<yafaray_bool_t>(reinterpret_cast<yafaray::Interface *>(interface)->defineVolumeIntegrator().isOk());
}

yafaray_bool_t yafaray_createVolumeRegion(yafaray_Interface_t *interface, const char *name)
{
	return static_cast<yafaray_bool_t>(reinterpret_cast<yafaray::Interface *>(interface)->createVolumeRegion(name).second.isOk());
}

yafaray_bool_t yafaray_createRenderView(yafaray_Interface_t *interface, const char *name)
{
	return static_cast<yafaray_bool_t>(reinterpret_cast<yafaray::Interface *>(interface)->createRenderView(name).second.isOk());
}

yafaray_bool_t yafaray_createOutput(yafaray_Interface_t *interface, const char *name)
{
	return static_cast<yafaray_bool_t>(reinterpret_cast<yafaray::Interface *>(interface)->createOutput(name).second.isOk());
}

void yafaray_setRenderNotifyViewCallback(yafaray_Interface_t *interface, yafaray_RenderNotifyViewCallback_t callback, void *callback_data)
{
	reinterpret_cast<yafaray::Interface *>(interface)->setRenderNotifyViewCallback(callback, callback_data);
}

void yafaray_setRenderNotifyLayerCallback(yafaray_Interface_t *interface, yafaray_RenderNotifyLayerCallback_t callback, void *callback_data)
{
	reinterpret_cast<yafaray::Interface *>(interface)->setRenderNotifyLayerCallback(callback, callback_data);
}

void yafaray_setRenderPutPixelCallback(yafaray_Interface_t *interface, yafaray_RenderPutPixelCallback_t callback, void *callback_data)
{
	reinterpret_cast<yafaray::Interface *>(interface)->setRenderPutPixelCallback(callback, callback_data);
}

void yafaray_setRenderHighlightPixelCallback(yafaray_Interface_t *interface, yafaray_RenderHighlightPixelCallback_t callback, void *callback_data)
{
	reinterpret_cast<yafaray::Interface *>(interface)->setRenderHighlightPixelCallback(callback, callback_data);
}

void yafaray_setRenderFlushAreaCallback(yafaray_Interface_t *interface, yafaray_RenderFlushAreaCallback_t callback, void *callback_data)
{
	reinterpret_cast<yafaray::Interface *>(interface)->setRenderFlushAreaCallback(callback, callback_data);
}

void yafaray_setRenderFlushCallback(yafaray_Interface_t *interface, yafaray_RenderFlushCallback_t callback, void *callback_data)
{
	reinterpret_cast<yafaray::Interface *>(interface)->setRenderFlushCallback(callback, callback_data);
}

void yafaray_setRenderHighlightAreaCallback(yafaray_Interface_t *interface, yafaray_RenderHighlightAreaCallback_t callback, void *callback_data)
{
	reinterpret_cast<yafaray::Interface *>(interface)->setRenderHighlightAreaCallback(callback, callback_data);
}

int yafaray_getSceneFilmWidth(const yafaray_Interface_t *interface)
{
	return reinterpret_cast<const yafaray::Interface *>(interface)->getSceneFilmWidth();
}

int yafaray_getSceneFilmHeight(const yafaray_Interface_t *interface)
{
	return reinterpret_cast<const yafaray::Interface *>(interface)->getSceneFilmHeight();
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

void yafaray_render(yafaray_Interface_t *interface, yafaray_ProgressBarCallback_t monitor_callback, void *callback_data, yafaray_DisplayConsole_t progress_bar_display_console)
{
	std::unique_ptr<yafaray::ProgressBar> progress_bar;
	if(progress_bar_display_console == YAFARAY_DISPLAY_CONSOLE_NORMAL) progress_bar = std::make_unique<yafaray::ConsoleProgressBar>(80, monitor_callback, callback_data);
	else progress_bar = std::make_unique<yafaray::ProgressBar>(monitor_callback, callback_data);
	reinterpret_cast<yafaray::Interface *>(interface)->render(std::move(progress_bar));
}

void yafaray_defineLayer(yafaray_Interface_t *interface)
{
	reinterpret_cast<yafaray::Interface *>(interface)->defineLayer();
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
	return reinterpret_cast<yafaray_Image_t *>(reinterpret_cast<yafaray::Interface *>(interface)->createImage(name).first);
}

yafaray_bool_t yafaray_setImageColor(yafaray_Image_t *image, int x, int y, float red, float green, float blue, float alpha)
{
	auto *yaf_image = reinterpret_cast<yafaray::Image *>(image);
	if(!yaf_image) return YAFARAY_BOOL_FALSE;
	if(x < 0 || x >= yaf_image->getWidth() || y < 0 || y >= yaf_image->getHeight()) return YAFARAY_BOOL_FALSE;
	yaf_image->setColor({{x, y}}, {red, green, blue, alpha});
	return YAFARAY_BOOL_TRUE;
}

yafaray_bool_t yafaray_getImageColor(const yafaray_Image_t *image, int x, int y, float *red, float *green, float *blue, float *alpha)
{
	const auto *yaf_image = reinterpret_cast<const yafaray::Image *>(image);
	if(!yaf_image) return YAFARAY_BOOL_FALSE;
	if(x < 0 || x >= yaf_image->getWidth() || y < 0 || y >= yaf_image->getHeight()) return YAFARAY_BOOL_FALSE;
	const yafaray::Rgba color = yaf_image->getColor({{x, y}});
	*red = color.r_;
	*green = color.g_;
	*blue = color.b_;
	*alpha = color.a_;
	return YAFARAY_BOOL_TRUE;
}

int yafaray_getImageWidth(const yafaray_Image_t *image)
{
	if(image) return reinterpret_cast<const yafaray::Image *>(image)->getWidth();
	else return 0;
}

int yafaray_getImageHeight(const yafaray_Image_t *image)
{
	if(image) return reinterpret_cast<const yafaray::Image *>(image)->getHeight();
	else return 0;
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

int yafaray_getVersionMajor() { return yafaray::buildinfo::getVersionMajor(); }
int yafaray_getVersionMinor() { return yafaray::buildinfo::getVersionMinor(); }
int yafaray_getVersionPatch() { return yafaray::buildinfo::getVersionPatch(); }

char *createCString(const std::string &std_string)
{
	const size_t string_size = std_string.size();
	char *c_string = new char[string_size + 1];
	std::strcpy(c_string, std_string.c_str());
	return c_string;
}

char *yafaray_getVersionString()
{
	return createCString(yafaray::buildinfo::getVersionString());
}

char *yafaray_getLayersTable(const yafaray_Interface_t *interface)
{
	return createCString(reinterpret_cast<const yafaray::Interface *>(interface)->printLayersTable());
}

char *yafaray_getViewsTable(const yafaray_Interface_t *interface)
{
	return createCString(reinterpret_cast<const yafaray::Interface *>(interface)->printViewsTable());
}

void yafaray_deallocateCharPointer(char *string_pointer_to_deallocate)
{
	delete[] string_pointer_to_deallocate;
}
