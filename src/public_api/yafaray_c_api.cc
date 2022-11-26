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
#include "geometry/primitive/face_indices.h"
#include "math/math.h"
#include "image/image.h"
#include "interface/export/export_c.h"
#include "interface/export/export_python.h"
#include "interface/export/export_xml.h"
#include "interface/interface.h"
#include "render/progress_bar.h"
#include <cstring>

yafaray_Interface *yafaray_createInterface(yafaray_InterfaceType interface_type, const char *exported_file_path, const yafaray_LoggerCallback logger_callback, void *callback_data, yafaray_DisplayConsole display_console)
{
	yafaray::Interface *interface;
	if(interface_type == YAFARAY_INTERFACE_EXPORT_XML) interface = new yafaray::ExportXml(exported_file_path, logger_callback, callback_data, display_console);
	else if(interface_type == YAFARAY_INTERFACE_EXPORT_C) interface = new yafaray::ExportC(exported_file_path, logger_callback, callback_data, display_console);
	else if(interface_type == YAFARAY_INTERFACE_EXPORT_PYTHON) interface = new yafaray::ExportPython(exported_file_path, logger_callback, callback_data, display_console);
	else interface = new yafaray::Interface(logger_callback, callback_data, display_console);
	return reinterpret_cast<yafaray_Interface *>(interface);
}

void yafaray_destroyInterface(yafaray_Interface *interface)
{
	delete reinterpret_cast<yafaray::Interface *>(interface);
}

void yafaray_setLoggingCallback(yafaray_Interface *interface, const yafaray_LoggerCallback logger_callback, void *callback_data)
{
	reinterpret_cast<yafaray::Interface *>(interface)->setLoggingCallback(logger_callback, callback_data);
}

void yafaray_createScene(yafaray_Interface *interface)
{
	reinterpret_cast<yafaray::Interface *>(interface)->createScene();
}

yafaray_Bool yafaray_initObject(yafaray_Interface *interface, size_t object_id, size_t material_id) //!< initialize object. The material_id may or may not be used by the object depending on the type of the object
{
	return static_cast<yafaray_Bool>(reinterpret_cast<yafaray::Interface *>(interface)->initObject(object_id, material_id));
}

size_t yafaray_addVertex(yafaray_Interface *interface, size_t object_id, double x, double y, double z) //!< add vertex to mesh; returns index to be used for addTriangle/addQuad
{
	return reinterpret_cast<yafaray::Interface *>(interface)->addVertex(object_id, {{static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)}}, 0);
}

size_t yafaray_addVertexTimeStep(yafaray_Interface *interface, size_t object_id, double x, double y, double z, unsigned char time_step) //!< add vertex to mesh; returns index to be used for addTriangle/addQuad
{
	return reinterpret_cast<yafaray::Interface *>(interface)->addVertex(object_id, {{static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)}}, time_step);
}

size_t yafaray_addVertexWithOrco(yafaray_Interface *interface, size_t object_id, double x, double y, double z, double ox, double oy, double oz) //!< add vertex with Orco to mesh; returns index to be used for addTriangle/addQuad
{
	return reinterpret_cast<yafaray::Interface *>(interface)->addVertex(object_id, {{static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)}}, {{static_cast<float>(ox), static_cast<float>(oy), static_cast<float>(oz)}}, 0);
}

size_t yafaray_addVertexWithOrcoTimeStep(yafaray_Interface *interface, size_t object_id, double x, double y, double z, double ox, double oy, double oz, unsigned char time_step) //!< add vertex with Orco to mesh; returns index to be used for addTriangle/addQuad
{
	return reinterpret_cast<yafaray::Interface *>(interface)->addVertex(object_id, {{static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)}}, {{static_cast<float>(ox), static_cast<float>(oy), static_cast<float>(oz)}}, time_step);
}

void yafaray_addNormal(yafaray_Interface *interface, size_t object_id, double nx, double ny, double nz) //!< add vertex normal to mesh; the vertex that will be attached to is the last one inserted by addVertex method
{
	reinterpret_cast<yafaray::Interface *>(interface)->addVertexNormal(object_id, {{static_cast<float>(nx), static_cast<float>(ny), static_cast<float>(nz)}}, 0);
}

void yafaray_addNormalTimeStep(yafaray_Interface *interface, size_t object_id, double nx, double ny, double nz, unsigned char time_step) //!< add vertex normal to mesh; the vertex that will be attached to is the last one inserted by addVertex method
{
	reinterpret_cast<yafaray::Interface *>(interface)->addVertexNormal(object_id, {{static_cast<float>(nx), static_cast<float>(ny), static_cast<float>(nz)}}, time_step);
}

