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

#ifndef LIBYAFARAY_YAFARAY_C_API_H
#define LIBYAFARAY_YAFARAY_C_API_H

#include "yafaray_c_api_export.h"
#include <stddef.h>

#define YAFARAY_C_API_VERSION_MAJOR 4

#ifdef __cplusplus
extern "C" {
#endif

/* Opaque structs/objects pointers */
typedef struct yafaray_Logger yafaray_Logger;
typedef struct yafaray_RenderControl yafaray_RenderControl;
typedef struct yafaray_RenderMonitor yafaray_RenderMonitor;
typedef struct yafaray_Scene yafaray_Scene;
typedef struct yafaray_ParamMap yafaray_ParamMap;
typedef struct yafaray_ParamMapList yafaray_ParamMapList;
typedef struct yafaray_SurfaceIntegrator yafaray_SurfaceIntegrator;
typedef struct yafaray_Film yafaray_Film;
typedef struct yafaray_Container yafaray_Container;

/* Basic enums */
typedef enum { YAFARAY_LOG_LEVEL_MUTE = 0, YAFARAY_LOG_LEVEL_ERROR, YAFARAY_LOG_LEVEL_WARNING, YAFARAY_LOG_LEVEL_PARAMS, YAFARAY_LOG_LEVEL_INFO, YAFARAY_LOG_LEVEL_VERBOSE, YAFARAY_LOG_LEVEL_DEBUG } yafaray_LogLevel;
typedef enum { YAFARAY_DISPLAY_CONSOLE_HIDDEN, YAFARAY_DISPLAY_CONSOLE_NORMAL } yafaray_DisplayConsole;
typedef enum { YAFARAY_CONTAINER_EXPORT_XML, YAFARAY_CONTAINER_EXPORT_C, YAFARAY_CONTAINER_EXPORT_PYTHON } yafaray_ContainerExportType;
typedef enum { YAFARAY_BOOL_FALSE = 0, YAFARAY_BOOL_TRUE = 1 } yafaray_Bool;
typedef enum
{
	YAFARAY_SCENE_MODIFIED_NOTHING = 0,
	YAFARAY_SCENE_MODIFIED_OBJECTS = 1 << 0,
	YAFARAY_SCENE_MODIFIED_LIGHTS = 1 << 1,
	YAFARAY_SCENE_MODIFIED_MATERIALS = 1 << 2,
	YAFARAY_SCENE_MODIFIED_TEXTURES = 1 << 3,
	YAFARAY_SCENE_MODIFIED_VOLUME_REGIONS = 1 << 4,
	YAFARAY_SCENE_MODIFIED_IMAGES = 1 << 5,
	YAFARAY_SCENE_MODIFIED_SCENE_ACCELERATOR_PARAMS = 1 << 6
} yafaray_SceneModifiedFlags;
typedef enum
{
	YAFARAY_RESULT_OK = 0,
	YAFARAY_RESULT_ERROR_TYPE_UNKNOWN_PARAM = 1 << 0,
	YAFARAY_RESULT_WARNING_UNKNOWN_PARAM = 1 << 1,
	YAFARAY_RESULT_WARNING_PARAM_NOT_SET = 1 << 2,
	YAFARAY_RESULT_ERROR_WRONG_PARAM_TYPE = 1 << 3,
	YAFARAY_RESULT_WARNING_UNKNOWN_ENUM_OPTION = 1 << 4,
	YAFARAY_RESULT_ERROR_ALREADY_EXISTS = 1 << 5,
	YAFARAY_RESULT_ERROR_WHILE_CREATING = 1 << 6,
	YAFARAY_RESULT_ERROR_NOT_FOUND = 1 << 7,
	YAFARAY_RESULT_WARNING_OVERWRITTEN = 1 << 8,
	YAFARAY_RESULT_ERROR_DUPLICATED_NAME = 1 << 9
} yafaray_ResultFlags;

/* Callback definitions for the C API - FIXME: Should we care about the function call convention being the same for libYafaRay and its client(s)? */
typedef void (*yafaray_FilmNotifyLayerCallback)(const char *internal_layer_name, const char *exported_layer_name, int width, int height, int exported_channels, void *callback_data);
typedef void (*yafaray_FilmPutPixelCallback)(const char *layer_name, int x, int y, float r, float g, float b, float a, void *callback_data);
typedef void (*yafaray_FilmFlushAreaCallback)(int area_id, int x_0, int y_0, int x_1, int y_1, void *callback_data);
typedef void (*yafaray_FilmFlushCallback)(void *callback_data);
typedef void (*yafaray_FilmHighlightAreaCallback)(int area_id, int x_0, int y_0, int x_1, int y_1, void *callback_data);
typedef void (*yafaray_FilmHighlightPixelCallback)(int x, int y, float r, float g, float b, float a, void *callback_data);
typedef void (*yafaray_ProgressBarCallback)(int steps_total, int steps_done, const char *tag, void *callback_data);
typedef void (*yafaray_LoggerCallback)(yafaray_LogLevel log_level, size_t datetime, const char *time_of_day, const char *description, void *callback_data);

/* C API Public functions.
 * In the source code the YafaRay developers *MUST* ensure that each of them appears in the Exported Symbols Map file with the correct annotated version */

/* Important note: in the functions that return a char string, memory is allocated by libYafaRay itself. Do not free the char* directly with free, use "yafaray_destroyCharString" to free them instead to ensure proper deallocation. */

/* General / version functions */
YAFARAY_C_API_EXPORT int yafaray_getVersionMajor();
YAFARAY_C_API_EXPORT int yafaray_getVersionMinor();
YAFARAY_C_API_EXPORT int yafaray_getVersionPatch();
YAFARAY_C_API_EXPORT char *yafaray_getVersionString();
YAFARAY_C_API_EXPORT void yafaray_destroyCharString(char *string);

/* Render Control functions */
YAFARAY_C_API_EXPORT yafaray_RenderControl *yafaray_createRenderControl();
YAFARAY_C_API_EXPORT void yafaray_destroyRenderControl(yafaray_RenderControl *render_control);
YAFARAY_C_API_EXPORT void yafaray_setRenderControlForNormalStart(yafaray_RenderControl *render_control);
YAFARAY_C_API_EXPORT void yafaray_setRenderControlForProgressiveStart(yafaray_RenderControl *render_control);
YAFARAY_C_API_EXPORT void yafaray_setRenderControlForResuming(yafaray_RenderControl *render_control);
YAFARAY_C_API_EXPORT void yafaray_cancelRendering(yafaray_RenderControl *render_control);

/* Render Monitor functions */
YAFARAY_C_API_EXPORT yafaray_RenderMonitor *yafaray_createRenderMonitor(yafaray_ProgressBarCallback monitor_callback, void *callback_data, yafaray_DisplayConsole progress_bar_display_console);
YAFARAY_C_API_EXPORT void yafaray_destroyRenderMonitor(yafaray_RenderMonitor *render_monitor);

/* Parameter Map functions */
YAFARAY_C_API_EXPORT yafaray_ParamMap *yafaray_createParamMap();
YAFARAY_C_API_EXPORT void yafaray_destroyParamMap(yafaray_ParamMap *param_map);
YAFARAY_C_API_EXPORT void yafaray_setParamMapVector(yafaray_ParamMap *param_map, const char *name, double x, double y, double z);
YAFARAY_C_API_EXPORT void yafaray_setParamMapString(yafaray_ParamMap *param_map, const char *name, const char *s);
YAFARAY_C_API_EXPORT void yafaray_setParamMapBool(yafaray_ParamMap *param_map, const char *name, yafaray_Bool b);
YAFARAY_C_API_EXPORT void yafaray_setParamMapInt(yafaray_ParamMap *param_map, const char *name, int i);
YAFARAY_C_API_EXPORT void yafaray_setParamMapFloat(yafaray_ParamMap *param_map, const char *name, double f);
YAFARAY_C_API_EXPORT void yafaray_setParamMapColor(yafaray_ParamMap *param_map, const char *name, double r, double g, double b, double a);
YAFARAY_C_API_EXPORT void yafaray_setParamMapMatrix(yafaray_ParamMap *param_map, const char *name, double m_00, double m_01, double m_02, double m_03, double m_10, double m_11, double m_12, double m_13, double m_20, double m_21, double m_22, double m_23, double m_30, double m_31, double m_32, double m_33, yafaray_Bool transpose);
YAFARAY_C_API_EXPORT void yafaray_setParamMapMatrixArray(yafaray_ParamMap *param_map, const char *name, const double *matrix, yafaray_Bool transpose);
YAFARAY_C_API_EXPORT void yafaray_clearParamMap(yafaray_ParamMap *param_map);
YAFARAY_C_API_EXPORT void yafaray_setInputColorSpace(yafaray_ParamMap *param_map, const char *color_space_string, float gamma_val);

/* Parameter Map List functions, for material shader nodes */
YAFARAY_C_API_EXPORT yafaray_ParamMapList *yafaray_createParamMapList();
YAFARAY_C_API_EXPORT void yafaray_destroyParamMapList(yafaray_ParamMapList *param_map_list);
YAFARAY_C_API_EXPORT void yafaray_addParamMapToList(yafaray_ParamMapList *param_map_list, const yafaray_ParamMap *param_map);
YAFARAY_C_API_EXPORT void yafaray_clearParamMapList(yafaray_ParamMapList *param_map_list);

/* Logger functions */
YAFARAY_C_API_EXPORT yafaray_Logger *yafaray_createLogger(const char *logger_name, yafaray_LoggerCallback logger_callback, void *callback_data, yafaray_DisplayConsole display_console);
YAFARAY_C_API_EXPORT void yafaray_destroyLogger(yafaray_Logger *logger);
YAFARAY_C_API_EXPORT void yafaray_enablePrintDateTime(yafaray_Logger *logger, yafaray_Bool value);
YAFARAY_C_API_EXPORT void yafaray_setConsoleVerbosityLevel(yafaray_Logger *logger, yafaray_LogLevel log_level);
YAFARAY_C_API_EXPORT void yafaray_setLogVerbosityLevel(yafaray_Logger *logger, yafaray_LogLevel log_level);
YAFARAY_C_API_EXPORT yafaray_LogLevel yafaray_logLevelFromString(const char *log_level_string);
YAFARAY_C_API_EXPORT void yafaray_printDebug(yafaray_Logger *logger, const char *msg);
YAFARAY_C_API_EXPORT void yafaray_printVerbose(yafaray_Logger *logger, const char *msg);
YAFARAY_C_API_EXPORT void yafaray_printInfo(yafaray_Logger *logger, const char *msg);
YAFARAY_C_API_EXPORT void yafaray_printParams(yafaray_Logger *logger, const char *msg);
YAFARAY_C_API_EXPORT void yafaray_printWarning(yafaray_Logger *logger, const char *msg);
YAFARAY_C_API_EXPORT void yafaray_printError(yafaray_Logger *logger, const char *msg);
YAFARAY_C_API_EXPORT void yafaray_setConsoleLogColorsEnabled(yafaray_Logger *logger, yafaray_Bool colors_enabled);
YAFARAY_C_API_EXPORT void yafaray_setLoggerCallbacks(yafaray_Logger *logger, yafaray_LoggerCallback logger_callback, void *callback_data);

/* Surface Integrator functions */
YAFARAY_C_API_EXPORT yafaray_SurfaceIntegrator *yafaray_createSurfaceIntegrator(yafaray_Logger *logger, const char *name, const yafaray_ParamMap *param_map);
YAFARAY_C_API_EXPORT void yafaray_destroySurfaceIntegrator(yafaray_SurfaceIntegrator *surface_integrator);
YAFARAY_C_API_EXPORT char *yafaray_getSurfaceIntegratorName(yafaray_SurfaceIntegrator *surface_integrator);
YAFARAY_C_API_EXPORT yafaray_ResultFlags yafaray_defineVolumeIntegrator(yafaray_SurfaceIntegrator *surface_integrator, const yafaray_Scene *scene, const yafaray_ParamMap *param_map);
YAFARAY_C_API_EXPORT void yafaray_preprocessSurfaceIntegrator(yafaray_RenderMonitor *render_monitor, yafaray_SurfaceIntegrator *surface_integrator, const yafaray_RenderControl *render_control, const yafaray_Scene *scene);
YAFARAY_C_API_EXPORT void yafaray_render(yafaray_RenderControl *render_control, yafaray_RenderMonitor *render_monitor, yafaray_SurfaceIntegrator *surface_integrator, yafaray_Film *film);

/* Film functions */
YAFARAY_C_API_EXPORT yafaray_Film *yafaray_createFilm(yafaray_Logger *logger, yafaray_SurfaceIntegrator *surface_integrator, const char *name, const yafaray_ParamMap *param_map);
YAFARAY_C_API_EXPORT void yafaray_destroyFilm(yafaray_Film *film);
YAFARAY_C_API_EXPORT char *yafaray_getFilmName(yafaray_Film *film);
YAFARAY_C_API_EXPORT int yafaray_getFilmWidth(const yafaray_Film *film);
YAFARAY_C_API_EXPORT int yafaray_getFilmHeight(const yafaray_Film *film);
YAFARAY_C_API_EXPORT yafaray_ResultFlags yafaray_defineCamera(yafaray_Film *film, const yafaray_ParamMap *param_map);
YAFARAY_C_API_EXPORT yafaray_ResultFlags yafaray_createOutput(yafaray_Film *film, const char *name, const yafaray_ParamMap *param_map);
YAFARAY_C_API_EXPORT void yafaray_defineLayer(yafaray_Film *film, const yafaray_ParamMap *param_map);
YAFARAY_C_API_EXPORT char *yafaray_getLayersTable(const yafaray_Film *film);
YAFARAY_C_API_EXPORT void yafaray_setNotifyLayerCallback(yafaray_Film *film, yafaray_FilmNotifyLayerCallback callback, void *callback_data);
YAFARAY_C_API_EXPORT void yafaray_setPutPixelCallback(yafaray_Film *film, yafaray_FilmPutPixelCallback callback, void *callback_data);
YAFARAY_C_API_EXPORT void yafaray_setHighlightPixelCallback(yafaray_Film *film, yafaray_FilmHighlightPixelCallback callback, void *callback_data);
YAFARAY_C_API_EXPORT void yafaray_setFlushAreaCallback(yafaray_Film *film, yafaray_FilmFlushAreaCallback callback, void *callback_data);
YAFARAY_C_API_EXPORT void yafaray_setFlushCallback(yafaray_Film *film, yafaray_FilmFlushCallback callback, void *callback_data);
YAFARAY_C_API_EXPORT void yafaray_setHighlightAreaCallback(yafaray_Film *film, yafaray_FilmHighlightAreaCallback callback, void *callback_data);

/* YafaRay Container functions */
YAFARAY_C_API_EXPORT yafaray_Container *yafaray_createContainer();
YAFARAY_C_API_EXPORT void yafaray_destroyContainerButNotContainedPointers(yafaray_Container *container);
YAFARAY_C_API_EXPORT void yafaray_destroyContainerAndContainedPointers(yafaray_Container *container);
YAFARAY_C_API_EXPORT void yafaray_addSceneToContainer(yafaray_Container *container, yafaray_Scene *scene);
YAFARAY_C_API_EXPORT void yafaray_addSurfaceIntegratorToContainer(yafaray_Container *container, yafaray_SurfaceIntegrator *surface_integrator);
YAFARAY_C_API_EXPORT void yafaray_addFilmToContainer(yafaray_Container *container, yafaray_Film *film);
YAFARAY_C_API_EXPORT size_t yafaray_numberOfScenesInContainer(const yafaray_Container *container);
YAFARAY_C_API_EXPORT size_t yafaray_numberOfSurfaceIntegratorsInContainer(const yafaray_Container *container);
YAFARAY_C_API_EXPORT size_t yafaray_numberOfFilmsInContainer(const yafaray_Container *container);
YAFARAY_C_API_EXPORT yafaray_Scene *yafaray_getSceneFromContainerByIndex(const yafaray_Container *container, size_t index);
YAFARAY_C_API_EXPORT yafaray_Scene *yafaray_getSceneFromContainerByName(const yafaray_Container *container, const char *name);
YAFARAY_C_API_EXPORT yafaray_SurfaceIntegrator *yafaray_getSurfaceIntegratorFromContainerByIndex(const yafaray_Container *container, size_t index);
YAFARAY_C_API_EXPORT yafaray_SurfaceIntegrator *yafaray_getSurfaceIntegratorFromContainerByName(const yafaray_Container *container, const char *name);
YAFARAY_C_API_EXPORT yafaray_Film *yafaray_getFilmFromContainerByIndex(const yafaray_Container *container, size_t index);
YAFARAY_C_API_EXPORT yafaray_Film *yafaray_getFilmFromContainerByName(const yafaray_Container *container, const char *name);
YAFARAY_C_API_EXPORT char *yafaray_exportContainerToString(const yafaray_Container *container, yafaray_ContainerExportType container_export_type, yafaray_Bool only_export_non_default_parameters);
YAFARAY_C_API_EXPORT yafaray_ResultFlags yafaray_exportContainerToFile(const yafaray_Container *container, yafaray_ContainerExportType container_export_type, yafaray_Bool only_export_non_default_parameters, const char *file_path);

/* Scene functions */
YAFARAY_C_API_EXPORT yafaray_Scene *yafaray_createScene(yafaray_Logger *logger, const char *name);
YAFARAY_C_API_EXPORT void yafaray_destroyScene(yafaray_Scene *scene);
YAFARAY_C_API_EXPORT char *yafaray_getSceneName(yafaray_Scene *scene);
YAFARAY_C_API_EXPORT void yafaray_setSceneAcceleratorParams(yafaray_Scene *scene, const yafaray_ParamMap *param_map);
YAFARAY_C_API_EXPORT yafaray_SceneModifiedFlags yafaray_checkAndClearSceneModifiedFlags(yafaray_Scene *scene);
YAFARAY_C_API_EXPORT yafaray_Bool yafaray_preprocessScene(yafaray_Scene *scene, const yafaray_RenderControl *render_control, yafaray_SceneModifiedFlags scene_modified_flags);
YAFARAY_C_API_EXPORT yafaray_ResultFlags yafaray_getMaterialId(yafaray_Scene *scene, size_t *id_obtained, const char *name);
YAFARAY_C_API_EXPORT yafaray_ResultFlags yafaray_createLight(yafaray_Scene *scene, const char *name, const yafaray_ParamMap *param_map);
YAFARAY_C_API_EXPORT yafaray_ResultFlags yafaray_createTexture(yafaray_Scene *scene, const char *name, const yafaray_ParamMap *param_map);
YAFARAY_C_API_EXPORT yafaray_ResultFlags yafaray_createMaterial(yafaray_Scene *scene, size_t *id_obtained, const char *name, const yafaray_ParamMap *param_map, const yafaray_ParamMapList *param_map_list_nodes);
YAFARAY_C_API_EXPORT yafaray_ResultFlags yafaray_defineBackground(yafaray_Scene *scene, const yafaray_ParamMap *param_map);
YAFARAY_C_API_EXPORT yafaray_ResultFlags yafaray_createVolumeRegion(yafaray_Scene *scene, const char *name, const yafaray_ParamMap *param_map);

/* Scene images */
YAFARAY_C_API_EXPORT yafaray_ResultFlags yafaray_createImage(yafaray_Scene *scene, const char *name, size_t *id_obtained, const yafaray_ParamMap *param_map);
YAFARAY_C_API_EXPORT yafaray_ResultFlags yafaray_getImageId(yafaray_Scene *scene, const char *name, size_t *id_obtained);
YAFARAY_C_API_EXPORT yafaray_Bool yafaray_setImageColor(yafaray_Scene *scene, size_t image_id, int x, int y, float red, float green, float blue, float alpha);
YAFARAY_C_API_EXPORT yafaray_Bool yafaray_getImageColor(yafaray_Scene *scene, size_t image_id, int x, int y, float *red, float *green, float *blue, float *alpha);
YAFARAY_C_API_EXPORT int yafaray_getImageWidth(yafaray_Scene *scene, size_t image_id);
YAFARAY_C_API_EXPORT int yafaray_getImageHeight(yafaray_Scene *scene, size_t image_id);

/* Scene objects */
YAFARAY_C_API_EXPORT yafaray_ResultFlags yafaray_createObject(yafaray_Scene *scene, size_t *id_obtained, const char *name, const yafaray_ParamMap *param_map);
YAFARAY_C_API_EXPORT yafaray_ResultFlags yafaray_getObjectId(yafaray_Scene *scene, size_t *id_obtained, const char *name);
YAFARAY_C_API_EXPORT yafaray_Bool yafaray_initObject(yafaray_Scene *scene, size_t object_id, size_t material_id);
YAFARAY_C_API_EXPORT size_t yafaray_addVertex(yafaray_Scene *scene, size_t object_id, double x, double y, double z);
YAFARAY_C_API_EXPORT size_t yafaray_addVertexTimeStep(yafaray_Scene *scene, size_t object_id, double x, double y, double z, unsigned char time_step);
YAFARAY_C_API_EXPORT size_t yafaray_addVertexWithOrco(yafaray_Scene *scene, size_t object_id, double x, double y, double z, double ox, double oy, double oz);
YAFARAY_C_API_EXPORT size_t yafaray_addVertexWithOrcoTimeStep(yafaray_Scene *scene, size_t object_id, double x, double y, double z, double ox, double oy, double oz, unsigned char time_step);
YAFARAY_C_API_EXPORT void yafaray_addNormal(yafaray_Scene *scene, size_t object_id, double nx, double ny, double nz);
YAFARAY_C_API_EXPORT void yafaray_addNormalTimeStep(yafaray_Scene *scene, size_t object_id, double nx, double ny, double nz, unsigned char time_step);
YAFARAY_C_API_EXPORT yafaray_Bool yafaray_addTriangle(yafaray_Scene *scene, size_t object_id, size_t a, size_t b, size_t c, size_t material_id);
YAFARAY_C_API_EXPORT yafaray_Bool yafaray_addTriangleWithUv(yafaray_Scene *scene, size_t object_id, size_t a, size_t b, size_t c, size_t uv_a, size_t uv_b, size_t uv_c, size_t material_id);
YAFARAY_C_API_EXPORT yafaray_Bool yafaray_addQuad(yafaray_Scene *scene, size_t object_id, size_t a, size_t b, size_t c, size_t d, size_t material_id);
YAFARAY_C_API_EXPORT yafaray_Bool yafaray_addQuadWithUv(yafaray_Scene *scene, size_t object_id, size_t a, size_t b, size_t c, size_t d, size_t uv_a, size_t uv_b, size_t uv_c, size_t uv_d, size_t material_id);
YAFARAY_C_API_EXPORT size_t yafaray_addUv(yafaray_Scene *scene, size_t object_id, double u, double v);
YAFARAY_C_API_EXPORT yafaray_Bool yafaray_smoothObjectMesh(yafaray_Scene *scene, size_t object_id, double angle);

/* Scene instances */
YAFARAY_C_API_EXPORT size_t yafaray_createInstance(yafaray_Scene *scene);
YAFARAY_C_API_EXPORT yafaray_Bool yafaray_addInstanceObject(yafaray_Scene *scene, size_t instance_id, size_t base_object_id);
YAFARAY_C_API_EXPORT yafaray_Bool yafaray_addInstanceOfInstance(yafaray_Scene *scene, size_t instance_id, size_t base_instance_id);
YAFARAY_C_API_EXPORT yafaray_Bool yafaray_addInstanceMatrix(yafaray_Scene *scene, size_t instance_id, double m_00, double m_01, double m_02, double m_03, double m_10, double m_11, double m_12, double m_13, double m_20, double m_21, double m_22, double m_23, double m_30, double m_31, double m_32, double m_33, float time);
YAFARAY_C_API_EXPORT yafaray_Bool yafaray_addInstanceMatrixArray(yafaray_Scene *scene, size_t instance_id, const double *obj_to_world, float time);

#ifdef __cplusplus
}
#endif

#endif /* LIBYAFARAY_YAFARAY_C_API_H */
