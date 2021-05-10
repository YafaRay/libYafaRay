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

#include <cstring>
#include "interface/interface_c.h"
#include "interface/interface.h"
#include "export/export_xml.h"
#include "geometry/matrix4.h"
#include "render/monitor.h"

struct yafaray4_Interface *yafaray4_createInterface(const char *interface_type)
{
	std::string interface_type_str(interface_type);
	yafaray4::Interface *interface;
	if(interface_type_str == "export_xml") interface = new yafaray4::XmlExport("test.xml"); //FIXME!!!
	else interface = new yafaray4::Interface();
	return reinterpret_cast<yafaray4_Interface *>(interface);
}

void yafaray4_destroyInterface(yaf4_Interface_t *interface)
{
	delete reinterpret_cast<yafaray4::Interface *>(interface);
}

void yafaray4_createScene(yaf4_Interface_t *interface)
{
	reinterpret_cast<yafaray4::Interface *>(interface)->createScene();
}

bool_t yafaray4_startGeometry(yaf4_Interface_t *interface) //!< call before creating geometry; only meshes and vmaps can be created in this state
{
	return reinterpret_cast<yafaray4::Interface *>(interface)->startGeometry();
}
bool_t yafaray4_endGeometry(yaf4_Interface_t *interface) //!< call after creating geometry;
{
	return reinterpret_cast<yafaray4::Interface *>(interface)->endGeometry();
}

unsigned int yafaray4_getNextFreeId(yaf4_Interface_t *interface)
{
	return reinterpret_cast<yafaray4::Interface *>(interface)->getNextFreeId();
}

bool_t yafaray4_endObject(yaf4_Interface_t *interface) //!< end current mesh and return to geometry state
{
	return reinterpret_cast<yafaray4::Interface *>(interface)->endObject();
}

int  yafaray4_addVertex(yaf4_Interface_t *interface, double x, double y, double z) //!< add vertex to mesh; returns index to be used for addTriangle
{
	return reinterpret_cast<yafaray4::Interface *>(interface)->addVertex(x, y, z);
}

int  yafaray4_addVertexWithOrco(yaf4_Interface_t *interface, double x, double y, double z, double ox, double oy, double oz) //!< add vertex with Orco to mesh; returns index to be used for addTriangle
{
	return reinterpret_cast<yafaray4::Interface *>(interface)->addVertex(x, y, z, ox, oy, oz);
}

void yafaray4_addNormal(yaf4_Interface_t *interface, double nx, double ny, double nz) //!< add vertex normal to mesh; the vertex that will be attached to is the last one inserted by addVertex method
{
	reinterpret_cast<yafaray4::Interface *>(interface)->addNormal(nx, ny, nz);
}

bool_t yafaray4_addFace(yaf4_Interface_t *interface, int a, int b, int c) //!< add a triangle given vertex indices and material pointer
{
	return reinterpret_cast<yafaray4::Interface *>(interface)->addFace(a, b, c);
}

bool_t yafaray4_addFaceWithUv(yaf4_Interface_t *interface, int a, int b, int c, int uv_a, int uv_b, int uv_c) //!< add a triangle given vertex and uv indices and material pointer
{
	return reinterpret_cast<yafaray4::Interface *>(interface)->addFace(a, b, c, uv_a, uv_b, uv_c);
}

int  yafaray4_addUv(yaf4_Interface_t *interface, float u, float v) //!< add a UV coordinate pair; returns index to be used for addTriangle
{
	return reinterpret_cast<yafaray4::Interface *>(interface)->addUv(u, v);
}

bool_t yafaray4_smoothMesh(yaf4_Interface_t *interface, const char *name, double angle) //!< smooth vertex normals of mesh with given ID and angle (in degrees)
{
	return reinterpret_cast<yafaray4::Interface *>(interface)->smoothMesh(name, angle);
}

bool_t yafaray4_addInstance(yaf4_Interface_t *interface, const char *base_object_name, const float obj_to_world[4][4])
// functions to build paramMaps instead of passing them from Blender
// (decouling implementation details of STL containers, paraMap_t etc. as much as possible)
{
	return reinterpret_cast<yafaray4::Interface *>(interface)->addInstance(base_object_name, obj_to_world);
}

void yafaray4_paramsSetVector(yaf4_Interface_t *interface, const char *name, double x, double y, double z)
{
	reinterpret_cast<yafaray4::Interface *>(interface)->paramsSetVector(name, x, y, z);
}

void yafaray4_paramsSetString(yaf4_Interface_t *interface, const char *name, const char *s)
{
	reinterpret_cast<yafaray4::Interface *>(interface)->paramsSetString(name, s);
}