yafaray_Bool yafaray_addTriangle(yafaray_Interface *interface, size_t object_id, size_t a, size_t b, size_t c, size_t material_id)
{
	return static_cast<yafaray_Bool>(reinterpret_cast<yafaray::Interface *>(interface)->addFace(object_id, yafaray::FaceIndices<int>{{
		yafaray::VertexIndices<int>{static_cast<int>(a)},
		yafaray::VertexIndices<int>{static_cast<int>(b)},
		yafaray::VertexIndices<int>{static_cast<int>(c)},
	}}, material_id));
}

yafaray_Bool yafaray_addTriangleWithUv(yafaray_Interface *interface, size_t object_id, size_t a, size_t b, size_t c, size_t uv_a, size_t uv_b, size_t uv_c, size_t material_id)
{
	return static_cast<yafaray_Bool>(reinterpret_cast<yafaray::Interface *>(interface)->addFace(object_id, yafaray::FaceIndices<int>{{
		yafaray::VertexIndices<int>{static_cast<int>(a), yafaray::math::invalid<int>, static_cast<int>(uv_a)},
		yafaray::VertexIndices<int>{static_cast<int>(b), yafaray::math::invalid<int>, static_cast<int>(uv_b)},
		yafaray::VertexIndices<int>{static_cast<int>(c), yafaray::math::invalid<int>, static_cast<int>(uv_c)},
	}}, material_id));
}

yafaray_Bool yafaray_addQuad(yafaray_Interface *interface, size_t object_id, size_t a, size_t b, size_t c, size_t d, size_t material_id)
{
	return static_cast<yafaray_Bool>(reinterpret_cast<yafaray::Interface *>(interface)->addFace(object_id, yafaray::FaceIndices<int>{{
		yafaray::VertexIndices<int>{static_cast<int>(a)},
		yafaray::VertexIndices<int>{static_cast<int>(b)},
		yafaray::VertexIndices<int>{static_cast<int>(c)},
		yafaray::VertexIndices<int>{static_cast<int>(d)},
	}}, material_id));
}

yafaray_Bool yafaray_addQuadWithUv(yafaray_Interface *interface, size_t object_id, size_t a, size_t b, size_t c, size_t d, size_t uv_a, size_t uv_b, size_t uv_c, size_t uv_d, size_t material_id)
{
	return static_cast<yafaray_Bool>(reinterpret_cast<yafaray::Interface *>(interface)->addFace(object_id, yafaray::FaceIndices<int>{{
		yafaray::VertexIndices<int>{static_cast<int>(a), yafaray::math::invalid<int>, static_cast<int>(uv_a)},
		yafaray::VertexIndices<int>{static_cast<int>(b), yafaray::math::invalid<int>, static_cast<int>(uv_b)},
		yafaray::VertexIndices<int>{static_cast<int>(c), yafaray::math::invalid<int>, static_cast<int>(uv_c)},
		yafaray::VertexIndices<int>{static_cast<int>(d), yafaray::math::invalid<int>, static_cast<int>(uv_d)},
	}}, material_id));
}

size_t yafaray_addUv(yafaray_Interface *interface, size_t object_id, double u, double v) //!< add a UV coordinate pair; returns index to be used for addTriangle/addQuad
{
	return reinterpret_cast<yafaray::Interface *>(interface)->addUv(object_id, {static_cast<float>(u), static_cast<float>(v)});
}

yafaray_Bool yafaray_smoothObjectMesh(yafaray_Interface *interface, size_t object_id, double angle) //!< smooth vertex normals of mesh with given ID and angle (in degrees)
{
	return static_cast<yafaray_Bool>(reinterpret_cast<yafaray::Interface *>(interface)->smoothVerticesNormals(object_id, angle));
}

size_t yafaray_createInstance(yafaray_Interface *interface)
{
	return reinterpret_cast<yafaray::Interface *>(interface)->createInstance();
}

yafaray_Bool yafaray_addInstanceObject(yafaray_Interface *interface, size_t instance_id, size_t base_object_id)
{
	return static_cast<yafaray_Bool>(reinterpret_cast<yafaray::Interface *>(interface)->addInstanceObject(instance_id, base_object_id));
}

