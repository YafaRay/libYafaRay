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
#include "scene/scene.h"
#include "geometry/matrix.h"
#include "geometry/primitive/face_indices.h"
#include "param/param_result.h"
#include "color/color.h"

yafaray_Scene *yafaray_createScene(yafaray_Logger *logger, const char *name, const yafaray_ParamMap *param_map)
{
	if(!logger || !name || !param_map) return nullptr;
	auto scene{new yafaray::Scene(*reinterpret_cast<yafaray::Logger *>(logger), name, *reinterpret_cast<const yafaray::ParamMap *>(param_map))};
	return reinterpret_cast<yafaray_Scene *>(scene);
}

void yafaray_destroyScene(yafaray_Scene *scene)
{
	delete reinterpret_cast<yafaray::Scene *>(scene);
}

yafaray_Bool yafaray_initObject(yafaray_Scene *scene, size_t object_id, size_t material_id) //!< initialize object. The material_id may or may not be used by the object depending on the type of the object
{
	if(!scene) return YAFARAY_BOOL_FALSE;
	return static_cast<yafaray_Bool>(reinterpret_cast<yafaray::Scene *>(scene)->initObject(object_id, material_id));
}

size_t yafaray_addVertex(yafaray_Scene *scene, size_t object_id, double x, double y, double z) //!< add vertex to mesh; returns index to be used for addTriangle/addQuad
{
	if(!scene) return 0;
	return reinterpret_cast<yafaray::Scene *>(scene)->addVertex(object_id, {{static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)}}, 0);
}

size_t yafaray_addVertexTimeStep(yafaray_Scene *scene, size_t object_id, double x, double y, double z, unsigned char time_step) //!< add vertex to mesh; returns index to be used for addTriangle/addQuad
{
	if(!scene) return 0;
	return reinterpret_cast<yafaray::Scene *>(scene)->addVertex(object_id, {{static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)}}, time_step);
}

size_t yafaray_addVertexWithOrco(yafaray_Scene *scene, size_t object_id, double x, double y, double z, double ox, double oy, double oz) //!< add vertex with Orco to mesh; returns index to be used for addTriangle/addQuad
{
	if(!scene) return 0;
	return reinterpret_cast<yafaray::Scene *>(scene)->addVertex(object_id, {{static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)}}, {{static_cast<float>(ox), static_cast<float>(oy), static_cast<float>(oz)}}, 0);
}

size_t yafaray_addVertexWithOrcoTimeStep(yafaray_Scene *scene, size_t object_id, double x, double y, double z, double ox, double oy, double oz, unsigned char time_step) //!< add vertex with Orco to mesh; returns index to be used for addTriangle/addQuad
{
	if(!scene) return 0;
	return reinterpret_cast<yafaray::Scene *>(scene)->addVertex(object_id, {{static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)}}, {{static_cast<float>(ox), static_cast<float>(oy), static_cast<float>(oz)}}, time_step);
}

void yafaray_addNormal(yafaray_Scene *scene, size_t object_id, double nx, double ny, double nz) //!< add vertex normal to mesh; the vertex that will be attached to is the last one inserted by addVertex method
{
	if(!scene) return;
	reinterpret_cast<yafaray::Scene *>(scene)->addVertexNormal(object_id, {{static_cast<float>(nx), static_cast<float>(ny), static_cast<float>(nz)}}, 0);
}

void yafaray_addNormalTimeStep(yafaray_Scene *scene, size_t object_id, double nx, double ny, double nz, unsigned char time_step) //!< add vertex normal to mesh; the vertex that will be attached to is the last one inserted by addVertex method
{
	if(!scene) return;
	reinterpret_cast<yafaray::Scene *>(scene)->addVertexNormal(object_id, {{static_cast<float>(nx), static_cast<float>(ny), static_cast<float>(nz)}}, time_step);
}

yafaray_Bool yafaray_addTriangle(yafaray_Scene *scene, size_t object_id, size_t a, size_t b, size_t c, size_t material_id)
{
	if(!scene) return YAFARAY_BOOL_FALSE;
	return static_cast<yafaray_Bool>(reinterpret_cast<yafaray::Scene *>(scene)->addFace(object_id, yafaray::FaceIndices<int>{{
		yafaray::VertexIndices<int>{static_cast<int>(a)},
		yafaray::VertexIndices<int>{static_cast<int>(b)},
		yafaray::VertexIndices<int>{static_cast<int>(c)},
	}}, material_id));
}

