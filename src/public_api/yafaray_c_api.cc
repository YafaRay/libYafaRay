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
#include "color/color.h"
#include "common/logger.h"
#include "common/version_build_info.h"
#include "geometry/matrix.h"
#include "geometry/primitive/face_indices.h"
#include "math/math.h"
#include "render/progress_bar.h"
#include "scene/scene.h"
#include "param/param.h"
#include "render/renderer.h"
#include "render/imagefilm.h"
#include <cstring>

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

yafaray_Logger *yafaray_createLogger(yafaray_LoggerCallback logger_callback, void *callback_data, yafaray_DisplayConsole display_console)
{
	auto logger{new yafaray::Logger(logger_callback, callback_data, display_console)};
	return reinterpret_cast<yafaray_Logger *>(logger);
}

void yafaray_setLoggerCallbacks(yafaray_Logger *logger, yafaray_LoggerCallback logger_callback, void *callback_data)
{
	auto yafaray_logger{reinterpret_cast<yafaray::Logger *>(logger)};
	yafaray_logger->setCallback(logger_callback, callback_data);
}

void yafaray_destroyLogger(yafaray_Logger *logger)
{
	delete reinterpret_cast<yafaray::Logger *>(logger);
}

yafaray_Film *yafaray_createFilm(yafaray_Logger *logger, yafaray_Renderer *renderer, const char *name, const yafaray_ParamMap *param_map)
{
	auto [image_film, image_film_result]{yafaray::ImageFilm::factory(*reinterpret_cast<yafaray::Logger *>(logger), reinterpret_cast<yafaray::Renderer *>(renderer)->getRenderControl(), name, *reinterpret_cast<const yafaray::ParamMap *>(param_map))};
	return reinterpret_cast<yafaray_Film *>(image_film);
}

void yafaray_destroyFilm(yafaray_Film *film)
{
	delete reinterpret_cast<yafaray::ImageFilm *>(film);
}

yafaray_Scene *yafaray_createScene(yafaray_Logger *logger, const char *name, const yafaray_ParamMap *param_map)
{
	auto scene{new yafaray::Scene(*reinterpret_cast<yafaray::Logger *>(logger), name, *reinterpret_cast<const yafaray::ParamMap *>(param_map))};
	return reinterpret_cast<yafaray_Scene *>(scene);
}

void yafaray_destroyScene(yafaray_Scene *scene)
{
	delete reinterpret_cast<yafaray::Scene *>(scene);
}

yafaray_ParamMap *yafaray_createParamMap()
{
	auto param_map{new yafaray::ParamMap()};
	return reinterpret_cast<yafaray_ParamMap *>(param_map);
}

void yafaray_destroyParamMap(yafaray_ParamMap *param_map)
{
	delete reinterpret_cast<yafaray::ParamMap *>(param_map);
}

yafaray_ParamMapList *yafaray_createParamMapList()
{
	auto param_map_list{new std::list<yafaray::ParamMap>()};
	return reinterpret_cast<yafaray_ParamMapList *>(param_map_list);
}

void yafaray_addParamMapToList(yafaray_ParamMapList *param_map_list, const yafaray_ParamMap *param_map)
{
	reinterpret_cast<std::list<yafaray::ParamMap> *>(param_map_list)->emplace_back(*reinterpret_cast<const yafaray::ParamMap *>(param_map));
}

void yafaray_clearParamMapList(yafaray_ParamMapList *param_map_list)
{
	reinterpret_cast<std::list<yafaray::ParamMap> *>(param_map_list)->clear();
}

void yafaray_destroyParamMapList(yafaray_ParamMapList *param_map_list)
{
	delete reinterpret_cast<std::list<yafaray::ParamMap> *>(param_map_list);
}