yafaray_Bool yafaray_addInstanceOfInstance(yafaray_Interface *interface, size_t instance_id, size_t base_instance_id)
{
	return static_cast<yafaray_Bool>(reinterpret_cast<yafaray::Interface *>(interface)->addInstanceOfInstance(instance_id, base_instance_id));
}

yafaray_Bool yafaray_addInstanceMatrix(yafaray_Interface *interface, size_t instance_id, double m_00, double m_01, double m_02, double m_03, double m_10, double m_11, double m_12, double m_13, double m_20, double m_21, double m_22, double m_23, double m_30, double m_31, double m_32, double m_33, float time)
{
	return static_cast<yafaray_Bool>(reinterpret_cast<yafaray::Interface *>(interface)->addInstanceMatrix(instance_id, yafaray::Matrix4f{std::array<std::array<float, 4>, 4>{{{static_cast<float>(m_00), static_cast<float>(m_01), static_cast<float>(m_02), static_cast<float>(m_03)}, {static_cast<float>(m_10), static_cast<float>(m_11), static_cast<float>(m_12), static_cast<float>(m_13)}, {static_cast<float>(m_20), static_cast<float>(m_21), static_cast<float>(m_22), static_cast<float>(m_23)}, {static_cast<float>(m_30), static_cast<float>(m_31), static_cast<float>(m_32), static_cast<float>(m_33)}}}}, time));
}

yafaray_Bool yafaray_addInstanceMatrixArray(yafaray_Interface *interface, size_t instance_id, const double *obj_to_world, float time)
{
	return static_cast<yafaray_Bool>(reinterpret_cast<yafaray::Interface *>(interface)->addInstanceMatrix(instance_id, yafaray::Matrix4f{obj_to_world}, time));
}

void yafaray_paramsSetVector(yafaray_Interface *interface, const char *name, double x, double y, double z)
{
	reinterpret_cast<yafaray::Interface *>(interface)->paramsSetVector(name, {{static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)}});
}

void yafaray_paramsSetString(yafaray_Interface *interface, const char *name, const char *s)
{
	reinterpret_cast<yafaray::Interface *>(interface)->paramsSetString(name, s);
}

void yafaray_paramsSetBool(yafaray_Interface *interface, const char *name, yafaray_Bool b)
{
	reinterpret_cast<yafaray::Interface *>(interface)->paramsSetBool(name, b);
}

void yafaray_paramsSetInt(yafaray_Interface *interface, const char *name, int i)
{
	reinterpret_cast<yafaray::Interface *>(interface)->paramsSetInt(name, i);
}

void yafaray_paramsSetFloat(yafaray_Interface *interface, const char *name, double f)
{
	reinterpret_cast<yafaray::Interface *>(interface)->paramsSetFloat(name, f);
}

void yafaray_paramsSetColor(yafaray_Interface *interface, const char *name, double r, double g, double b, double a)
{
	reinterpret_cast<yafaray::Interface *>(interface)->paramsSetColor(name, yafaray::Rgba{static_cast<float>(r), static_cast<float>(g), static_cast<float>(b), static_cast<float>(a)});
}

void yafaray_paramsSetMatrix(yafaray_Interface *interface, const char *name, float m_00, float m_01, float m_02, float m_03, float m_10, float m_11, float m_12, float m_13, float m_20, float m_21, float m_22, float m_23, float m_30, float m_31, float m_32, float m_33, yafaray_Bool transpose)
{
	reinterpret_cast<yafaray::Interface *>(interface)->paramsSetMatrix(name, yafaray::Matrix4f{std::array<std::array<float, 4>, 4>{{{m_00, m_01, m_02, m_03}, {m_10, m_11, m_12, m_13}, {m_20, m_21, m_22, m_23}, {m_30, m_31, m_32, m_33}}}}, transpose);
}

void yafaray_paramsSetMatrixArray(yafaray_Interface *interface, const char *name, const double *matrix, yafaray_Bool transpose)
{
	reinterpret_cast<yafaray::Interface *>(interface)->paramsSetMatrix(name, yafaray::Matrix4f{matrix}, transpose);
}

void yafaray_paramsClearAll(yafaray_Interface *interface) 	//!< clear the paramMap and paramList
{
	reinterpret_cast<yafaray::Interface *>(interface)->paramsClearAll();
}

void yafaray_paramsPushList(yafaray_Interface *interface) 	//!< push new list item in paramList (e.g. new shader node description)
{
	reinterpret_cast<yafaray::Interface *>(interface)->paramsPushList();
}

