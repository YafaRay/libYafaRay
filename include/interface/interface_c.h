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

#include "constants.h"

typedef char bool_t;
bool_t TRUE = 1;
bool_t FALSE = 0;

#ifdef __cplusplus
extern "C" {
#endif
	struct yafaray4_ColorOutput;
	struct yafaray4_ProgressBar;

	LIBYAFARAY_EXPORT void yafaray4_createScene();
	LIBYAFARAY_EXPORT bool_t yafaray4_startGeometry(); //!< call before creating geometry; only meshes and vmaps can be created in this state
	LIBYAFARAY_EXPORT bool_t yafaray4_endGeometry(); //!< call after creating geometry;
	LIBYAFARAY_EXPORT unsigned int yafaray4_getNextFreeId();
	LIBYAFARAY_EXPORT bool_t yafaray4_endObject(); //!< end current mesh and return to geometry state
	int  yafaray4_addVertex(double x, double y, double z); //!< add vertex to mesh; returns index to be used for addTriangle
	int  yafaray4_addVertexWithOrco(double x, double y, double z, double ox, double oy, double oz); //!< add vertex with Orco to mesh; returns index to be used for addTriangle
	LIBYAFARAY_EXPORT void yafaray4_addNormal(double nx, double ny, double nz); //!< add vertex normal to mesh; the vertex that will be attached to is the last one inserted by addVertex method
	LIBYAFARAY_EXPORT bool_t yafaray4_addFace(int a, int b, int c); //!< add a triangle given vertex indices and material pointer
	LIBYAFARAY_EXPORT bool_t yafaray4_addFaceWithUv(int a, int b, int c, int uv_a, int uv_b, int uv_c); //!< add a triangle given vertex and uv indices and material pointer
	LIBYAFARAY_EXPORT int  yafaray4_addUv(float u, float v); //!< add a UV coordinate pair; returns index to be used for addTriangle
	LIBYAFARAY_EXPORT bool_t yafaray4_smoothMesh(const char *name, double angle); //!< smooth vertex normals of mesh with given ID and angle (in degrees)
	LIBYAFARAY_EXPORT bool_t yafaray4_addInstance(const char *base_object_name, const float obj_to_world[4][4]);
	// functions to build paramMaps instead of passing them from Blender
	// (decouling implementation details of STL containers, paraMap_t etc. as much as possible)
	LIBYAFARAY_EXPORT void yafaray4_paramsSetVector(const char *name, double x, double y, double z);
	LIBYAFARAY_EXPORT void yafaray4_paramsSetString(const char *name, const char *s);
	LIBYAFARAY_EXPORT void yafaray4_paramsSetBool(const char *name, bool_t b);
	LIBYAFARAY_EXPORT void yafaray4_paramsSetInt(const char *name, int i);
	LIBYAFARAY_EXPORT void yafaray4_paramsSetFloat(const char *name, double f);
	LIBYAFARAY_EXPORT void yafaray4_paramsSetColor(const char *name, float r, float g, float b, float a);
	LIBYAFARAY_EXPORT void yafaray4_paramsSetMatrix(const char *name, const float m[4][4], bool_t transpose);
	LIBYAFARAY_EXPORT void yafaray4_paramsClearAll(); 	//!< clear the paramMap and paramList
	LIBYAFARAY_EXPORT void yafaray4_paramsStartList(); //!< start writing parameters to the extended paramList (used by materials)
	LIBYAFARAY_EXPORT void yafaray4_paramsPushList(); 	//!< push new list item in paramList (e.g. new shader node description)
	LIBYAFARAY_EXPORT void yafaray4_paramsEndList(); 	//!< revert to writing to normal paramMap
	LIBYAFARAY_EXPORT void yafaray4_setCurrentMaterial(const char *name);
	LIBYAFARAY_EXPORT bool_t yafaray4_createObject(const char *name);
	LIBYAFARAY_EXPORT bool_t yafaray4_createLight(const char *name);
	LIBYAFARAY_EXPORT bool_t yafaray4_createTexture(const char *name);
	LIBYAFARAY_EXPORT bool_t yafaray4_createMaterial(const char *name);
	LIBYAFARAY_EXPORT bool_t yafaray4_createCamera(const char *name);
	LIBYAFARAY_EXPORT bool_t yafaray4_createBackground(const char *name);
	LIBYAFARAY_EXPORT bool_t yafaray4_createIntegrator(const char *name);
	LIBYAFARAY_EXPORT bool_t yafaray4_createVolumeRegion(const char *name);
	LIBYAFARAY_EXPORT bool_t yafaray4_createRenderView(const char *name);
	LIBYAFARAY_EXPORT bool_t yafaray4_createInternalOutput(const char *name, bool_t auto_delete); //!< ColorOutput creation, usually for internally-owned outputs that are destroyed when the scene is deleted or when libYafaRay instance is closed. If the client wants to keep ownership, it can set the "auto_delete" to false.
	LIBYAFARAY_EXPORT bool_t yafaray4_setExternalOutput(const char *name, struct yafaray4_ColorOutput *output, bool_t auto_delete); //!< ColorOutput creation, usually for externally client-owned and client-supplied outputs that are *NOT* destroyed when the scene is deleted or when libYafaRay instance is closed. If the client wants to transfer ownership to libYafaRay, it can set the "auto_delete" to true.
	LIBYAFARAY_EXPORT bool_t yafaray4_removeOutput(const char *name);
	LIBYAFARAY_EXPORT void yafaray4_clearOutputs();
	LIBYAFARAY_EXPORT void yafaray4_clearAll();
	LIBYAFARAY_EXPORT void yafaray4_render(struct yafaray4_ProgressBar *pb, bool_t auto_delete_progress_bar); //!< render the scene...
	LIBYAFARAY_EXPORT void yafaray4_defineLayer(const char *layer_type_name, const char *exported_image_type_name, const char *exported_image_name, const char *image_type_name);
	LIBYAFARAY_EXPORT bool_t yafaray4_setupLayersParameters();
	LIBYAFARAY_EXPORT void yafaray4_abort();
	LIBYAFARAY_EXPORT const char **yafaray4_getRenderParameters();

	LIBYAFARAY_EXPORT bool_t yafaray4_setInteractive(bool_t interactive);
	LIBYAFARAY_EXPORT void yafaray4_enablePrintDateTime(bool_t value);
	LIBYAFARAY_EXPORT void yafaray4_setConsoleVerbosityLevel(const char *str_v_level);
	LIBYAFARAY_EXPORT void yafaray4_setLogVerbosityLevel(const char *str_v_level);
	LIBYAFARAY_EXPORT char * yafaray4_getVersion(); //!< Get version to check against the exporters

	/*! Console Printing wrappers to report in color with yafaray's own console coloring */
	LIBYAFARAY_EXPORT void yafaray4_printDebug(const char *msg);
	LIBYAFARAY_EXPORT void yafaray4_printVerbose(const char *msg);
	LIBYAFARAY_EXPORT void yafaray4_printInfo(const char *msg);
	LIBYAFARAY_EXPORT void yafaray4_printParams(const char *msg);
	LIBYAFARAY_EXPORT void yafaray4_printWarning(const char *msg);
	LIBYAFARAY_EXPORT void yafaray4_printError(const char *msg);

	LIBYAFARAY_EXPORT void yafaray4_setInputColorSpace(const char *color_space_string, float gamma_val);
	LIBYAFARAY_EXPORT void yafaray4_free(void *ptr); //!< Free memory allocated by libYafaRay
#ifdef __cplusplus
};
#endif

#endif //YAFARAY_INTERFACE_C_H