yafaray_Renderer *yafaray_createRenderer(yafaray_Logger *logger, const yafaray_Scene *scene, const char *name, yafaray_DisplayConsole display_console, yafaray_ParamMap *param_map)
{
	auto renderer{new yafaray::Renderer(*reinterpret_cast<yafaray::Logger *>(logger), name, *reinterpret_cast<const yafaray::Scene *>(scene), *reinterpret_cast<const yafaray::ParamMap *>(param_map), display_console)};
	return reinterpret_cast<yafaray_Renderer *>(renderer);
}

void yafaray_destroyRenderer(yafaray_Renderer *renderer)
{
	delete reinterpret_cast<yafaray::Renderer *>(renderer);
}

yafaray_Bool yafaray_initObject(yafaray_Scene *scene, size_t object_id, size_t material_id) //!< initialize object. The material_id may or may not be used by the object depending on the type of the object
{
	return static_cast<yafaray_Bool>(reinterpret_cast<yafaray::Scene *>(scene)->initObject(object_id, material_id));
}

size_t yafaray_addVertex(yafaray_Scene *scene, size_t object_id, double x, double y, double z) //!< add vertex to mesh; returns index to be used for addTriangle/addQuad
{
	return reinterpret_cast<yafaray::Scene *>(scene)->addVertex(object_id, {{static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)}}, 0);
}

size_t yafaray_addVertexTimeStep(yafaray_Scene *scene, size_t object_id, double x, double y, double z, unsigned char time_step) //!< add vertex to mesh; returns index to be used for addTriangle/addQuad
{
	return reinterpret_cast<yafaray::Scene *>(scene)->addVertex(object_id, {{static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)}}, time_step);
}

size_t yafaray_addVertexWithOrco(yafaray_Scene *scene, size_t object_id, double x, double y, double z, double ox, double oy, double oz) //!< add vertex with Orco to mesh; returns index to be used for addTriangle/addQuad
{
	return reinterpret_cast<yafaray::Scene *>(scene)->addVertex(object_id, {{static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)}}, {{static_cast<float>(ox), static_cast<float>(oy), static_cast<float>(oz)}}, 0);
}

size_t yafaray_addVertexWithOrcoTimeStep(yafaray_Scene *scene, size_t object_id, double x, double y, double z, double ox, double oy, double oz, unsigned char time_step) //!< add vertex with Orco to mesh; returns index to be used for addTriangle/addQuad
{
	return reinterpret_cast<yafaray::Scene *>(scene)->addVertex(object_id, {{static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)}}, {{static_cast<float>(ox), static_cast<float>(oy), static_cast<float>(oz)}}, time_step);
}

void yafaray_addNormal(yafaray_Scene *scene, size_t object_id, double nx, double ny, double nz) //!< add vertex normal to mesh; the vertex that will be attached to is the last one inserted by addVertex method
{
	reinterpret_cast<yafaray::Scene *>(scene)->addVertexNormal(object_id, {{static_cast<float>(nx), static_cast<float>(ny), static_cast<float>(nz)}}, 0);
}

void yafaray_addNormalTimeStep(yafaray_Scene *scene, size_t object_id, double nx, double ny, double nz, unsigned char time_step) //!< add vertex normal to mesh; the vertex that will be attached to is the last one inserted by addVertex method
{
	reinterpret_cast<yafaray::Scene *>(scene)->addVertexNormal(object_id, {{static_cast<float>(nx), static_cast<float>(ny), static_cast<float>(nz)}}, time_step);
}

yafaray_Bool yafaray_addTriangle(yafaray_Scene *scene, size_t object_id, size_t a, size_t b, size_t c, size_t material_id)
{
	return static_cast<yafaray_Bool>(reinterpret_cast<yafaray::Scene *>(scene)->addFace(object_id, yafaray::FaceIndices<int>{{
		yafaray::VertexIndices<int>{static_cast<int>(a)},
		yafaray::VertexIndices<int>{static_cast<int>(b)},
		yafaray::VertexIndices<int>{static_cast<int>(c)},
	}}, material_id));
}