void yafaray_paramsEndList(yafaray_Interface *interface) 	//!< revert to writing to normal paramMap
{
	reinterpret_cast<yafaray::Interface *>(interface)->paramsEndList();
}

yafaray_ResultFlags yafaray_getObjectId(yafaray_Interface *interface, const char *name, size_t *id_obtained)
{
	auto result{reinterpret_cast<yafaray::Interface *>(interface)->getObjectId(name)};
	if(id_obtained) *id_obtained = result.first;
	return static_cast<yafaray_ResultFlags>(result.second.value());
}

yafaray_ResultFlags yafaray_getMaterialId(yafaray_Interface *interface, const char *name, size_t *id_obtained)
{
	auto result{reinterpret_cast<yafaray::Interface *>(interface)->getMaterialId(name)};
	if(id_obtained) *id_obtained = result.first;
	return static_cast<yafaray_ResultFlags>(result.second.value());
}

yafaray_ResultFlags yafaray_createObject(yafaray_Interface *interface, const char *name, size_t *id_obtained)
{
	auto creation_result{reinterpret_cast<yafaray::Interface *>(interface)->createObject(name)};
	if(id_obtained) *id_obtained = creation_result.first;
	return static_cast<yafaray_ResultFlags>(creation_result.second.flags_.value());
}

yafaray_ResultFlags yafaray_createLight(yafaray_Interface *interface, const char *name)
{
	auto creation_result{reinterpret_cast<yafaray::Interface *>(interface)->createLight(name)};
	return static_cast<yafaray_ResultFlags>(creation_result.second.flags_.value());
}

yafaray_ResultFlags yafaray_createTexture(yafaray_Interface *interface, const char *name)
{
	auto creation_result{reinterpret_cast<yafaray::Interface *>(interface)->createTexture(name)};
	return static_cast<yafaray_ResultFlags>(creation_result.second.flags_.value());
}

yafaray_ResultFlags yafaray_createMaterial(yafaray_Interface *interface, const char *name, size_t *id_obtained)
{
	auto creation_result{reinterpret_cast<yafaray::Interface *>(interface)->createMaterial(name)};
	if(id_obtained) *id_obtained = creation_result.first;
	return static_cast<yafaray_ResultFlags>(creation_result.second.flags_.value());
}

yafaray_ResultFlags yafaray_createCamera(yafaray_Interface *interface, const char *name)
{
	auto creation_result{reinterpret_cast<yafaray::Interface *>(interface)->createCamera(name)};
	return static_cast<yafaray_ResultFlags>(creation_result.second.flags_.value());
}

yafaray_ResultFlags yafaray_defineBackground(yafaray_Interface *interface)
{
	auto creation_result{reinterpret_cast<yafaray::Interface *>(interface)->defineBackground()};
	return static_cast<yafaray_ResultFlags>(creation_result.flags_.value());
}

yafaray_ResultFlags yafaray_defineSurfaceIntegrator(yafaray_Interface *interface)
{
	auto creation_result{reinterpret_cast<yafaray::Interface *>(interface)->defineSurfaceIntegrator()};
	return static_cast<yafaray_ResultFlags>(creation_result.flags_.value());
}

yafaray_ResultFlags yafaray_defineVolumeIntegrator(yafaray_Interface *interface)
{
	auto creation_result{reinterpret_cast<yafaray::Interface *>(interface)->defineVolumeIntegrator()};
	return static_cast<yafaray_ResultFlags>(creation_result.flags_.value());
}

yafaray_ResultFlags yafaray_createVolumeRegion(yafaray_Interface *interface, const char *name)
{
	auto creation_result{reinterpret_cast<yafaray::Interface *>(interface)->createVolumeRegion(name)};
	return static_cast<yafaray_ResultFlags>(creation_result.second.flags_.value());
}

yafaray_ResultFlags yafaray_createRenderView(yafaray_Interface *interface, const char *name)
{
	auto creation_result{reinterpret_cast<yafaray::Interface *>(interface)->createRenderView(name)};
	return static_cast<yafaray_ResultFlags>(creation_result.second.flags_.value());
}

yafaray_ResultFlags yafaray_createOutput(yafaray_Interface *interface, const char *name)
{
	auto creation_result{reinterpret_cast<yafaray::Interface *>(interface)->createOutput(name)};
	return static_cast<yafaray_ResultFlags>(creation_result.second.flags_.value());
}

void yafaray_setRenderNotifyViewCallback(yafaray_Interface *interface, yafaray_RenderNotifyViewCallback callback, void *callback_data)
{
	reinterpret_cast<yafaray::Interface *>(interface)->setRenderNotifyViewCallback(callback, callback_data);
}

