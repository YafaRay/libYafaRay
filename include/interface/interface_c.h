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

#ifndef YAFARAY_INTERFACE_C_H
#define YAFARAY_INTERFACE_C_H

#include <stddef.h>
#include "constants.h"

typedef char bool_t;
bool_t TRUE = 1;
bool_t FALSE = 0;

#ifdef __cplusplus
extern "C" {
#endif
	struct yafaray4_Interface;
	typedef struct yafaray4_Interface yaf4_Interface_t;
	struct yafaray4_ColorOutput;
	struct yafaray4_ProgressBar;

	LIBYAFARAY_EXPORT yaf4_Interface_t *yafaray4_createInterface(const char *interface_type);
	LIBYAFARAY_EXPORT void yafaray4_destroyInterface(yaf4_Interface_t *interface);
	LIBYAFARAY_EXPORT void yafaray4_createScene(yaf4_Interface_t *interface);
	LIBYAFARAY_EXPORT bool_t yafaray4_startGeometry(yaf4_Interface_t *interface); //!< call before creating geometry; only meshes and vmaps can be created in this state
	LIBYAFARAY_EXPORT bool_t yafaray4_endGeometry(yaf4_Interface_t *interface); //!< call after creating geometry;
	LIBYAFARAY_EXPORT unsigned int yafaray4_getNextFreeId(yaf4_Interface_t *interface);
	LIBYAFARAY_EXPORT bool_t yafaray4_endObject(yaf4_Interface_t *interface); //!< end current mesh and return to geometry state
	LIBYAFARAY_EXPORT int yafaray4_addVertex(yaf4_Interface_t *interface, double x, double y, double z); //!< add vertex to mesh; returns index to be used for addTriangle
	LIBYAFARAY_EXPORT int yafaray4_addVertexWithOrco(yaf4_Interface_t *interface, double x, double y, double z, double ox, double oy, double oz); //!< add vertex with Orco to mesh; returns index to be used for addTriangle
	LIBYAFARAY_EXPORT void yafaray4_addNormal(yaf4_Interface_t *interface, double nx, double ny, double nz); //!< add vertex normal to mesh; the vertex that will be attached to is the last one inserted by addVertex method
	LIBYAFARAY_EXPORT bool_t yafaray4_addFace(yaf4_Interface_t *interface, int a, int b, int c); //!< add a triangle given vertex indices and material pointer
	LIBYAFARAY_EXPORT bool_t yafaray4_addFaceWithUv(yaf4_Interface_t *interface, int a, int b, int c, int uv_a, int uv_b, int uv_c); //!< add a triangle given vertex and uv indices and material pointer
	LIBYAFARAY_EXPORT int  yafaray4_addUv(yaf4_Interface_t *interface, float u, float v); //!< add a UV coordinate pair; returns index to be used for addTriangle
	LIBYAFARAY_EXPORT bool_t yafaray4_smoothMesh(yaf4_Interface_t *interface, const char *name, double angle); //!< smooth vertex normals of mesh with given ID and angle (in degrees)
	LIBYAFARAY_EXPORT bool_t yafaray4_addInstance(yaf4_Interface_t *interface, const char *base_object_name, const float obj_to_world[4][4]);
	// functions to build paramMaps instead of passing them from Blender
	// (decouling implementation details of STL containers, paraMap_t etc. as much as possible)
	LIBYAFARAY_EXPORT void yafaray4_paramsSetVector(yaf4_Interface_t *interface, const char *name, double x, double y, double z);
	LIBYAFARAY_EXPORT void yafaray4_paramsSetString(yaf4_Interface_t *interface, const char *name, const char *s);
	LIBYAFARAY_EXPORT void yafaray4_paramsSetBool(yaf4_Interface_t *interface, const char *name, bool_t b);
	LIBYAFARAY_EXPORT void yafaray4_paramsSetInt(yaf4_Interface_t *interface, const char *name, int i);
	LIBYAFARAY_EXPORT void yafaray4_paramsSetFloat(yaf4_Interface_t *interface, const char *name, double f);
	LIBYAFARAY_EXPORT void yafaray4_paramsSetColor(yaf4_Interface_t *interface, const char *name, float r, float g, float b, float a);
	LIBYAFARAY_EXPORT void yafaray4_paramsSetMatrix(yaf4_Interface_t *interface, const char *name, const float m[4][4], bool_t transpose);
	LIBYAFARAY_EXPORT void yafaray4_paramsClearAll(yaf4_Interface_t *interface); 	//!< clear the paramMap and paramList
	LIBYAFARAY_EXPORT void yafaray4_paramsPushList(yaf4_Interface_t *interface); 	//!< push new list item in paramList (e.g. new shader node description)
	LIBYAFARAY_EXPORT void yafaray4_paramsEndList(yaf4_Interface_t *interface); 	//!< revert to writing to normal paramMap
	LIBYAFARAY_EXPORT void yafaray4_setCurrentMaterial(yaf4_Interface_t *interface, const char *name);
	LIBYAFARAY_EXPORT bool_t yafaray4_createObject(yaf4_Interface_t *interface, const char *name);
	LIBYAFARAY_EXPORT bool_t yafaray4_createLight(yaf4_Interface_t *interface, const char *name);
	LIBYAFARAY_EXPORT bool_t yafaray4_createTexture(yaf4_Interface_t *interface, const char *name);
	LIBYAFARAY_EXPORT bool_t yafaray4_createMaterial(yaf4_Interface_t *interface, const char *name);
	LIBYAFARAY_EXPORT bool_t yafaray4_createCamera(yaf4_Interface_t *interface, const char *name);
	LIBYAFARAY_EXPORT bool_t yafaray4_createBackground(yaf4_Interface_t *interface, const char *name);
	LIBYAFARAY_EXPORT bool_t yafaray4_createIntegrator(yaf4_Interface_t *interface, const char *name);
	LIBYAFARAY_EXPORT bool_t yafaray4_createVolumeRegion(yaf4_Interface_t *interface, const char *name);
	LIBYAFARAY_EXPORT bool_t yafaray4_createRenderView(yaf4_Interface_t *interface, const char *name);
	LIBYAFARAY_EXPORT bool_t yafaray4_createInternalOutput(yaf4_Interface_t *interface, const char *name, bool_t auto_delete); //!< ColorOutput creation, usually for internally-owned outputs that are destroyed when the scene is deleted or when libYafaRay instance is closed. If the client wants to keep ownership, it can set the "auto_delete" to false.
	LIBYAFARAY_EXPORT bool_t yafaray4_setExternalOutput(yaf4_Interface_t *interface, const char *name, struct yafaray4_ColorOutput *output, bool_t auto_delete); //!< ColorOutput creation, usually for externally client-owned and client-supplied outputs that are *NOT* destroyed when the scene is deleted or when libYafaRay instance is closed. If the client wants to transfer ownership to libYafaRay, it can set the "auto_delete" to true.
	LIBYAFARAY_EXPORT bool_t yafaray4_removeOutput(yaf4_Interface_t *interface, const char *name);
	LIBYAFARAY_EXPORT void yafaray4_clearOutputs(yaf4_Interface_t *interface);
	LIBYAFARAY_EXPORT void yafaray4_clearAll(yaf4_Interface_t *interface);
	LIBYAFARAY_EXPORT void yafaray4_render(yaf4_Interface_t *interface, struct yafaray4_ProgressBar *pb, bool_t auto_delete_progress_bar); //!< render the scene...
	LIBYAFARAY_EXPORT void yafaray4_defineLayer(yaf4_Interface_t *interface, const char *layer_type_name, const char *exported_image_type_name, const char *exported_image_name, const char *image_type_name);
	LIBYAFARAY_EXPORT bool_t yafaray4_setupLayersParameters(yaf4_Interface_t *interface);
	LIBYAFARAY_EXPORT void yafaray4_abort(yaf4_Interface_t *interface);
	LIBYAFARAY_EXPORT const char **yafaray4_getRenderParameters(yaf4_Interface_t *interface);

	LIBYAFARAY_EXPORT bool_t yafaray4_setInteractive(yaf4_Interface_t *interface, bool_t interactive);
	LIBYAFARAY_EXPORT void yafaray4_enablePrintDateTime(yaf4_Interface_t *interface, bool_t value);
	LIBYAFARAY_EXPORT void yafaray4_setConsoleVerbosityLevel(yaf4_Interface_t *interface, const char *str_v_level);
	LIBYAFARAY_EXPORT void yafaray4_setLogVerbosityLevel(yaf4_Interface_t *interface, const char *str_v_level);
	LIBYAFARAY_EXPORT void yafaray4_getVersion(yaf4_Interface_t *interface, char *dest_string, size_t dest_string_size); //!< Get version to check against the exporters

	/*! Console Printing wrappers to report in color with yafaray's own console coloring */
	LIBYAFARAY_EXPORT void yafaray4_printDebug(yaf4_Interface_t *interface, const char *msg);
	LIBYAFARAY_EXPORT void yafaray4_printVerbose(yaf4_Interface_t *interface, const char *msg);
	LIBYAFARAY_EXPORT void yafaray4_printInfo(yaf4_Interface_t *interface, const char *msg);
	LIBYAFARAY_EXPORT void yafaray4_printParams(yaf4_Interface_t *interface, const char *msg);
	LIBYAFARAY_EXPORT void yafaray4_printWarning(yaf4_Interface_t *interface, const char *msg);
	LIBYAFARAY_EXPORT void yafaray4_printError(yaf4_Interface_t *interface, const char *msg);

	LIBYAFARAY_EXPORT void yafaray4_setInputColorSpace(const char *color_space_string, float gamma_val);
	LIBYAFARAY_EXPORT void yafaray4_free(void *ptr); //!< Free memory allocated by libYafaRay
#ifdef __cplusplus
};
#endif

#endif //YAFARAY_INTERFACE_C_H