yafaray_Bool yafaray_addTriangleWithUv(yafaray_Scene *scene, size_t object_id, size_t a, size_t b, size_t c, size_t uv_a, size_t uv_b, size_t uv_c, size_t material_id)
{
	return static_cast<yafaray_Bool>(reinterpret_cast<yafaray::Scene *>(scene)->addFace(object_id, yafaray::FaceIndices<int>{{
		yafaray::VertexIndices<int>{static_cast<int>(a), yafaray::math::invalid<int>, static_cast<int>(uv_a)},
		yafaray::VertexIndices<int>{static_cast<int>(b), yafaray::math::invalid<int>, static_cast<int>(uv_b)},
		yafaray::VertexIndices<int>{static_cast<int>(c), yafaray::math::invalid<int>, static_cast<int>(uv_c)},
	}}, material_id));
}

yafaray_Bool yafaray_addQuad(yafaray_Scene *scene, size_t object_id, size_t a, size_t b, size_t c, size_t d, size_t material_id)
{
	return static_cast<yafaray_Bool>(reinterpret_cast<yafaray::Scene *>(scene)->addFace(object_id, yafaray::FaceIndices<int>{{
		yafaray::VertexIndices<int>{static_cast<int>(a)},
		yafaray::VertexIndices<int>{static_cast<int>(b)},
		yafaray::VertexIndices<int>{static_cast<int>(c)},
		yafaray::VertexIndices<int>{static_cast<int>(d)},
	}}, material_id));
}

yafaray_Bool yafaray_addQuadWithUv(yafaray_Scene *scene, size_t object_id, size_t a, size_t b, size_t c, size_t d, size_t uv_a, size_t uv_b, size_t uv_c, size_t uv_d, size_t material_id)
{
	return static_cast<yafaray_Bool>(reinterpret_cast<yafaray::Scene *>(scene)->addFace(object_id, yafaray::FaceIndices<int>{{
		yafaray::VertexIndices<int>{static_cast<int>(a), yafaray::math::invalid<int>, static_cast<int>(uv_a)},
		yafaray::VertexIndices<int>{static_cast<int>(b), yafaray::math::invalid<int>, static_cast<int>(uv_b)},
		yafaray::VertexIndices<int>{static_cast<int>(c), yafaray::math::invalid<int>, static_cast<int>(uv_c)},
		yafaray::VertexIndices<int>{static_cast<int>(d), yafaray::math::invalid<int>, static_cast<int>(uv_d)},
	}}, material_id));
}

size_t yafaray_addUv(yafaray_Scene *scene, size_t object_id, double u, double v) //!< add a UV coordinate pair; returns index to be used for addTriangle/addQuad
{
	return reinterpret_cast<yafaray::Scene *>(scene)->addUv(object_id, {static_cast<float>(u), static_cast<float>(v)});
}

yafaray_Bool yafaray_smoothObjectMesh(yafaray_Scene *scene, size_t object_id, double angle) //!< smooth vertex normals of mesh with given ID and angle (in degrees)
{
	return static_cast<yafaray_Bool>(reinterpret_cast<yafaray::Scene *>(scene)->smoothVerticesNormals(object_id, angle));
}

size_t yafaray_createInstance(yafaray_Scene *scene)
{
	return reinterpret_cast<yafaray::Scene *>(scene)->createInstance();
}

yafaray_Bool yafaray_addInstanceObject(yafaray_Scene *scene, size_t instance_id, size_t base_object_id)
{
	return static_cast<yafaray_Bool>(reinterpret_cast<yafaray::Scene *>(scene)->addInstanceObject(instance_id, base_object_id));
}

yafaray_Bool yafaray_addInstanceOfInstance(yafaray_Scene *scene, size_t instance_id, size_t base_instance_id)
{
	return static_cast<yafaray_Bool>(reinterpret_cast<yafaray::Scene *>(scene)->addInstanceOfInstance(instance_id, base_instance_id));
}