yafaray_Bool yafaray_addTriangleWithUv(yafaray_Scene *scene, size_t object_id, size_t a, size_t b, size_t c, size_t uv_a, size_t uv_b, size_t uv_c, size_t material_id)
{
	if(!scene) return YAFARAY_BOOL_FALSE;
	return static_cast<yafaray_Bool>(reinterpret_cast<yafaray::Scene *>(scene)->addFace(object_id, yafaray::FaceIndices<int>{{
		yafaray::VertexIndices<int>{static_cast<int>(a), yafaray::math::invalid<int>, static_cast<int>(uv_a)},
		yafaray::VertexIndices<int>{static_cast<int>(b), yafaray::math::invalid<int>, static_cast<int>(uv_b)},
		yafaray::VertexIndices<int>{static_cast<int>(c), yafaray::math::invalid<int>, static_cast<int>(uv_c)},
	}}, material_id));
}

yafaray_Bool yafaray_addQuad(yafaray_Scene *scene, size_t object_id, size_t a, size_t b, size_t c, size_t d, size_t material_id)
{
	if(!scene) return YAFARAY_BOOL_FALSE;
	return static_cast<yafaray_Bool>(reinterpret_cast<yafaray::Scene *>(scene)->addFace(object_id, yafaray::FaceIndices<int>{{
		yafaray::VertexIndices<int>{static_cast<int>(a)},
		yafaray::VertexIndices<int>{static_cast<int>(b)},
		yafaray::VertexIndices<int>{static_cast<int>(c)},
		yafaray::VertexIndices<int>{static_cast<int>(d)},
	}}, material_id));
}

yafaray_Bool yafaray_addQuadWithUv(yafaray_Scene *scene, size_t object_id, size_t a, size_t b, size_t c, size_t d, size_t uv_a, size_t uv_b, size_t uv_c, size_t uv_d, size_t material_id)
{
	if(!scene) return YAFARAY_BOOL_FALSE;
	return static_cast<yafaray_Bool>(reinterpret_cast<yafaray::Scene *>(scene)->addFace(object_id, yafaray::FaceIndices<int>{{
		yafaray::VertexIndices<int>{static_cast<int>(a), yafaray::math::invalid<int>, static_cast<int>(uv_a)},
		yafaray::VertexIndices<int>{static_cast<int>(b), yafaray::math::invalid<int>, static_cast<int>(uv_b)},
		yafaray::VertexIndices<int>{static_cast<int>(c), yafaray::math::invalid<int>, static_cast<int>(uv_c)},
		yafaray::VertexIndices<int>{static_cast<int>(d), yafaray::math::invalid<int>, static_cast<int>(uv_d)},
	}}, material_id));
}

size_t yafaray_addUv(yafaray_Scene *scene, size_t object_id, double u, double v) //!< add a UV coordinate pair; returns index to be used for addTriangle/addQuad
{
	if(!scene) return 0;
	return reinterpret_cast<yafaray::Scene *>(scene)->addUv(object_id, {static_cast<float>(u), static_cast<float>(v)});
}

yafaray_Bool yafaray_smoothObjectMesh(yafaray_Scene *scene, size_t object_id, double angle) //!< smooth vertex normals of mesh with given ID and angle (in degrees)
{
	if(!scene) return YAFARAY_BOOL_FALSE;
	return static_cast<yafaray_Bool>(reinterpret_cast<yafaray::Scene *>(scene)->smoothVerticesNormals(object_id, angle));
}

size_t yafaray_createInstance(yafaray_Scene *scene)
{
	if(!scene) return 0;
	return reinterpret_cast<yafaray::Scene *>(scene)->createInstance();
}

yafaray_Bool yafaray_addInstanceObject(yafaray_Scene *scene, size_t instance_id, size_t base_object_id)
{
	if(!scene) return YAFARAY_BOOL_FALSE;
	return static_cast<yafaray_Bool>(reinterpret_cast<yafaray::Scene *>(scene)->addInstanceObject(instance_id, base_object_id));
}

yafaray_Bool yafaray_addInstanceOfInstance(yafaray_Scene *scene, size_t instance_id, size_t base_instance_id)
{
	if(!scene) return YAFARAY_BOOL_FALSE;
	return static_cast<yafaray_Bool>(reinterpret_cast<yafaray::Scene *>(scene)->addInstanceOfInstance(instance_id, base_instance_id));
}

