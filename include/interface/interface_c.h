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
	struct yafaray4_ColorOutput;
	struct yafaray4_ProgressBar;

	void yafaray4_createScene();
	bool yafaray4_startGeometry(); //!< call before creating geometry; only meshes and vmaps can be created in this state
	bool yafaray4_endGeometry(); //!< call after creating geometry;
	unsigned int yafaray4_getNextFreeId();
	bool yafaray4_endObject(); //!< end current mesh and return to geometry state
	int  yafaray4_addVertex(double x, double y, double z); //!< add vertex to mesh; returns index to be used for addTriangle
	int  yafaray4_addVertexWithOrco(double x, double y, double z, double ox, double oy, double oz); //!< add vertex with Orco to mesh; returns index to be used for addTriangle
	void yafaray4_addNormal(double nx, double ny, double nz); //!< add vertex normal to mesh; the vertex that will be attached to is the last one inserted by addVertex method
	bool yafaray4_addFace(int a, int b, int c); //!< add a triangle given vertex indices and material pointer
	bool yafaray4_addFaceWithUv(int a, int b, int c, int uv_a, int uv_b, int uv_c); //!< add a triangle given vertex and uv indices and material pointer
	int  yafaray4_addUv(float u, float v); //!< add a UV coordinate pair; returns index to be used for addTriangle
	bool yafaray4_smoothMesh(const char *name, double angle); //!< smooth vertex normals of mesh with given ID and angle (in degrees)
	bool yafaray4_addInstance(const char *base_object_name, const float obj_to_world[4][4]);
	// functions to build paramMaps instead of passing them from Blender
	// (decouling implementation details of STL containers, paraMap_t etc. as much as possible)
	void yafaray4_paramsSetVector(const char *name, double x, double y, double z);
	void yafaray4_paramsSetString(const char *name, const char *s);
	void yafaray4_paramsSetBool(const char *name, bool b);
	void yafaray4_paramsSetInt(const char *name, int i);
	void yafaray4_paramsSetFloat(const char *name, double f);
	void yafaray4_paramsSetColor(const char *name, float r, float g, float b, float a = 1.f);
	void yafaray4_paramsSetMatrix(const char *name, const float m[4][4], bool transpose = false);
	void yafaray4_paramsClearAll(); 	//!< clear the paramMap and paramList
	void yafaray4_paramsStartList(); //!< start writing parameters to the extended paramList (used by materials)
	void yafaray4_paramsPushList(); 	//!< push new list item in paramList (e.g. new shader node description)
	void yafaray4_paramsEndList(); 	//!< revert to writing to normal paramMap
	void yafaray4_setCurrentMaterial(const char *name);
	bool yafaray4_createObject(const char *name);
	bool yafaray4_createLight(const char *name);
	bool yafaray4_createTexture(const char *name);
	bool yafaray4_createMaterial(const char *name);
	bool yafaray4_createCamera(const char *name);
	bool yafaray4_createBackground(const char *name);
	bool yafaray4_createIntegrator(const char *name);
	bool yafaray4_createVolumeRegion(const char *name);
	bool yafaray4_createRenderView(const char *name);
	bool yafaray4_createInternalOutput(const char *name, bool auto_delete = true); //!< ColorOutput creation, usually for internally-owned outputs that are destroyed when the scene is deleted or when libYafaRay instance is closed. If the client wants to keep ownership, it can set the "auto_delete" to false.
	bool yafaray4_setExternalOutput(const char *name, yafaray4_ColorOutput *output, bool auto_delete = false); //!< ColorOutput creation, usually for externally client-owned and client-supplied outputs that are *NOT* destroyed when the scene is deleted or when libYafaRay instance is closed. If the client wants to transfer ownership to libYafaRay, it can set the "auto_delete" to true.
	bool yafaray4_removeOutput(const char *name);
	void yafaray4_clearOutputs();
	void yafaray4_clearAll();
	void yafaray4_render(yafaray4_ProgressBar *pb = nullptr, bool auto_delete_progress_bar = false); //!< render the scene...
	void yafaray4_defineLayer(const char *layer_type_name, const char *exported_image_type_name, const char *exported_image_name, const char *image_type_name = "");
	bool yafaray4_setupLayersParameters();
	void yafaray4_abort();
	const char **yafaray4_getRenderParameters();

	bool yafaray4_setInteractive(bool interactive);
	void yafaray4_enablePrintDateTime(bool value);
	void yafaray4_setConsoleVerbosityLevel(const char *str_v_level);
	void yafaray4_setLogVerbosityLevel(const char *str_v_level);
	const char *yafaray4_getVersion(); //!< Get version to check against the exporters

	/*! Console Printing wrappers to report in color with yafaray's own console coloring */
	void yafaray4_printDebug(const char *msg);
	void yafaray4_printVerbose(const char *msg);
	void yafaray4_printInfo(const char *msg);
	void yafaray4_printParams(const char *msg);
	void yafaray4_printWarning(const char *msg);
	void yafaray4_printError(const char *msg);

	void yafaray4_setInputColorSpace(const char *color_space_string, float gamma_val);
};

#endif //YAFARAY_INTERFACE_C_H