yafaray_Bool yafaray_addInstanceMatrix(yafaray_Scene *scene, size_t instance_id, double m_00, double m_01, double m_02, double m_03, double m_10, double m_11, double m_12, double m_13, double m_20, double m_21, double m_22, double m_23, double m_30, double m_31, double m_32, double m_33, float time)
{
	return static_cast<yafaray_Bool>(reinterpret_cast<yafaray::Scene *>(scene)->addInstanceMatrix(instance_id, yafaray::Matrix4f{std::array<std::array<float, 4>, 4>{{{static_cast<float>(m_00), static_cast<float>(m_01), static_cast<float>(m_02), static_cast<float>(m_03)}, {static_cast<float>(m_10), static_cast<float>(m_11), static_cast<float>(m_12), static_cast<float>(m_13)}, {static_cast<float>(m_20), static_cast<float>(m_21), static_cast<float>(m_22), static_cast<float>(m_23)}, {static_cast<float>(m_30), static_cast<float>(m_31), static_cast<float>(m_32), static_cast<float>(m_33)}}}}, time));
}

yafaray_Bool yafaray_addInstanceMatrixArray(yafaray_Scene *scene, size_t instance_id, const double *obj_to_world, float time)
{
	return static_cast<yafaray_Bool>(reinterpret_cast<yafaray::Scene *>(scene)->addInstanceMatrix(instance_id, yafaray::Matrix4f{obj_to_world}, time));
}

void yafaray_setParamMapVector(yafaray_ParamMap *param_map, const char *name, double x, double y, double z)
{
	auto &params{*reinterpret_cast<yafaray::ParamMap *>(param_map)};
	params[std::string(name)] = yafaray::Vec3f{{static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)}};
}

void yafaray_setParamMapString(yafaray_ParamMap *param_map, const char *name, const char *s)
{
	auto &params{*reinterpret_cast<yafaray::ParamMap *>(param_map)};
	params[std::string(name)] = std::string(s);
}

void yafaray_setParamMapBool(yafaray_ParamMap *param_map, const char *name, yafaray_Bool b)
{
	auto &params{*reinterpret_cast<yafaray::ParamMap *>(param_map)};
	params[std::string(name)] = (b == YAFARAY_BOOL_FALSE) ? false : true;
}

void yafaray_setParamMapInt(yafaray_ParamMap *param_map, const char *name, int i)
{
	auto &params{*reinterpret_cast<yafaray::ParamMap *>(param_map)};
	params[std::string(name)] = i;
}

void yafaray_setParamMapFloat(yafaray_ParamMap *param_map, const char *name, double f)
{
	auto &params{*reinterpret_cast<yafaray::ParamMap *>(param_map)};
	params[std::string(name)] = yafaray::Parameter{f};
}

void yafaray_setParamMapColor(yafaray_ParamMap *param_map, const char *name, double r, double g, double b, double a)
{
	auto &params{*reinterpret_cast<yafaray::ParamMap *>(param_map)};
	params[std::string(name)] = yafaray::Rgba{static_cast<float>(r), static_cast<float>(g), static_cast<float>(b), static_cast<float>(a)};
}

void paramsSetMatrix(yafaray::ParamMap &param_map, const std::string &name, yafaray::Matrix4f &&matrix, bool transpose) noexcept
{
	if(transpose)
	{
		param_map[std::string(name)] = std::move(matrix.transpose());
	}
	else param_map[std::string(name)] = std::move(matrix);
}

void yafaray_setParamMapMatrix(yafaray_ParamMap *param_map, const char *name, double m_00, double m_01, double m_02, double m_03, double m_10, double m_11, double m_12, double m_13, double m_20, double m_21, double m_22, double m_23, double m_30, double m_31, double m_32, double m_33, yafaray_Bool transpose)
{
	paramsSetMatrix(*reinterpret_cast<yafaray::ParamMap *>(param_map), name, yafaray::Matrix4f{std::array<std::array<float, 4>, 4>{{{static_cast<float>(m_00), static_cast<float>(m_01), static_cast<float>(m_02), static_cast<float>(m_03) }, {static_cast<float>(m_10), static_cast<float>(m_11), static_cast<float>(m_12), static_cast<float>(m_13) }, {static_cast<float>(m_20), static_cast<float>(m_21), static_cast<float>(m_22), static_cast<float>(m_23) }, {static_cast<float>(m_30), static_cast<float>(m_31), static_cast<float>(m_32), static_cast<float>(m_33) }}}}, transpose);
}