void yafaray_setRenderNotifyLayerCallback(yafaray_Interface *interface, yafaray_RenderNotifyLayerCallback callback, void *callback_data)
{
	reinterpret_cast<yafaray::Interface *>(interface)->setRenderNotifyLayerCallback(callback, callback_data);
}

void yafaray_setRenderPutPixelCallback(yafaray_Interface *interface, yafaray_RenderPutPixelCallback callback, void *callback_data)
{
	reinterpret_cast<yafaray::Interface *>(interface)->setRenderPutPixelCallback(callback, callback_data);
}

void yafaray_setRenderHighlightPixelCallback(yafaray_Interface *interface, yafaray_RenderHighlightPixelCallback callback, void *callback_data)
{
	reinterpret_cast<yafaray::Interface *>(interface)->setRenderHighlightPixelCallback(callback, callback_data);
}

void yafaray_setRenderFlushAreaCallback(yafaray_Interface *interface, yafaray_RenderFlushAreaCallback callback, void *callback_data)
{
	reinterpret_cast<yafaray::Interface *>(interface)->setRenderFlushAreaCallback(callback, callback_data);
}

void yafaray_setRenderFlushCallback(yafaray_Interface *interface, yafaray_RenderFlushCallback callback, void *callback_data)
{
	reinterpret_cast<yafaray::Interface *>(interface)->setRenderFlushCallback(callback, callback_data);
}

void yafaray_setRenderHighlightAreaCallback(yafaray_Interface *interface, yafaray_RenderHighlightAreaCallback callback, void *callback_data)
{
	reinterpret_cast<yafaray::Interface *>(interface)->setRenderHighlightAreaCallback(callback, callback_data);
}

int yafaray_getSceneFilmWidth(const yafaray_Interface *interface)
{
	return reinterpret_cast<const yafaray::Interface *>(interface)->getSceneFilmWidth();
}

int yafaray_getSceneFilmHeight(const yafaray_Interface *interface)
{
	return reinterpret_cast<const yafaray::Interface *>(interface)->getSceneFilmHeight();
}

yafaray_Bool yafaray_removeOutput(yafaray_Interface *interface, const char *name)
{
	return static_cast<yafaray_Bool>(reinterpret_cast<yafaray::Interface *>(interface)->removeOutput(name));
}

void yafaray_clearOutputs(yafaray_Interface *interface)
{
	reinterpret_cast<yafaray::Interface *>(interface)->clearOutputs();
}

void yafaray_clearAll(yafaray_Interface *interface)
{
	reinterpret_cast<yafaray::Interface *>(interface)->clearAll();
}

void yafaray_setupRender(yafaray_Interface *interface)
{
	reinterpret_cast<yafaray::Interface *>(interface)->setupRender();
}

void yafaray_render(yafaray_Interface *interface, yafaray_ProgressBarCallback monitor_callback, void *callback_data, yafaray_DisplayConsole progress_bar_display_console)
{
	std::unique_ptr<yafaray::ProgressBar> progress_bar;
	if(progress_bar_display_console == YAFARAY_DISPLAY_CONSOLE_NORMAL) progress_bar = std::make_unique<yafaray::ConsoleProgressBar>(80, monitor_callback, callback_data);
	else progress_bar = std::make_unique<yafaray::ProgressBar>(monitor_callback, callback_data);
	reinterpret_cast<yafaray::Interface *>(interface)->render(std::move(progress_bar));
}

void yafaray_defineLayer(yafaray_Interface *interface)
{
	reinterpret_cast<yafaray::Interface *>(interface)->defineLayer();
}

void yafaray_enablePrintDateTime(yafaray_Interface *interface, yafaray_Bool value)
{
	reinterpret_cast<yafaray::Interface *>(interface)->enablePrintDateTime(value);
}

void yafaray_setConsoleVerbosityLevel(yafaray_Interface *interface, yafaray_LogLevel log_level)
{
	reinterpret_cast<yafaray::Interface *>(interface)->setConsoleVerbosityLevel(log_level);
}

void yafaray_setLogVerbosityLevel(yafaray_Interface *interface, yafaray_LogLevel log_level)
{
	reinterpret_cast<yafaray::Interface *>(interface)->setLogVerbosityLevel(log_level);
}