yafaray_Bool yafaray_addInstanceMatrix(yafaray_Scene *scene, size_t instance_id, double m_00, double m_01, double m_02, double m_03, double m_10, double m_11, double m_12, double m_13, double m_20, double m_21, double m_22, double m_23, double m_30, double m_31, double m_32, double m_33, float time)
{
	if(!scene) return YAFARAY_BOOL_FALSE;
	return static_cast<yafaray_Bool>(reinterpret_cast<yafaray::Scene *>(scene)->addInstanceMatrix(instance_id, yafaray::Matrix4f{std::array<std::array<float, 4>, 4>{{{static_cast<float>(m_00), static_cast<float>(m_01), static_cast<float>(m_02), static_cast<float>(m_03)}, {static_cast<float>(m_10), static_cast<float>(m_11), static_cast<float>(m_12), static_cast<float>(m_13)}, {static_cast<float>(m_20), static_cast<float>(m_21), static_cast<float>(m_22), static_cast<float>(m_23)}, {static_cast<float>(m_30), static_cast<float>(m_31), static_cast<float>(m_32), static_cast<float>(m_33)}}}}, time));
}

yafaray_Bool yafaray_addInstanceMatrixArray(yafaray_Scene *scene, size_t instance_id, const double *obj_to_world, float time)
{
	if(!scene) return YAFARAY_BOOL_FALSE;
	return static_cast<yafaray_Bool>(reinterpret_cast<yafaray::Scene *>(scene)->addInstanceMatrix(instance_id, yafaray::Matrix4f{obj_to_world}, time));
}

yafaray_ResultFlags yafaray_getObjectId(yafaray_Scene *scene, size_t *id_obtained, const char *name)
{
	if(!scene || !name) return YAFARAY_RESULT_ERROR_WHILE_CREATING;
	auto [object, object_id, object_result]{reinterpret_cast<yafaray::Scene *>(scene)->getObject(name)};
	if(id_obtained) *id_obtained = object_id;
	return static_cast<yafaray_ResultFlags>(object_result.value());
}

yafaray_ResultFlags yafaray_getMaterialId(yafaray_Scene *scene, size_t *id_obtained, const char *name)
{
	if(!scene || !name) return YAFARAY_RESULT_ERROR_WHILE_CREATING;
	auto [material_id, material_result]{reinterpret_cast<yafaray::Scene *>(scene)->getMaterial(name)};
	if(id_obtained) *id_obtained = material_id;
	return static_cast<yafaray_ResultFlags>(material_result.value());
}

yafaray_ResultFlags yafaray_createObject(yafaray_Scene *scene, size_t *id_obtained, const char *name, const yafaray_ParamMap *param_map)
{
	if(!scene || !name) return YAFARAY_RESULT_ERROR_WHILE_CREATING;
	auto creation_result{reinterpret_cast<yafaray::Scene *>(scene)->createObject(name, *reinterpret_cast<const yafaray::ParamMap *>(param_map))};
	if(id_obtained) *id_obtained = creation_result.first;
	return static_cast<yafaray_ResultFlags>(creation_result.second.flags_.value());
}

yafaray_ResultFlags yafaray_createLight(yafaray_Scene *scene, const char *name, const yafaray_ParamMap *param_map)
{
	if(!scene || !name) return YAFARAY_RESULT_ERROR_WHILE_CREATING;
	auto creation_result{reinterpret_cast<yafaray::Scene *>(scene)->createLight(name, *reinterpret_cast<const yafaray::ParamMap *>(param_map))};
	return static_cast<yafaray_ResultFlags>(creation_result.second.flags_.value());
}

yafaray_ResultFlags yafaray_createTexture(yafaray_Scene *scene, const char *name, const yafaray_ParamMap *param_map)
{
	if(!scene || !name) return YAFARAY_RESULT_ERROR_WHILE_CREATING;
	auto creation_result{reinterpret_cast<yafaray::Scene *>(scene)->createTexture(name, *reinterpret_cast<const yafaray::ParamMap *>(param_map))};
	return static_cast<yafaray_ResultFlags>(creation_result.second.flags_.value());
}

yafaray_ResultFlags yafaray_createMaterial(yafaray_Scene *scene, size_t *id_obtained, const char *name, const yafaray_ParamMap *param_map, const yafaray_ParamMapList *param_map_list_nodes)
{
	if(!scene || !name) return YAFARAY_RESULT_ERROR_WHILE_CREATING;
	auto creation_result{reinterpret_cast<yafaray::Scene *>(scene)->createMaterial(name, *reinterpret_cast<const yafaray::ParamMap *>(param_map), *reinterpret_cast<const std::list<yafaray::ParamMap> *>(param_map_list_nodes))};
	if(id_obtained) *id_obtained = creation_result.first;
	return static_cast<yafaray_ResultFlags>(creation_result.second.flags_.value());
}