void yafaray_setParamMapMatrixArray(yafaray_ParamMap *param_map, const char *name, const double *matrix, yafaray_Bool transpose)
{
	paramsSetMatrix(*reinterpret_cast<yafaray::ParamMap *>(param_map), name, yafaray::Matrix4f{matrix}, transpose);
}

void yafaray_clearParamMap(yafaray_ParamMap *param_map) 	//!< clear the paramMap and paramList
{
	reinterpret_cast<yafaray::ParamMap *>(param_map)->clear();
}

yafaray_ResultFlags yafaray_getObjectId(yafaray_Scene *scene, size_t *id_obtained, const char *name)
{
	auto [object, object_id, object_result]{reinterpret_cast<yafaray::Scene *>(scene)->getObject(name)};
	if(id_obtained) *id_obtained = object_id;
	return static_cast<yafaray_ResultFlags>(object_result.value());
}

yafaray_ResultFlags yafaray_getMaterialId(yafaray_Scene *scene, size_t *id_obtained, const char *name)
{
	auto [material_id, material_result]{reinterpret_cast<yafaray::Scene *>(scene)->getMaterial(name)};
	if(id_obtained) *id_obtained = material_id;
	return static_cast<yafaray_ResultFlags>(material_result.value());
}

yafaray_ResultFlags yafaray_createObject(yafaray_Scene *scene, size_t *id_obtained, const char *name, const yafaray_ParamMap *param_map)
{
	auto creation_result{reinterpret_cast<yafaray::Scene *>(scene)->createObject(name, *reinterpret_cast<const yafaray::ParamMap *>(param_map))};
	if(id_obtained) *id_obtained = creation_result.first;
	return static_cast<yafaray_ResultFlags>(creation_result.second.flags_.value());
}

yafaray_ResultFlags yafaray_createLight(yafaray_Scene *scene, const char *name, const yafaray_ParamMap *param_map)
{
	auto creation_result{reinterpret_cast<yafaray::Scene *>(scene)->createLight(name, *reinterpret_cast<const yafaray::ParamMap *>(param_map))};
	return static_cast<yafaray_ResultFlags>(creation_result.second.flags_.value());
}

yafaray_ResultFlags yafaray_createTexture(yafaray_Scene *scene, const char *name, const yafaray_ParamMap *param_map)
{
	auto creation_result{reinterpret_cast<yafaray::Scene *>(scene)->createTexture(name, *reinterpret_cast<const yafaray::ParamMap *>(param_map))};
	return static_cast<yafaray_ResultFlags>(creation_result.second.flags_.value());
}

yafaray_ResultFlags yafaray_createMaterial(yafaray_Scene *scene, size_t *id_obtained, const char *name, const yafaray_ParamMap *param_map, const yafaray_ParamMapList *param_map_list_nodes)
{
	auto creation_result{reinterpret_cast<yafaray::Scene *>(scene)->createMaterial(name, *reinterpret_cast<const yafaray::ParamMap *>(param_map), *reinterpret_cast<const std::list<yafaray::ParamMap> *>(param_map_list_nodes))};
	if(id_obtained) *id_obtained = creation_result.first;
	return static_cast<yafaray_ResultFlags>(creation_result.second.flags_.value());
}

yafaray_ResultFlags yafaray_defineCamera(yafaray_Film *film, const char *name, const yafaray_ParamMap *param_map)
{
	auto creation_result{reinterpret_cast<yafaray::ImageFilm *>(film)->defineCamera(name, *reinterpret_cast<const yafaray::ParamMap *>(param_map))};
	return static_cast<yafaray_ResultFlags>(creation_result.flags_.value());
}

