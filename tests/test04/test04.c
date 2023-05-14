/****************************************************************************
 *      This is part of the libYafaRay package
 *
 *      test04.c : dynamic scene with changes "on the fly"
 *      Should work even with a "barebones" libYafaRay built without
 *      any dependencies
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

#include "yafaray_c_api.h"
#include <stdio.h>
#include <string.h>

int main()
{
	size_t material_id = 0;
	size_t object_id = 0;
	const int width = 400;
	const int height = 400;

	printf("***** Test client 'test04' for libYafaRay *****\n");
	printf("Using libYafaRay version (%d.%d.%d)\n", yafaray_getVersionMajor(), yafaray_getVersionMinor(), yafaray_getVersionPatch());

	/* YafaRay standard rendering interface */
	yafaray_Scene *yi = yafaray_createInterface(YAFARAY_INTERFACE_FOR_RENDERING, "test04.xml", NULL, NULL, YAFARAY_DISPLAY_CONSOLE_NORMAL);
	yafaray_setConsoleLogColorsEnabled(yi, YAFARAY_BOOL_TRUE);
	yafaray_setConsoleVerbosityLevel(yi, YAFARAY_LOG_LEVEL_VERBOSE);

	/* Creating scene */
	yafaray_createScene(yi);
	yafaray_clearParamMap(param_map);

	/* Creating images for textures */
	yafaray_setParamMapString(yi, "type", "ColorAlpha");
	yafaray_setParamMapString(yi, "image_optimization", "optimized");
	yafaray_setParamMapString(yi, "filename", "tex.tga");
	yafaray_createImage(yi, "ImageTGA", NULL);
	yafaray_clearParamMap(param_map);

	yafaray_setParamMapString(yi, "type", "ColorAlpha");
	yafaray_setParamMapString(yi, "image_optimization", "none"); /* Note: only "none" allows HDR values > 1.f */
	yafaray_setParamMapString(yi, "filename", "tex.hdr");
	yafaray_createImage(yi, "ImageHDR", NULL);
	yafaray_clearParamMap(param_map);

	/* Creating textures from images */
	yafaray_setParamMapString(yi, "type", "image");
	yafaray_setParamMapString(yi, "image_name", "ImageTGA");
	yafaray_createTexture(yi, "TextureTGA");
	yafaray_clearParamMap(param_map);

	yafaray_setParamMapString(yi, "type", "image");
	yafaray_setParamMapString(yi, "image_name", "ImageHDR");
	yafaray_createTexture(yi, "TextureHDR");
	yafaray_clearParamMap(param_map);

	/* Creating material */
	/* General material parameters */
	yafaray_setParamMapString(yi, "type", "shinydiffusemat");
	yafaray_setParamMapColor(yi, "color", 1.f, 1.f, 1.f, 1.f);
	/* Shader tree definition */
	yafaray_addParamMapToList(param_map_list, param_map);
	yafaray_setParamMapString(yi, "name", "diff_layer0");
	yafaray_setParamMapString(yi, "input", "map0");
	yafaray_setParamMapString(yi, "type", "layer");
	yafaray_setParamMapString(yi, "blend_mode", "mix");
	yafaray_setParamMapColor(yi, "upper_color", 1.f,1.f,1.f,1.f);
	yafaray_addParamMapToList(param_map_list, param_map);
	yafaray_setParamMapString(yi, "name", "map0");
	yafaray_setParamMapString(yi, "type", "texture_mapper");
	yafaray_setParamMapString(yi, "mapping", "cube");
	yafaray_setParamMapString(yi, "texco", "orco");
	yafaray_setParamMapString(yi, "texture", "TextureTGA");
	yafaray_paramsEndList(yi);
	/* Actual material creation */
	yafaray_setParamMapString(yi, "diffuse_shader", "diff_layer0");
	yafaray_createMaterial(yi, "MaterialDynamic", NULL);
	yafaray_clearParamMap(param_map);

	/* Creating a geometric object */
	yafaray_setParamMapBool(yi, "has_orco", 1);
	yafaray_setParamMapString(yi, "type", "mesh");
	yafaray_createObject(yi, "Cube", &object_id);
	yafaray_clearParamMap(param_map);
	/* Creating vertices for the object */
	yafaray_addVertexWithOrco(yi, object_id, -4.f, 1.5f, 0.f, -1.f, -1.f, -1);
	yafaray_addVertexWithOrco(yi, object_id, -4.f, 1.5f, 2.f, -1.f, -1.f, 1);
	yafaray_addVertexWithOrco(yi, object_id, -4.f, 3.5f, 0.f, -1.f, 1.f, -1);
	yafaray_addVertexWithOrco(yi, object_id, -4.f, 3.5f, 2.f, -1.f, 1.f, 1);
	yafaray_addVertexWithOrco(yi, object_id, -2.f, 1.5f, 0.f, 1.f, -1.f, -1);
	yafaray_addVertexWithOrco(yi, object_id, -2.f, 1.5f, 2.f, 1.f, -1.f, 1);
	yafaray_addVertexWithOrco(yi, object_id, -2.f, 3.5f, 0.f, 1.f, 1.f, -1);
	yafaray_addVertexWithOrco(yi, object_id, -2.f, 3.5f, 2.f, 1.f, 1.f, 1);
	/* Setting up material for the faces (each face or group of faces can have different materials assigned) */
	yafaray_getMaterialId(yi, "MaterialDynamic", &material_id);
	/* Adding faces indicating the vertices indices used in each face */
	yafaray_addTriangle(yi, object_id, 2, 0, 1, material_id);
	yafaray_addTriangle(yi, object_id, 2, 1, 3, material_id);
	yafaray_addTriangle(yi, object_id, 3, 7, 6, material_id);
	yafaray_addTriangle(yi, object_id, 3, 6, 2, material_id);
	yafaray_addTriangle(yi, object_id, 7, 5, 4, material_id);
	yafaray_addTriangle(yi, object_id, 7, 4, 6, material_id);
	yafaray_addTriangle(yi, object_id, 0, 4, 5, material_id);
	yafaray_addTriangle(yi, object_id, 0, 5, 1, material_id);
	yafaray_addTriangle(yi, object_id, 0, 2, 6, material_id);
	yafaray_addTriangle(yi, object_id, 0, 6, 4, material_id);
	yafaray_addTriangle(yi, object_id, 5, 7, 3, material_id);
	yafaray_addTriangle(yi, object_id, 5, 3, 1, material_id);

	/* Creating light/lamp */
	yafaray_setParamMapString(yi, "type", "pointlight");
	yafaray_setParamMapColor(yi, "color", 1.f, 1.f, 1.f, 1.f);
	yafaray_setParamMapVector(yi, "from", 5.3f, -4.9f, 8.9f);
	yafaray_setParamMapFloat(yi, "power", 150.f);
	yafaray_createLight(yi, "light_1");
	yafaray_clearParamMap(param_map);

	/* Creating scene background */
	yafaray_setParamMapString(yi, "type", "constant");
	yafaray_setParamMapColor(yi, "color", 1.f, 1.f, 1.f, 1.f);
	yafaray_defineBackground(yi);
	yafaray_clearParamMap(param_map);

	/* Creating camera */
	yafaray_setParamMapString(yi, "type", "perspective");
	yafaray_setParamMapInt(yi, "resx", width);
	yafaray_setParamMapInt(yi, "resy", height);
	yafaray_setParamMapFloat(yi, "focal", 1.1f);
	yafaray_setParamMapVector(yi, "from", 8.6f, -7.2f, 8.1f);
	yafaray_setParamMapVector(yi, "to", 8.0f, -6.7f, 7.6f);
	yafaray_setParamMapVector(yi, "up", 8.3f, -6.8f, 9.f);
	yafaray_defineCamera(yi, "cam_1");
	yafaray_clearParamMap(param_map);

	/* Creating scene view */
	yafaray_setParamMapString(yi, "camera_name", "cam_1");
	yafaray_createRenderView(yi, "view_1");
	yafaray_clearParamMap(param_map);

	/* Creating surface integrator */
	yafaray_setParamMapString(yi, "type", "directlighting");
	yafaray_defineSurfaceIntegrator(yi);
	yafaray_clearParamMap(param_map);

	/* Setting up render parameters */
	yafaray_setParamMapString(yi, "scene_accelerator", "yafaray-kdtree-original");
	yafaray_setParamMapInt(yi, "width", width);
	yafaray_setParamMapInt(yi, "height", height);
	yafaray_setParamMapInt(yi, "AA_minsamples", 1);
	yafaray_setParamMapInt(yi, "AA_passes", 1);
	yafaray_setParamMapInt(yi, "threads", 1);
	yafaray_setParamMapInt(yi, "threads_photons", 1);
	yafaray_preprocessSurfaceIntegrator(yi);
	yafaray_clearParamMap(param_map);

	/* Creating image output */
	yafaray_setParamMapString(yi, "image_path", "./test04-output1.tga");
	yafaray_createOutput(yi, "output1_tga");
	yafaray_clearParamMap(param_map);

	/* Rendering */
	yafaray_render(yi, NULL, NULL, YAFARAY_DISPLAY_CONSOLE_NORMAL);


	/* Modifying (replacing) material */
	/* General material parameters */
	yafaray_setParamMapString(yi, "type", "shinydiffusemat");
	yafaray_setParamMapColor(yi, "color", 1.f, 1.f, 1.f, 1.f);
	/* Shader tree definition */
	yafaray_addParamMapToList(param_map_list, param_map);
	yafaray_setParamMapString(yi, "name", "diff_layer0");
	yafaray_setParamMapString(yi, "input", "map0");
	yafaray_setParamMapString(yi, "type", "layer");
	yafaray_setParamMapString(yi, "blend_mode", "mix");
	yafaray_setParamMapColor(yi, "upper_color", 1.f,1.f,1.f,1.f);
	yafaray_addParamMapToList(param_map_list, param_map);
	yafaray_setParamMapString(yi, "name", "map0");
	yafaray_setParamMapString(yi, "type", "texture_mapper");
	yafaray_setParamMapString(yi, "mapping", "cube");
	yafaray_setParamMapString(yi, "texco", "orco");
	yafaray_setParamMapString(yi, "texture", "TextureHDR");
	yafaray_paramsEndList(yi);
	/* Actual material creation */
	yafaray_setParamMapString(yi, "diffuse_shader", "diff_layer0");
	yafaray_createMaterial(yi, "MaterialDynamic", NULL);
	yafaray_clearParamMap(param_map);

	/* Using another image output */
	yafaray_clearOutputs(yi);
	yafaray_setParamMapString(yi, "image_path", "./test04-output2.tga");
	yafaray_createOutput(yi, "output2_tga");
	yafaray_clearParamMap(param_map);

	/* Rendering modified scene */
	yafaray_render(yi, NULL, NULL, YAFARAY_DISPLAY_CONSOLE_NORMAL);

	/* Destroying YafaRay interface. Scene and all objects inside are automatically destroyed */
	yafaray_destroyInterface(yi);
	return 0;
}