yafaray_ResultFlags yafaray_defineBackground(yafaray_Scene *scene, const yafaray_ParamMap *param_map)
{
	if(!scene || !param_map) return YAFARAY_RESULT_ERROR_WHILE_CREATING;
	auto creation_result{reinterpret_cast<yafaray::Scene *>(scene)->defineBackground(*reinterpret_cast<const yafaray::ParamMap *>(param_map))};
	return static_cast<yafaray_ResultFlags>(creation_result.flags_.value());
}

yafaray_ResultFlags yafaray_createVolumeRegion(yafaray_Scene *scene, const char *name, const yafaray_ParamMap *param_map)
{
	if(!scene || !name || !param_map) return YAFARAY_RESULT_ERROR_WHILE_CREATING;
	auto creation_result{reinterpret_cast<yafaray::Scene *>(scene)->createVolumeRegion(name, *reinterpret_cast<const yafaray::ParamMap *>(param_map))};
	return static_cast<yafaray_ResultFlags>(creation_result.second.flags_.value());
}

yafaray_SceneModifiedFlags yafaray_checkAndClearSceneModifiedFlags(yafaray_Scene *scene)
{
	if(!scene) return YAFARAY_SCENE_MODIFIED_NOTHING;
	else return reinterpret_cast<yafaray::Scene *>(scene)->checkAndClearSceneModifiedFlags();
}

yafaray_Bool yafaray_preprocessScene(yafaray_Scene *scene, const yafaray_RenderControl *render_control, yafaray_SceneModifiedFlags scene_modified_flags)
{
	if(!render_control || !scene) return YAFARAY_BOOL_FALSE;
	return static_cast<yafaray_Bool>(reinterpret_cast<yafaray::Scene *>(scene)->preprocess(*reinterpret_cast<const yafaray::RenderControl *>(render_control), scene_modified_flags));
}

yafaray_ResultFlags yafaray_getImageId(yafaray_Scene *scene, const char *name, size_t *id_obtained)
{
	if(!scene || !name) return YAFARAY_RESULT_ERROR_WHILE_CREATING;
	auto [image, image_id, image_result]{reinterpret_cast<yafaray::Scene *>(scene)->getImage(name)};
	if(id_obtained) *id_obtained = image_id;
	return static_cast<yafaray_ResultFlags>(image_result.value());
}

yafaray_ResultFlags yafaray_createImage(yafaray_Scene *scene, const char *name, size_t *id_obtained, const yafaray_ParamMap *param_map)
{
	if(!scene || !name || !param_map) return YAFARAY_RESULT_ERROR_WHILE_CREATING;
	auto creation_result{reinterpret_cast<yafaray::Scene *>(scene)->createImage(name, *reinterpret_cast<const yafaray::ParamMap *>(param_map))};
	if(id_obtained) *id_obtained = creation_result.first;
	return static_cast<yafaray_ResultFlags>(creation_result.second.flags_.value());
}

yafaray_Bool yafaray_setImageColor(yafaray_Scene *scene, size_t image_id, int x, int y, float red, float green, float blue, float alpha)
{
	if(!scene) return YAFARAY_BOOL_FALSE;
	return static_cast<yafaray_Bool>(reinterpret_cast<yafaray::Scene *>(scene)->setImageColor(image_id, {{x, y}}, {red, green, blue, alpha}));
}

yafaray_Bool yafaray_getImageColor(yafaray_Scene *scene, size_t image_id, int x, int y, float *red, float *green, float *blue, float *alpha)
{
	if(!scene || !red || !green || !blue || !alpha) return YAFARAY_BOOL_FALSE;
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
	if(!scene) return 0;
	return static_cast<yafaray_Bool>(reinterpret_cast<yafaray::Scene *>(scene)->getImageSize(image_id).first[yafaray::Axis::X]);
}

int yafaray_getImageHeight(yafaray_Scene *scene, size_t image_id)
{
	if(!scene) return 0;
	return static_cast<yafaray_Bool>(reinterpret_cast<yafaray::Scene *>(scene)->getImageSize(image_id).first[yafaray::Axis::Y]);
}