yafaray_ResultFlags yafaray_defineBackground(yafaray_Scene *scene, const yafaray_ParamMap *param_map)
{
	auto creation_result{reinterpret_cast<yafaray::Scene *>(scene)->defineBackground(*reinterpret_cast<const yafaray::ParamMap *>(param_map))};
	return static_cast<yafaray_ResultFlags>(creation_result.flags_.value());
}

yafaray_ResultFlags yafaray_defineSurfaceIntegrator(yafaray_Renderer *renderer, const yafaray_ParamMap *param_map)
{
	auto creation_result{reinterpret_cast<yafaray::Renderer *>(renderer)->defineSurfaceIntegrator(*reinterpret_cast<const yafaray::ParamMap *>(param_map))};
	return static_cast<yafaray_ResultFlags>(creation_result.flags_.value());
}

yafaray_ResultFlags yafaray_defineVolumeIntegrator(yafaray_Renderer *renderer, const yafaray_Scene *scene, const yafaray_ParamMap *param_map)
{
	auto creation_result{reinterpret_cast<yafaray::Renderer *>(renderer)->defineVolumeIntegrator(*reinterpret_cast<const yafaray::Scene *>(scene), *reinterpret_cast<const yafaray::ParamMap *>(param_map))};
	return static_cast<yafaray_ResultFlags>(creation_result.flags_.value());
}

yafaray_ResultFlags yafaray_createVolumeRegion(yafaray_Scene *scene, const char *name, const yafaray_ParamMap *param_map)
{
	auto creation_result{reinterpret_cast<yafaray::Scene *>(scene)->createVolumeRegion(name, *reinterpret_cast<const yafaray::ParamMap *>(param_map))};
	return static_cast<yafaray_ResultFlags>(creation_result.second.flags_.value());
}

yafaray_ResultFlags yafaray_createOutput(yafaray_Film *film, const char *name, const yafaray_ParamMap *param_map)
{
	auto creation_result{reinterpret_cast<yafaray::ImageFilm *>(film)->createOutput(name, *reinterpret_cast<const yafaray::ParamMap *>(param_map))};
	return static_cast<yafaray_ResultFlags>(creation_result.second.flags_.value());
}

void yafaray_setNotifyLayerCallback(yafaray_Film *film, yafaray_RenderNotifyLayerCallback callback, void *callback_data)
{
	reinterpret_cast<yafaray::ImageFilm *>(film)->setRenderNotifyLayerCallback(callback, callback_data);
}

void yafaray_setPutPixelCallback(yafaray_Film *film, yafaray_RenderPutPixelCallback callback, void *callback_data)
{
	reinterpret_cast<yafaray::ImageFilm *>(film)->setRenderPutPixelCallback(callback, callback_data);
}

void yafaray_setHighlightPixelCallback(yafaray_Film *film, yafaray_RenderHighlightPixelCallback callback, void *callback_data)
{
	reinterpret_cast<yafaray::ImageFilm *>(film)->setRenderHighlightPixelCallback(callback, callback_data);
}

void yafaray_setFlushAreaCallback(yafaray_Film *film, yafaray_RenderFlushAreaCallback callback, void *callback_data)
{
	reinterpret_cast<yafaray::ImageFilm *>(film)->setRenderFlushAreaCallback(callback, callback_data);
}

void yafaray_setFlushCallback(yafaray_Film *film, yafaray_RenderFlushCallback callback, void *callback_data)
{
	reinterpret_cast<yafaray::ImageFilm *>(film)->setRenderFlushCallback(callback, callback_data);
}

void yafaray_setHighlightAreaCallback(yafaray_Film *film, yafaray_RenderHighlightAreaCallback callback, void *callback_data)
{
	reinterpret_cast<yafaray::ImageFilm *>(film)->setRenderHighlightAreaCallback(callback, callback_data);
}

