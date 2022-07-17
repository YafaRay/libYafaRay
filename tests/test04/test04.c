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
	const int width = 400;
	const int height = 400;

	printf("***** Test client 'test04' for libYafaRay *****\n");
	printf("Using libYafaRay version (%d.%d.%d)\n", yafaray_getVersionMajor(), yafaray_getVersionMinor(), yafaray_getVersionPatch());

	/* YafaRay standard rendering interface */
	yafaray_Interface_t *yi = yafaray_createInterface(YAFARAY_INTERFACE_FOR_RENDERING, "test04.xml", NULL, NULL, YAFARAY_DISPLAY_CONSOLE_NORMAL);
	yafaray_setConsoleLogColorsEnabled(yi, YAFARAY_BOOL_TRUE);
	yafaray_setConsoleVerbosityLevel(yi, YAFARAY_LOG_LEVEL_DEBUG);

	/* Creating scene */
	yafaray_createScene(yi);
	yafaray_paramsClearAll(yi);

	/* Creating images for textures */
	yafaray_paramsSetString(yi, "type", "ColorAlpha");
	yafaray_paramsSetString(yi, "image_optimization", "optimized");
	yafaray_paramsSetInt(yi, "tex_width", 200);
	yafaray_paramsSetInt(yi, "tex_height", 200);
	yafaray_paramsSetString(yi, "filename", "tex.tga");
	yafaray_createImage(yi, "ImageTGA");
	yafaray_paramsClearAll(yi);

	yafaray_paramsSetString(yi, "type", "ColorAlpha");
	yafaray_paramsSetString(yi, "image_optimization", "none"); /* Note: only "none" allows HDR values > 1.f */
	yafaray_paramsSetInt(yi, "tex_width", 200);
	yafaray_paramsSetInt(yi, "tex_height", 200);
	yafaray_paramsSetString(yi, "filename", "tex.hdr");
	yafaray_createImage(yi, "ImageHDR");
	yafaray_paramsClearAll(yi);

	/* Creating textures from images */
	yafaray_paramsSetString(yi, "type", "image");
	yafaray_paramsSetString(yi, "image_name", "ImageTGA");
	yafaray_createTexture(yi, "TextureTGA");
	yafaray_paramsClearAll(yi);

	yafaray_paramsSetString(yi, "type", "image");
	yafaray_paramsSetString(yi, "image_name", "ImageHDR");
	yafaray_createTexture(yi, "TextureHDR");
	yafaray_paramsClearAll(yi);

	/* Creating material */
	/* General material parameters */
	yafaray_paramsSetString(yi, "type", "shinydiffusemat");
	yafaray_paramsSetColor(yi, "color", 1.f, 1.f, 1.f, 1.f);
	/* Shader tree definition */
	yafaray_paramsPushList(yi);
	yafaray_paramsSetString(yi, "element", "shader_node");
	yafaray_paramsSetString(yi, "name", "diff_layer0");
	yafaray_paramsSetString(yi, "input", "map0");
	yafaray_paramsSetString(yi, "type", "layer");
	yafaray_paramsSetString(yi, "blend_mode", "mix");
	yafaray_paramsSetColor(yi, "upper_color", 1.f,1.f,1.f,1.f);
	yafaray_paramsPushList(yi);
	yafaray_paramsSetString(yi, "element", "shader_node");
	yafaray_paramsSetString(yi, "name", "map0");
	yafaray_paramsSetString(yi, "type", "texture_mapper");
	yafaray_paramsSetString(yi, "mapping", "cube");
	yafaray_paramsSetString(yi, "texco", "orco");
	yafaray_paramsSetString(yi, "texture", "TextureTGA");
	yafaray_paramsEndList(yi);
	/* Actual material creation */
	yafaray_paramsSetString(yi, "diffuse_shader", "diff_layer0");
	yafaray_createMaterial(yi, "MaterialDynamic");
	yafaray_paramsClearAll(yi);

	/* Creating geometric objects in the scene */
	yafaray_startGeometry(yi);

	/* Creating a geometric object */
	yafaray_paramsSetBool(yi, "has_orco", 1);
	yafaray_paramsSetString(yi, "type", "mesh");
	yafaray_createObject(yi, "Cube");
	yafaray_paramsClearAll(yi);
	/* Creating vertices for the object */
	yafaray_addVertexWithOrco(yi, -4.f, 1.5f, 0.f, -1.f, -1.f, -1);
	yafaray_addVertexWithOrco(yi, -4.f, 1.5f, 2.f, -1.f, -1.f, 1);
	yafaray_addVertexWithOrco(yi, -4.f, 3.5f, 0.f, -1.f, 1.f, -1);
	yafaray_addVertexWithOrco(yi, -4.f, 3.5f, 2.f, -1.f, 1.f, 1);
	yafaray_addVertexWithOrco(yi, -2.f, 1.5f, 0.f, 1.f, -1.f, -1);
	yafaray_addVertexWithOrco(yi, -2.f, 1.5f, 2.f, 1.f, -1.f, 1);
	yafaray_addVertexWithOrco(yi, -2.f, 3.5f, 0.f, 1.f, 1.f, -1);
	yafaray_addVertexWithOrco(yi, -2.f, 3.5f, 2.f, 1.f, 1.f, 1);
	/* Setting up material for the faces (each face or group of faces can have different materials assigned) */
	yafaray_setCurrentMaterial(yi, "MaterialDynamic");
	/* Adding faces indicating the vertices indices used in each face */
	yafaray_addTriangle(yi, 2, 0, 1);
	yafaray_addTriangle(yi, 2, 1, 3);
	yafaray_addTriangle(yi, 3, 7, 6);
	yafaray_addTriangle(yi, 3, 6, 2);
	yafaray_addTriangle(yi, 7, 5, 4);
	yafaray_addTriangle(yi, 7, 4, 6);
	yafaray_addTriangle(yi, 0, 4, 5);
	yafaray_addTriangle(yi, 0, 5, 1);
	yafaray_addTriangle(yi, 0, 2, 6);
	yafaray_addTriangle(yi, 0, 6, 4);
	yafaray_addTriangle(yi, 5, 7, 3);
	yafaray_addTriangle(yi, 5, 3, 1);

	/* Ending definition of geometric objects */
	yafaray_endGeometry(yi);

	/* Creating light/lamp */
	yafaray_paramsSetString(yi, "type", "pointlight");
	yafaray_paramsSetColor(yi, "color", 1.f, 1.f, 1.f, 1.f);
	yafaray_paramsSetVector(yi, "from", 5.3f, -4.9f, 8.9f);
	yafaray_paramsSetFloat(yi, "power", 150.f);
	yafaray_createLight(yi, "light_1");
	yafaray_paramsClearAll(yi);

	/* Creating scene background */
	yafaray_paramsSetString(yi, "type", "constant");
	yafaray_paramsSetColor(yi, "color", 1.f, 1.f, 1.f, 1.f);
	yafaray_defineBackground(yi);
	yafaray_paramsClearAll(yi);

	/* Creating camera */
	yafaray_paramsSetString(yi, "type", "perspective");
	yafaray_paramsSetInt(yi, "resx", width);
	yafaray_paramsSetInt(yi, "resy", height);
	yafaray_paramsSetFloat(yi, "focal", 1.1f);
	yafaray_paramsSetVector(yi, "from", 8.6f, -7.2f, 8.1f);
	yafaray_paramsSetVector(yi, "to", 8.0f, -6.7f, 7.6f);
	yafaray_paramsSetVector(yi, "up", 8.3f, -6.8f, 9.f);
	yafaray_createCamera(yi, "cam_1");
	yafaray_paramsClearAll(yi);

	/* Creating scene view */
	yafaray_paramsSetString(yi, "camera_name", "cam_1");
	yafaray_createRenderView(yi, "view_1");
	yafaray_paramsClearAll(yi);

	/* Creating surface integrator */
	yafaray_paramsSetString(yi, "type", "directlighting");
	yafaray_defineSurfaceIntegrator(yi);
	yafaray_paramsClearAll(yi);

	/* Setting up render parameters */
	yafaray_paramsSetString(yi, "scene_accelerator", "yafaray-kdtree-original");
	yafaray_paramsSetInt(yi, "width", width);
	yafaray_paramsSetInt(yi, "height", height);
	yafaray_paramsSetInt(yi, "AA_minsamples", 1);
	yafaray_paramsSetInt(yi, "AA_passes", 1);
	yafaray_paramsSetInt(yi, "threads", 1);
	yafaray_paramsSetInt(yi, "threads_photons", 1);
	yafaray_setupRender(yi);
	yafaray_paramsClearAll(yi);

	/* Creating image output */
	yafaray_paramsSetString(yi, "image_path", "./test04-output1.tga");
	yafaray_createOutput(yi, "output1_tga");
	yafaray_paramsClearAll(yi);

	/* Rendering */
	yafaray_render(yi, NULL, NULL, YAFARAY_DISPLAY_CONSOLE_NORMAL);


	/* Modifying (replacing) material */
	/* General material parameters */
	yafaray_paramsSetString(yi, "type", "shinydiffusemat");
	yafaray_paramsSetColor(yi, "color", 1.f, 1.f, 1.f, 1.f);
	/* Shader tree definition */
	yafaray_paramsPushList(yi);
	yafaray_paramsSetString(yi, "element", "shader_node");
	yafaray_paramsSetString(yi, "name", "diff_layer0");
	yafaray_paramsSetString(yi, "input", "map0");
	yafaray_paramsSetString(yi, "type", "layer");
	yafaray_paramsSetString(yi, "blend_mode", "mix");
	yafaray_paramsSetColor(yi, "upper_color", 1.f,1.f,1.f,1.f);
	yafaray_paramsPushList(yi);
	yafaray_paramsSetString(yi, "element", "shader_node");
	yafaray_paramsSetString(yi, "name", "map0");
	yafaray_paramsSetString(yi, "type", "texture_mapper");
	yafaray_paramsSetString(yi, "mapping", "cube");
	yafaray_paramsSetString(yi, "texco", "orco");
	yafaray_paramsSetString(yi, "texture", "TextureHDR");
	yafaray_paramsEndList(yi);
	/* Actual material creation */
	yafaray_paramsSetString(yi, "diffuse_shader", "diff_layer0");
	yafaray_createMaterial(yi, "MaterialDynamic");
	yafaray_paramsClearAll(yi);

	/* Using another image output */
	yafaray_clearOutputs(yi);
	yafaray_paramsSetString(yi, "image_path", "./test04-output2.tga");
	yafaray_createOutput(yi, "output2_tga");
	yafaray_paramsClearAll(yi);

	/* Rendering modified scene */
	yafaray_render(yi, NULL, NULL, YAFARAY_DISPLAY_CONSOLE_NORMAL);

	/* Destroying YafaRay interface. Scene and all objects inside are automatically destroyed */
	yafaray_destroyInterface(yi);
	return 0;
}