void yafaray4_paramsSetBool(yaf4_Interface_t *interface, const char *name, bool_t b)
{
	reinterpret_cast<yafaray4::Interface *>(interface)->paramsSetBool(name, b);
}

void yafaray4_paramsSetInt(yaf4_Interface_t *interface, const char *name, int i)
{
	reinterpret_cast<yafaray4::Interface *>(interface)->paramsSetInt(name, i);
}

void yafaray4_paramsSetFloat(yaf4_Interface_t *interface, const char *name, double f)
{
	reinterpret_cast<yafaray4::Interface *>(interface)->paramsSetFloat(name, f);
}

void yafaray4_paramsSetColor(yaf4_Interface_t *interface, const char *name, float r, float g, float b, float a)
{
	reinterpret_cast<yafaray4::Interface *>(interface)->paramsSetColor(name, r, g, b, a);
}

void yafaray4_paramsSetMatrix(yaf4_Interface_t *interface, const char *name, const float m[4][4], bool_t transpose)
{
	reinterpret_cast<yafaray4::Interface *>(interface)->paramsSetMatrix(name, m, transpose);
}

void yafaray4_paramsClearAll(yaf4_Interface_t *interface) 	//!< clear the paramMap and paramList
{
	reinterpret_cast<yafaray4::Interface *>(interface)->paramsClearAll();
}

void yafaray4_paramsPushList(yaf4_Interface_t *interface) 	//!< push new list item in paramList (e.g. new shader node description)
{
	reinterpret_cast<yafaray4::Interface *>(interface)->paramsPushList();
}

void yafaray4_paramsEndList(yaf4_Interface_t *interface) 	//!< revert to writing to normal paramMap
{
	reinterpret_cast<yafaray4::Interface *>(interface)->paramsEndList();
}

void yafaray4_setCurrentMaterial(yaf4_Interface_t *interface, const char *name)
{
	reinterpret_cast<yafaray4::Interface *>(interface)->setCurrentMaterial(name);
}

bool_t yafaray4_createObject(yaf4_Interface_t *interface, const char *name)
{
	return reinterpret_cast<yafaray4::Interface *>(interface)->createObject(name) != nullptr;
}

bool_t yafaray4_createLight(yaf4_Interface_t *interface, const char *name)
{
	return reinterpret_cast<yafaray4::Interface *>(interface)->createLight(name) != nullptr;
}

bool_t yafaray4_createTexture(yaf4_Interface_t *interface, const char *name)
{
	return reinterpret_cast<yafaray4::Interface *>(interface)->createTexture(name) != nullptr;
}

bool_t yafaray4_createMaterial(yaf4_Interface_t *interface, const char *name)
{
	return reinterpret_cast<yafaray4::Interface *>(interface)->createMaterial(name) != nullptr;
}

bool_t yafaray4_createCamera(yaf4_Interface_t *interface, const char *name)
{
	return reinterpret_cast<yafaray4::Interface *>(interface)->createCamera(name) != nullptr;
}

bool_t yafaray4_createBackground(yaf4_Interface_t *interface, const char *name)
{
	return reinterpret_cast<yafaray4::Interface *>(interface)->createBackground(name) != nullptr;
}

bool_t yafaray4_createIntegrator(yaf4_Interface_t *interface, const char *name)
{
	return reinterpret_cast<yafaray4::Interface *>(interface)->createIntegrator(name) != nullptr;
}

bool_t yafaray4_createVolumeRegion(yaf4_Interface_t *interface, const char *name)
{
	return reinterpret_cast<yafaray4::Interface *>(interface)->createVolumeRegion(name) != nullptr;
}

bool_t yafaray4_createRenderView(yaf4_Interface_t *interface, const char *name)
{
	return reinterpret_cast<yafaray4::Interface *>(interface)->createRenderView(name) != nullptr;
}

bool_t yafaray4_createInternalOutput(yaf4_Interface_t *interface, const char *name, bool_t auto_delete) //!< ColorOutput creation, usually for internally-owned outputs that are destroyed when the scene is deleted or when libYafaRay instance is closed. If the client wants to keep ownership, it can set the "auto_delete" to false.
{
	return reinterpret_cast<yafaray4::Interface *>(interface)->createOutput(name, auto_delete) != nullptr;
}