int yafaray_getFilmWidth(const yafaray_Film *film)
{
	const auto image_film{reinterpret_cast<const yafaray::ImageFilm *>(film)};
	if(image_film) return image_film->getWidth();
	else return 0;
}

int yafaray_getFilmHeight(const yafaray_Film *film)
{
	const auto image_film{reinterpret_cast<const yafaray::ImageFilm *>(film)};
	if(image_film) return image_film->getHeight();
	else return 0;
}

void yafaray_setupRender(yafaray_Scene *scene, yafaray_Renderer *renderer, const yafaray_ParamMap *param_map)
{
	auto yaf_scene{reinterpret_cast<yafaray::Scene *>(scene)};
	auto yaf_renderer{reinterpret_cast<yafaray::Renderer *>(renderer)};
	yaf_renderer->setupSceneRenderParams(*reinterpret_cast<const yafaray::ParamMap *>(param_map));
	yaf_scene->init();
}

void yafaray_render(yafaray_Renderer *renderer, yafaray_Film *film, const yafaray_Scene *scene, yafaray_ProgressBarCallback monitor_callback, void *callback_data, yafaray_DisplayConsole progress_bar_display_console)
{
	std::unique_ptr<yafaray::ProgressBar> progress_bar;
	if(progress_bar_display_console == YAFARAY_DISPLAY_CONSOLE_NORMAL) progress_bar = std::make_unique<yafaray::ConsoleProgressBar>(80, monitor_callback, callback_data);
	else progress_bar = std::make_unique<yafaray::ProgressBar>(monitor_callback, callback_data);
	reinterpret_cast<yafaray::Renderer *>(renderer)->render(*reinterpret_cast<yafaray::ImageFilm *>(film), std::move(progress_bar), *reinterpret_cast<const yafaray::Scene *>(scene));
}

void yafaray_defineLayer(yafaray_Film *film, const yafaray_ParamMap *param_map)
{
	reinterpret_cast<yafaray::ImageFilm *>(film)->defineLayer(*reinterpret_cast<const yafaray::ParamMap *>(param_map));
}

void yafaray_enablePrintDateTime(yafaray_Logger *logger, yafaray_Bool value)
{
	reinterpret_cast<yafaray::Logger *>(logger)->enablePrintDateTime(value);
}

void yafaray_setConsoleVerbosityLevel(yafaray_Logger *logger, yafaray_LogLevel log_level)
{
	reinterpret_cast<yafaray::Logger *>(logger)->setConsoleMasterVerbosity(log_level);
}

void yafaray_setLogVerbosityLevel(yafaray_Logger *logger, yafaray_LogLevel log_level)
{
	reinterpret_cast<yafaray::Logger *>(logger)->setLogMasterVerbosity(log_level);
}

/*! Console Printing wrappers to report in color with yafaray's own console coloring */
void yafaray_printDebug(yafaray_Logger *logger, const char *msg)
{
	reinterpret_cast<yafaray::Logger *>(logger)->logDebug(msg);
}

void yafaray_printVerbose(yafaray_Logger *logger, const char *msg)
{
	reinterpret_cast<yafaray::Logger *>(logger)->logVerbose(msg);
}

void yafaray_printInfo(yafaray_Logger *logger, const char *msg)
{
	reinterpret_cast<yafaray::Logger *>(logger)->logInfo(msg);
}

void yafaray_printParams(yafaray_Logger *logger, const char *msg)
{
	reinterpret_cast<yafaray::Logger *>(logger)->logParams(msg);
}

void yafaray_printWarning(yafaray_Logger *logger, const char *msg)
{
	reinterpret_cast<yafaray::Logger *>(logger)->logWarning(msg);
}

void yafaray_printError(yafaray_Logger *logger, const char *msg)
{
	reinterpret_cast<yafaray::Logger *>(logger)->logError(msg);
}

void yafaray_setInputColorSpace(yafaray_ParamMap *param_map, const char *color_space_string, float gamma_val)
{
	reinterpret_cast<yafaray::ParamMap *>(param_map)->setInputColorSpace(color_space_string, gamma_val);
}

