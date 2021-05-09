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

#include "interface/interface_c.h"

int main()
{
	//Basic libYafaRay C API usage example, rendering a cube with a TGA texture

	//YafaRay standard rendering interface
	yaf4_Interface_t *yi = yafaray4_createInterface("render");
	yafaray4_setConsoleVerbosityLevel(yi, "debug");

	//Creating scene
	yafaray4_paramsSetString(yi, "type", "yafaray");
	yafaray4_createScene(yi);
	yafaray4_paramsClearAll(yi);

	//Creating texture
	yafaray4_paramsSetString(yi, "type", "image");
	yafaray4_paramsSetString(yi, "filename", "test01_tex.tga");
	yafaray4_createTexture(yi, "TextureTGA");
	yafaray4_paramsClearAll(yi);

	//Creating material
	//General material parameters
	yafaray4_paramsSetString(yi, "type", "shinydiffusemat");
	yafaray4_paramsSetColor(yi, "color", 0.9, 0.9, 0.9, 1);
	//Shader tree definition
	yafaray4_paramsPushList(yi);
	yafaray4_paramsSetString(yi, "element", "shader_node");
	yafaray4_paramsSetString(yi, "name", "diff_layer0");
	yafaray4_paramsSetString(yi, "input", "map0");
	yafaray4_paramsSetString(yi, "type", "layer");
	yafaray4_paramsSetString(yi, "blend_mode", "mix");
	yafaray4_paramsSetColor(yi, "upper_color", 1,1,1,1);
	yafaray4_paramsPushList(yi);
	yafaray4_paramsSetString(yi, "element", "shader_node");
	yafaray4_paramsSetString(yi, "name", "map0");
	yafaray4_paramsSetString(yi, "type", "texture_mapper");
	yafaray4_paramsSetString(yi, "mapping", "cube");
	yafaray4_paramsSetString(yi, "texco", "orco");
	yafaray4_paramsSetString(yi, "texture", "TextureTGA");
	yafaray4_paramsEndList(yi);
	//Actual material creation
	yafaray4_paramsSetString(yi, "diffuse_shader", "diff_layer0");
	yafaray4_createMaterial(yi, "MaterialTGA");
	yafaray4_paramsClearAll(yi);

	//Creating geometric objects in the scene
	yafaray4_startGeometry(yi);

	//Creating a geometric object
	yafaray4_paramsSetBool(yi, "has_orco", 1);
	yafaray4_paramsSetString(yi, "type", "mesh");
	yafaray4_createObject(yi, "Cube");
	yafaray4_paramsClearAll(yi);
	//Creating vertices for the object
	yafaray4_addVertexWithOrco(yi, -4, 1.5, 0, -1, -1, -1);
	yafaray4_addVertexWithOrco(yi, -4, 1.5, 2, -1, -1, 1);
	yafaray4_addVertexWithOrco(yi, -4, 3.5, 0, -1, 1, -1);
	yafaray4_addVertexWithOrco(yi, -4, 3.5, 2, -1, 1, 1);
	yafaray4_addVertexWithOrco(yi, -2, 1.5, 0, 1, -1, -1);
	yafaray4_addVertexWithOrco(yi, -2, 1.5, 2, 1, -1, 1);
	yafaray4_addVertexWithOrco(yi, -2, 3.5, 0, 1, 1, -1);
	yafaray4_addVertexWithOrco(yi, -2, 3.5, 2, 1, 1, 1);
	//Setting up material for the faces (each face or group of faces can have different materials assigned)
	yafaray4_setCurrentMaterial(yi, "MaterialTGA");
	//Adding faces indicating the vertices indices used in each face
	yafaray4_addFace(yi, 2, 0, 1);
	yafaray4_addFace(yi, 2, 1, 3);
	yafaray4_addFace(yi, 3, 7, 6);
	yafaray4_addFace(yi, 3, 6, 2);
	yafaray4_addFace(yi, 7, 5, 4);
	yafaray4_addFace(yi, 7, 4, 6);
	yafaray4_addFace(yi, 0, 4, 5);
	yafaray4_addFace(yi, 0, 5, 1);
	yafaray4_addFace(yi, 0, 2, 6);
	yafaray4_addFace(yi, 0, 6, 4);
	yafaray4_addFace(yi, 5, 7, 3);
	yafaray4_addFace(yi, 5, 3, 1);

	//Ending definition of geometric objects
	yafaray4_endGeometry(yi);

	//Creating light/lamp
	yafaray4_paramsSetString(yi, "type", "pointlight");
	yafaray4_paramsSetColor(yi, "color", 1, 1, 1, 1);
	yafaray4_paramsSetVector(yi, "from", 5.3, -4.9, 8.9);
	yafaray4_paramsSetFloat(yi, "power", 150);
	yafaray4_createLight(yi, "light_1");
	yafaray4_paramsClearAll(yi);

	//Creating scene background
	yafaray4_paramsSetString(yi, "type", "constant");
	yafaray4_paramsSetColor(yi, "color", 1, 1, 1, 1);
	yafaray4_createBackground(yi, "world_background");
	yafaray4_paramsClearAll(yi);

	//Creating camera
	yafaray4_paramsSetString(yi, "type", "perspective");
	yafaray4_paramsSetInt(yi, "resx", 640);
	yafaray4_paramsSetInt(yi, "resy", 480);
	yafaray4_paramsSetFloat(yi, "focal", 1.1);
	yafaray4_paramsSetVector(yi, "from", 8.6, -7.2, 8.1);
	yafaray4_paramsSetVector(yi, "to", 8.0, -6.7, 7.6);
	yafaray4_paramsSetVector(yi, "up", 8.3, -6.8, 9);
	yafaray4_createCamera(yi, "cam_1");
	yafaray4_paramsClearAll(yi);

	//Creating scene view
	yafaray4_paramsSetString(yi, "camera_name", "cam_1");
	yafaray4_createRenderView(yi, "view_1");
	yafaray4_paramsClearAll(yi);

	//Creating image output
	yafaray4_paramsSetString(yi, "type", "image_output");
	yafaray4_paramsSetString(yi, "image_path", "./test01-output1.tga");
	yafaray4_createInternalOutput(yi, "output1_tga", 1);
	yafaray4_paramsClearAll(yi);

	//Creating surface integrator
	yafaray4_paramsSetString(yi, "type", "directlighting");
	yafaray4_createIntegrator(yi, "surfintegr");
	yafaray4_paramsClearAll(yi);

	//Creating volume integrator
	yafaray4_paramsSetString(yi, "type", "none");
	yafaray4_createIntegrator(yi, "volintegr");
	yafaray4_paramsClearAll(yi);

	//Defining internal and exported layers
	yafaray4_defineLayer(yi, "combined", "ColorAlpha", "Combined", "ColorAlphaWeight");
	yafaray4_setupLayersParameters(yi);
	yafaray4_paramsClearAll(yi);

	//Setting up render parameters
	yafaray4_paramsSetString(yi, "integrator_name", "surfintegr");
	yafaray4_paramsSetString(yi, "volintegrator_name", "volintegr");
	yafaray4_paramsSetString(yi, "scene_accelerator", "yafaray-kdtree-original");
	yafaray4_paramsSetString(yi, "background_name", "world_background");
	yafaray4_paramsSetInt(yi, "width", 640);
	yafaray4_paramsSetInt(yi, "height", 480);
	yafaray4_paramsSetInt(yi, "threads", -1);
	yafaray4_paramsSetInt(yi, "threads_photons", -1);
	//Rendering
	yafaray4_render(yi, NULL, 0);
	yafaray4_paramsClearAll(yi);

	//Destroying YafaRay interface. Scene and all objects inside are automatically destroyed
	yafaray4_destroyInterface(yi);
	return 0;
}