bool_t yafaray4_setExternalOutput(yaf4_Interface_t *interface, const char *name, struct yafaray4_ColorOutput *output, bool_t auto_delete) //!< ColorOutput creation, usually for externally client-owned and client-supplied outputs that are *NOT* destroyed when the scene is deleted or when libYafaRay instance is closed. If the client wants to transfer ownership to libYafaRay, it can set the "auto_delete" to true.
{
	return reinterpret_cast<yafaray4::Interface *>(interface)->createOutput(name, reinterpret_cast<yafaray4::ColorOutput *>(output), auto_delete) != nullptr;
}

bool_t yafaray4_removeOutput(yaf4_Interface_t *interface, const char *name)
{
	return reinterpret_cast<yafaray4::Interface *>(interface)->removeOutput(name);
}

void yafaray4_clearOutputs(yaf4_Interface_t *interface)
{
	reinterpret_cast<yafaray4::Interface *>(interface)->clearOutputs();
}

void yafaray4_clearAll(yaf4_Interface_t *interface)
{
	reinterpret_cast<yafaray4::Interface *>(interface)->clearAll();
}

void yafaray4_render(yaf4_Interface_t *interface, struct yafaray4_ProgressBar *pb, bool_t auto_delete_progress_bar) //!< render the scene...
{
	//FIXME!!
	auto progress_bar = new yafaray4::ConsoleProgressBar(80);
	reinterpret_cast<yafaray4::Interface *>(interface)->render(progress_bar, auto_delete_progress_bar);
}

void yafaray4_defineLayer(yaf4_Interface_t *interface, const char *layer_type_name, const char *exported_image_type_name, const char *exported_image_name, const char *image_type_name)
{
	reinterpret_cast<yafaray4::Interface *>(interface)->defineLayer(layer_type_name, exported_image_type_name, exported_image_name, image_type_name);
}

bool_t yafaray4_setupLayersParameters(yaf4_Interface_t *interface)
{
	return reinterpret_cast<yafaray4::Interface *>(interface)->setupLayersParameters();
}

void yafaray4_abort(yaf4_Interface_t *interface)
{
	reinterpret_cast<yafaray4::Interface *>(interface)->abort();
}

const char **yafaray4_getRenderParameters(yaf4_Interface_t *interface)
{
	return 0; //FIXME!
}

bool_t yafaray4_setInteractive(yaf4_Interface_t *interface, bool_t interactive)
{
	return reinterpret_cast<yafaray4::Interface *>(interface)->setInteractive(interactive);
}

void yafaray4_enablePrintDateTime(yaf4_Interface_t *interface, bool_t value)
{
	reinterpret_cast<yafaray4::Interface *>(interface)->enablePrintDateTime(value);
}

void yafaray4_setConsoleVerbosityLevel(yaf4_Interface_t *interface, const char *str_v_level)
{
	reinterpret_cast<yafaray4::Interface *>(interface)->setConsoleVerbosityLevel(str_v_level);
}

void yafaray4_setLogVerbosityLevel(yaf4_Interface_t *interface, const char *str_v_level)
{
	reinterpret_cast<yafaray4::Interface *>(interface)->setLogVerbosityLevel(str_v_level);
}

void yafaray4_getVersion(yaf4_Interface_t *interface, char *dest_string, size_t dest_string_size) //!< Get version to check against the exporters
{
	if(dest_string) strncpy(dest_string, reinterpret_cast<yafaray4::Interface *>(interface)->getVersion().c_str(), dest_string_size);
}

/*! Console Printing wrappers to report in color with yafaray's own console coloring */
void yafaray4_printDebug(yaf4_Interface_t *interface, const char *msg)
{
	reinterpret_cast<yafaray4::Interface *>(interface)->printDebug(msg);
}

void yafaray4_printVerbose(yaf4_Interface_t *interface, const char *msg)
{
	reinterpret_cast<yafaray4::Interface *>(interface)->printVerbose(msg);
}

void yafaray4_printInfo(yaf4_Interface_t *interface, const char *msg)
{
	reinterpret_cast<yafaray4::Interface *>(interface)->printInfo(msg);
}

void yafaray4_printParams(yaf4_Interface_t *interface, const char *msg)
{
	reinterpret_cast<yafaray4::Interface *>(interface)->printParams(msg);
}

void yafaray4_printWarning(yaf4_Interface_t *interface, const char *msg)
{
	reinterpret_cast<yafaray4::Interface *>(interface)->printWarning(msg);
}

void yafaray4_printError(yaf4_Interface_t *interface, const char *msg)
{
	reinterpret_cast<yafaray4::Interface *>(interface)->printError(msg);
}

void yafaray4_setInputColorSpace(yaf4_Interface_t *interface, const char *color_space_string, float gamma_val)
{
	reinterpret_cast<yafaray4::Interface *>(interface)->setInputColorSpace(color_space_string, gamma_val);
}