yafaray_ResultFlags yafaray_getImageId(yafaray_Scene *scene, const char *name, size_t *id_obtained)
{
	auto [image, image_id, image_result]{reinterpret_cast<yafaray::Scene *>(scene)->getImage(name)};
	if(id_obtained) *id_obtained = image_id;
	return static_cast<yafaray_ResultFlags>(image_result.value());
}

yafaray_ResultFlags yafaray_createImage(yafaray_Scene *scene, const char *name, size_t *id_obtained, const yafaray_ParamMap *param_map)
{
	auto creation_result{reinterpret_cast<yafaray::Scene *>(scene)->createImage(name, *reinterpret_cast<const yafaray::ParamMap *>(param_map))};
	if(id_obtained) *id_obtained = creation_result.first;
	return static_cast<yafaray_ResultFlags>(creation_result.second.flags_.value());
}

yafaray_Bool yafaray_setImageColor(yafaray_Scene *scene, size_t image_id, int x, int y, float red, float green, float blue, float alpha)
{
	return static_cast<yafaray_Bool>(reinterpret_cast<yafaray::Scene *>(scene)->setImageColor(image_id, {{x, y}}, {red, green, blue, alpha}));
}

yafaray_Bool yafaray_getImageColor(yafaray_Scene *scene, size_t image_id, int x, int y, float *red, float *green, float *blue, float *alpha)
{
	const auto [color, result_ok]{reinterpret_cast<yafaray::Scene *>(scene)->getImageColor(image_id, {{x, y}})};
	if(!result_ok) return YAFARAY_BOOL_FALSE;
	*red = color.r_;
	*green = color.g_;
	*blue = color.b_;
	*alpha = color.a_;
	return YAFARAY_BOOL_TRUE;
}

int yafaray_getImageWidth(yafaray_Scene *scene, size_t image_id)
{
	return static_cast<yafaray_Bool>(reinterpret_cast<yafaray::Scene *>(scene)->getImageSize(image_id).first[yafaray::Axis::X]);
}

int yafaray_getImageHeight(yafaray_Scene *scene, size_t image_id)
{
	return static_cast<yafaray_Bool>(reinterpret_cast<yafaray::Scene *>(scene)->getImageSize(image_id).first[yafaray::Axis::Y]);
}

void yafaray_cancelRendering(yafaray_Logger *logger, yafaray_Renderer *renderer)
{
	if(renderer) reinterpret_cast<yafaray::Renderer *>(renderer)->getRenderControl().setCanceled();
	reinterpret_cast<yafaray::Logger *>(logger)->logWarning("Interface: Render canceled by user.");
}

void yafaray_setConsoleLogColorsEnabled(yafaray_Logger *logger, yafaray_Bool colors_enabled)
{
	reinterpret_cast<yafaray::Logger *>(logger)->setConsoleLogColorsEnabled(colors_enabled);
}

yafaray_LogLevel yafaray_logLevelFromString(const char *log_level_string)
{
	return static_cast<yafaray_LogLevel>(yafaray::Logger::vlevelFromString(log_level_string));
}

int yafaray_getVersionMajor() { return yafaray::buildinfo::getVersionMajor(); }
int yafaray_getVersionMinor() { return yafaray::buildinfo::getVersionMinor(); }
int yafaray_getVersionPatch() { return yafaray::buildinfo::getVersionPatch(); }

char *createCharString(const std::string &std_string)
{
	auto c_string{new char[std_string.size() + 1]};
	std::strcpy(c_string, std_string.c_str());
	return c_string;
}

char *yafaray_getVersionString()
{
	return createCharString(yafaray::buildinfo::getVersionString());
}

char *yafaray_getLayersTable(const yafaray_Film *film)
{
	return createCharString(reinterpret_cast<const yafaray::ImageFilm *>(film)->getLayers()->printExportedTable());
}

void yafaray_destroyCharString(char *string)
{
	delete[] string;
}
