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

extern "C"
{
	struct libYafaRay4_ColorOutput;
	struct libYafaRay4_ProgressBar;

	void libYafaRay4_createScene();
	bool libYafaRay4_startGeometry(); //!< call before creating geometry; only meshes and vmaps can be created in this state
	bool libYafaRay4_endGeometry(); //!< call after creating geometry;
	unsigned int libYafaRay4_getNextFreeId();
	bool libYafaRay4_endObject(); //!< end current mesh and return to geometry state
	int  libYafaRay4_addVertex(double x, double y, double z); //!< add vertex to mesh; returns index to be used for addTriangle
	int  libYafaRay4_addVertexWithOrco(double x, double y, double z, double ox, double oy, double oz); //!< add vertex with Orco to mesh; returns index to be used for addTriangle
	void libYafaRay4_addNormal(double nx, double ny, double nz); //!< add vertex normal to mesh; the vertex that will be attached to is the last one inserted by addVertex method
	bool libYafaRay4_addFace(int a, int b, int c); //!< add a triangle given vertex indices and material pointer
	bool libYafaRay4_addFaceWithUv(int a, int b, int c, int uv_a, int uv_b, int uv_c); //!< add a triangle given vertex and uv indices and material pointer
	int  libYafaRay4_addUv(float u, float v); //!< add a UV coordinate pair; returns index to be used for addTriangle
	bool libYafaRay4_smoothMesh(const char *name, double angle); //!< smooth vertex normals of mesh with given ID and angle (in degrees)
	bool libYafaRay4_addInstance(const char *base_object_name, const float obj_to_world[4][4]);
	// functions to build paramMaps instead of passing them from Blender
	// (decouling implementation details of STL containers, paraMap_t etc. as much as possible)
	void libYafaRay4_paramsSetVector(const char *name, double x, double y, double z);
	void libYafaRay4_paramsSetString(const char *name, const char *s);
	void libYafaRay4_paramsSetBool(const char *name, bool b);
	void libYafaRay4_paramsSetInt(const char *name, int i);
	void libYafaRay4_paramsSetFloat(const char *name, double f);
	void libYafaRay4_paramsSetColor(const char *name, float r, float g, float b, float a = 1.f);
	void libYafaRay4_paramsSetMatrix(const char *name, const float m[4][4], bool transpose = false);
	void libYafaRay4_paramsClearAll(); 	//!< clear the paramMap and paramList
	void libYafaRay4_paramsStartList(); //!< start writing parameters to the extended paramList (used by materials)
	void libYafaRay4_paramsPushList(); 	//!< push new list item in paramList (e.g. new shader node description)
	void libYafaRay4_paramsEndList(); 	//!< revert to writing to normal paramMap
	void libYafaRay4_setCurrentMaterial(const char *name);
	bool libYafaRay4_createObject(const char *name);
	bool libYafaRay4_createLight(const char *name);
	bool libYafaRay4_createTexture(const char *name);
	bool libYafaRay4_createMaterial(const char *name);
	bool libYafaRay4_createCamera(const char *name);
	bool libYafaRay4_createBackground(const char *name);
	bool libYafaRay4_createIntegrator(const char *name);
	bool libYafaRay4_createVolumeRegion(const char *name);
	bool libYafaRay4_createRenderView(const char *name);
	bool libYafaRay4_createInternalOutput(const char *name, bool auto_delete = true); //!< ColorOutput creation, usually for internally-owned outputs that are destroyed when the scene is deleted or when libYafaRay instance is closed. If the client wants to keep ownership, it can set the "auto_delete" to false.
	bool libYafaRay4_setExternalOutput(const char *name, libYafaRay4_ColorOutput *output, bool auto_delete = false); //!< ColorOutput creation, usually for externally client-owned and client-supplied outputs that are *NOT* destroyed when the scene is deleted or when libYafaRay instance is closed. If the client wants to transfer ownership to libYafaRay, it can set the "auto_delete" to true.
	bool libYafaRay4_removeOutput(const char *name);
	void libYafaRay4_clearOutputs();
	void libYafaRay4_clearAll();
	void libYafaRay4_render(libYafaRay4_ProgressBar *pb = nullptr, bool auto_delete_progress_bar = false); //!< render the scene...
	void libYafaRay4_defineLayer(const char *layer_type_name, const char *exported_image_type_name, const char *exported_image_name, const char *image_type_name = "");
	bool libYafaRay4_setupLayersParameters();
	void libYafaRay4_abort();
	const char **libYafaRay4_getRenderParameters();

	bool libYafaRay4_setInteractive(bool interactive);
	void libYafaRay4_enablePrintDateTime(bool value);
	void libYafaRay4_setConsoleVerbosityLevel(const char *str_v_level);
	void libYafaRay4_setLogVerbosityLevel(const char *str_v_level);
	const char *libYafaRay4_getVersion(); //!< Get version to check against the exporters

	/*! Console Printing wrappers to report in color with yafaray's own console coloring */
	void libYafaRay4_printDebug(const char *msg);
	void libYafaRay4_printVerbose(const char *msg);
	void libYafaRay4_printInfo(const char *msg);
	void libYafaRay4_printParams(const char *msg);
	void libYafaRay4_printWarning(const char *msg);
	void libYafaRay4_printError(const char *msg);

	void libYafaRay4_setInputColorSpace(const char *color_space_string, float gamma_val);
};

#endif //YAFARAY_INTERFACE_C_H