/*! Console Printing wrappers to report in color with yafaray's own console coloring */
void yafaray_printDebug(yafaray_Interface *interface, const char *msg)
{
	reinterpret_cast<yafaray::Interface *>(interface)->printDebug(msg);
}

void yafaray_printVerbose(yafaray_Interface *interface, const char *msg)
{
	reinterpret_cast<yafaray::Interface *>(interface)->printVerbose(msg);
}

void yafaray_printInfo(yafaray_Interface *interface, const char *msg)
{
	reinterpret_cast<yafaray::Interface *>(interface)->printInfo(msg);
}

void yafaray_printParams(yafaray_Interface *interface, const char *msg)
{
	reinterpret_cast<yafaray::Interface *>(interface)->printParams(msg);
}

void yafaray_printWarning(yafaray_Interface *interface, const char *msg)
{
	reinterpret_cast<yafaray::Interface *>(interface)->printWarning(msg);
}

void yafaray_printError(yafaray_Interface *interface, const char *msg)
{
	reinterpret_cast<yafaray::Interface *>(interface)->printError(msg);
}

void yafaray_setInputColorSpace(yafaray_Interface *interface, const char *color_space_string, float gamma_val)
{
	reinterpret_cast<yafaray::Interface *>(interface)->setInputColorSpace(color_space_string, gamma_val);
}

yafaray_ResultFlags yafaray_getImageId(yafaray_Interface *interface, const char *name, size_t *id_obtained)
{
	auto result{reinterpret_cast<yafaray::Interface *>(interface)->getImageId(name)};
	if(id_obtained) *id_obtained = result.first;
	return static_cast<yafaray_ResultFlags>(result.second.value());
}

yafaray_ResultFlags yafaray_createImage(yafaray_Interface *interface, const char *name, size_t *id_obtained)
{
	auto creation_result{reinterpret_cast<yafaray::Interface *>(interface)->createImage(name)};
	if(id_obtained) *id_obtained = creation_result.first;
	return static_cast<yafaray_ResultFlags>(creation_result.second.flags_.value());
}

yafaray_Bool yafaray_setImageColor(yafaray_Interface *interface, size_t image_id, int x, int y, float red, float green, float blue, float alpha)
{
	return static_cast<yafaray_Bool>(reinterpret_cast<yafaray::Interface *>(interface)->setImageColor(image_id, {{x, y}}, {red, green, blue, alpha}));
}

yafaray_Bool yafaray_getImageColor(yafaray_Interface *interface, size_t image_id, int x, int y, float *red, float *green, float *blue, float *alpha)
{
	const auto [color, result_ok]{reinterpret_cast<yafaray::Interface *>(interface)->getImageColor(image_id, {{x, y}})};
	if(!result_ok) return YAFARAY_BOOL_FALSE;
	*red = color.r_;
	*green = color.g_;
	*blue = color.b_;
	*alpha = color.a_;
	return YAFARAY_BOOL_TRUE;
}

int yafaray_getImageWidth(yafaray_Interface *interface, size_t image_id)
{
	return static_cast<yafaray_Bool>(reinterpret_cast<yafaray::Interface *>(interface)->getImageSize(image_id).first[yafaray::Axis::X]);
}

int yafaray_getImageHeight(yafaray_Interface *interface, size_t image_id)
{
	return static_cast<yafaray_Bool>(reinterpret_cast<yafaray::Interface *>(interface)->getImageSize(image_id).first[yafaray::Axis::Y]);
}

void yafaray_cancelRendering(yafaray_Interface *interface)
{
	reinterpret_cast<yafaray::Interface *>(interface)->cancel();
}

void yafaray_setConsoleLogColorsEnabled(yafaray_Interface *interface, yafaray_Bool colors_enabled)
{
	reinterpret_cast<yafaray::Interface *>(interface)->setConsoleLogColorsEnabled(colors_enabled);
}

yafaray_LogLevel yafaray_logLevelFromString(const char *log_level_string)
{
	return static_cast<yafaray_LogLevel>(yafaray::Logger::vlevelFromString(log_level_string));
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

char *yafaray_getLayersTable(const yafaray_Interface *interface)
{
	return createCString(reinterpret_cast<const yafaray::Interface *>(interface)->printLayersTable());
}

char *yafaray_getViewsTable(const yafaray_Interface *interface)
{
	return createCString(reinterpret_cast<const yafaray::Interface *>(interface)->printViewsTable());
}

void yafaray_deallocateCharPointer(char *string_pointer_to_deallocate)
{
	delete[] string_pointer_to_deallocate;
}
