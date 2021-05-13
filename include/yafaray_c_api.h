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

#ifndef YAFARAY_C_API_H
#define YAFARAY_C_API_H

#include "yafaray_conf.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif
	typedef struct yafaray4_Interface yafaray4_Interface_t;
	typedef struct yafaray4_ColorOutput yafaray4_ColorOutput_t;
	typedef struct yafaray4_ProgressBar yafaray4_ProgressBar_t;

	typedef enum { YAFARAY_INTERFACE_FOR_RENDERING, YAFARAY_INTERFACE_EXPORT_XML } yafaray4_Interface_Type_t;
	typedef enum { YAFARAY_BOOL_FALSE = 0, YAFARAY_BOOL_TRUE = 1} yafaray4_bool_t;

	LIBYAFARAY_EXPORT yafaray4_Interface_t *yafaray4_createInterface(yafaray4_Interface_Type_t interface_type, const char *exported_file_path);
	LIBYAFARAY_EXPORT void yafaray4_destroyInterface(yafaray4_Interface_t *interface);
	LIBYAFARAY_EXPORT void yafaray4_createScene(yafaray4_Interface_t *interface);
	LIBYAFARAY_EXPORT yafaray4_bool_t yafaray4_startGeometry(yafaray4_Interface_t *interface); //!< call before creating geometry; only meshes and vmaps can be created in this state
	LIBYAFARAY_EXPORT yafaray4_bool_t yafaray4_endGeometry(yafaray4_Interface_t *interface); //!< call after creating geometry;
	LIBYAFARAY_EXPORT unsigned int yafaray4_getNextFreeId(yafaray4_Interface_t *interface);
	LIBYAFARAY_EXPORT yafaray4_bool_t yafaray4_endObject(yafaray4_Interface_t *interface); //!< end current mesh and return to geometry state
	LIBYAFARAY_EXPORT int yafaray4_addVertex(yafaray4_Interface_t *interface, double x, double y, double z); //!< add vertex to mesh; returns index to be used for addTriangle
	LIBYAFARAY_EXPORT int yafaray4_addVertexWithOrco(yafaray4_Interface_t *interface, double x, double y, double z, double ox, double oy, double oz); //!< add vertex with Orco to mesh; returns index to be used for addTriangle
	LIBYAFARAY_EXPORT void yafaray4_addNormal(yafaray4_Interface_t *interface, double nx, double ny, double nz); //!< add vertex normal to mesh; the vertex that will be attached to is the last one inserted by addVertex method
	LIBYAFARAY_EXPORT yafaray4_bool_t yafaray4_addFace(yafaray4_Interface_t *interface, int a, int b, int c); //!< add a triangle given vertex indices and material pointer
	LIBYAFARAY_EXPORT yafaray4_bool_t yafaray4_addFaceWithUv(yafaray4_Interface_t *interface, int a, int b, int c, int uv_a, int uv_b, int uv_c); //!< add a triangle given vertex and uv indices and material pointer
	LIBYAFARAY_EXPORT int  yafaray4_addUv(yafaray4_Interface_t *interface, float u, float v); //!< add a UV coordinate pair; returns index to be used for addTriangle
	LIBYAFARAY_EXPORT yafaray4_bool_t yafaray4_smoothMesh(yafaray4_Interface_t *interface, const char *name, double angle); //!< smooth vertex normals of mesh with given ID and angle (in degrees)
	LIBYAFARAY_EXPORT yafaray4_bool_t yafaray4_addInstance(yafaray4_Interface_t *interface, const char *base_object_name, const float obj_to_world[4][4]);
	// functions to build paramMaps instead of passing them from Blender
	// (decouling implementation details of STL containers, paraMap_t etc. as much as possible)
	LIBYAFARAY_EXPORT void yafaray4_paramsSetVector(yafaray4_Interface_t *interface, const char *name, double x, double y, double z);
	LIBYAFARAY_EXPORT void yafaray4_paramsSetString(yafaray4_Interface_t *interface, const char *name, const char *s);
	LIBYAFARAY_EXPORT void yafaray4_paramsSetBool(yafaray4_Interface_t *interface, const char *name, yafaray4_bool_t b);
	LIBYAFARAY_EXPORT void yafaray4_paramsSetInt(yafaray4_Interface_t *interface, const char *name, int i);
	LIBYAFARAY_EXPORT void yafaray4_paramsSetFloat(yafaray4_Interface_t *interface, const char *name, double f);
	LIBYAFARAY_EXPORT void yafaray4_paramsSetColor(yafaray4_Interface_t *interface, const char *name, float r, float g, float b, float a);
	LIBYAFARAY_EXPORT void yafaray4_paramsSetMatrix(yafaray4_Interface_t *interface, const char *name, const float m[4][4], yafaray4_bool_t transpose);
	LIBYAFARAY_EXPORT void yafaray4_paramsClearAll(yafaray4_Interface_t *interface); 	//!< clear the paramMap and paramList
	LIBYAFARAY_EXPORT void yafaray4_paramsPushList(yafaray4_Interface_t *interface); 	//!< push new list item in paramList (e.g. new shader node description)
	LIBYAFARAY_EXPORT void yafaray4_paramsEndList(yafaray4_Interface_t *interface); 	//!< revert to writing to normal paramMap
	LIBYAFARAY_EXPORT void yafaray4_setCurrentMaterial(yafaray4_Interface_t *interface, const char *name);
	LIBYAFARAY_EXPORT yafaray4_bool_t yafaray4_createObject(yafaray4_Interface_t *interface, const char *name);
	LIBYAFARAY_EXPORT yafaray4_bool_t yafaray4_createLight(yafaray4_Interface_t *interface, const char *name);
	LIBYAFARAY_EXPORT yafaray4_bool_t yafaray4_createTexture(yafaray4_Interface_t *interface, const char *name);
	LIBYAFARAY_EXPORT yafaray4_bool_t yafaray4_createMaterial(yafaray4_Interface_t *interface, const char *name);
	LIBYAFARAY_EXPORT yafaray4_bool_t yafaray4_createCamera(yafaray4_Interface_t *interface, const char *name);
	LIBYAFARAY_EXPORT yafaray4_bool_t yafaray4_createBackground(yafaray4_Interface_t *interface, const char *name);
	LIBYAFARAY_EXPORT yafaray4_bool_t yafaray4_createIntegrator(yafaray4_Interface_t *interface, const char *name);
	LIBYAFARAY_EXPORT yafaray4_bool_t yafaray4_createVolumeRegion(yafaray4_Interface_t *interface, const char *name);
	LIBYAFARAY_EXPORT yafaray4_bool_t yafaray4_createRenderView(yafaray4_Interface_t *interface, const char *name);
	LIBYAFARAY_EXPORT yafaray4_bool_t yafaray4_createInternalOutput(yafaray4_Interface_t *interface, const char *name, yafaray4_bool_t auto_delete); //!< ColorOutput creation, usually for internally-owned outputs that are destroyed when the scene is deleted or when libYafaRay instance is closed. If the client wants to keep ownership, it can set the "auto_delete" to false.
	LIBYAFARAY_EXPORT yafaray4_bool_t yafaray4_setExternalOutput(yafaray4_Interface_t *interface, const char *name, yafaray4_ColorOutput_t *output, yafaray4_bool_t auto_delete); //!< ColorOutput creation, usually for externally client-owned and client-supplied outputs that are *NOT* destroyed when the scene is deleted or when libYafaRay instance is closed. If the client wants to transfer ownership to libYafaRay, it can set the "auto_delete" to true.
	LIBYAFARAY_EXPORT yafaray4_bool_t yafaray4_removeOutput(yafaray4_Interface_t *interface, const char *name);
	LIBYAFARAY_EXPORT void yafaray4_clearOutputs(yafaray4_Interface_t *interface);
	LIBYAFARAY_EXPORT void yafaray4_clearAll(yafaray4_Interface_t *interface);
	LIBYAFARAY_EXPORT void yafaray4_render(yafaray4_Interface_t *interface, yafaray4_ProgressBar_t *pb); //!< render the scene...
	LIBYAFARAY_EXPORT void yafaray4_defineLayer(yafaray4_Interface_t *interface, const char *layer_type_name, const char *exported_image_type_name, const char *exported_image_name, const char *image_type_name);
	LIBYAFARAY_EXPORT yafaray4_bool_t yafaray4_setupLayersParameters(yafaray4_Interface_t *interface);
	LIBYAFARAY_EXPORT void yafaray4_cancel(yafaray4_Interface_t *interface);

	LIBYAFARAY_EXPORT yafaray4_bool_t yafaray4_setInteractive(yafaray4_Interface_t *interface, yafaray4_bool_t interactive);
	LIBYAFARAY_EXPORT void yafaray4_enablePrintDateTime(yafaray4_Interface_t *interface, yafaray4_bool_t value);
	LIBYAFARAY_EXPORT void yafaray4_setConsoleVerbosityLevel(yafaray4_Interface_t *interface, const char *str_v_level);
	LIBYAFARAY_EXPORT void yafaray4_setLogVerbosityLevel(yafaray4_Interface_t *interface, const char *str_v_level);
	LIBYAFARAY_EXPORT void yafaray4_getVersion(yafaray4_Interface_t *interface, char *dest_string, size_t dest_string_size); //!< Get version to check against the exporters

	/*! Console Printing wrappers to report in color with yafaray's own console coloring */
	LIBYAFARAY_EXPORT void yafaray4_printDebug(yafaray4_Interface_t *interface, const char *msg);
	LIBYAFARAY_EXPORT void yafaray4_printVerbose(yafaray4_Interface_t *interface, const char *msg);
	LIBYAFARAY_EXPORT void yafaray4_printInfo(yafaray4_Interface_t *interface, const char *msg);
	LIBYAFARAY_EXPORT void yafaray4_printParams(yafaray4_Interface_t *interface, const char *msg);
	LIBYAFARAY_EXPORT void yafaray4_printWarning(yafaray4_Interface_t *interface, const char *msg);
	LIBYAFARAY_EXPORT void yafaray4_printError(yafaray4_Interface_t *interface, const char *msg);

	LIBYAFARAY_EXPORT void yafaray4_setInputColorSpace(const char *color_space_string, float gamma_val);
	LIBYAFARAY_EXPORT void yafaray4_free(void *ptr); //!< Free memory allocated by libYafaRay
#ifdef __cplusplus
}
#endif

#endif //YAFARAY_C_API_H
