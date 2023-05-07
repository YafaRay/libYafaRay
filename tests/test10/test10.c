/****************************************************************************
 *      This is part of the libYafaRay package
 *
 *      test10.c : container tests and exporting container to a file
 *      If libYafaRay is not built with all the available image format
 *      dependencies, then some cubes will appear white lacking that texture.
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

int main()
{
	yafaray_ParamMap *param_map = NULL;
	yafaray_ParamMapList *param_map_list = NULL;
	yafaray_Logger *logger = NULL;
	yafaray_SurfaceIntegrator *surface_integrator = NULL;
	yafaray_Scene *scene = NULL;
	yafaray_Film *film_1 = NULL;
	yafaray_Film *film_2 = NULL;
	yafaray_RenderControl *render_control = NULL;
	yafaray_RenderMonitor *render_monitor = NULL;
	yafaray_SceneModifiedFlags scene_modified_flags = YAFARAY_SCENE_MODIFIED_NOTHING;
	size_t material_id = 0;
	size_t object_id = 0;
	size_t image_id = 0;
	yafaray_Container *container = NULL;
	char *exported_string = NULL;
	FILE *fp = NULL;

	/* Creating logger */
	logger = yafaray_createLogger(NULL, NULL, YAFARAY_DISPLAY_CONSOLE_NORMAL);
	yafaray_setConsoleLogColorsEnabled(logger, YAFARAY_BOOL_TRUE);
	yafaray_setConsoleVerbosityLevel(logger, YAFARAY_LOG_LEVEL_VERBOSE);

	/* Creating scene */
	scene = yafaray_createScene(logger, "scene");

	/* Creating param map and param map list */
	param_map = yafaray_createParamMap();
	param_map_list = yafaray_createParamMapList();

	/* Creating images and image textures */
	yafaray_clearParamMap(param_map);
	yafaray_setParamMapString(param_map, "filename", "tex.tif");
	yafaray_createImage(scene, "Texture.005_image", &image_id, param_map);

	yafaray_clearParamMap(param_map);
	yafaray_setParamMapString(param_map, "image_name", "Texture.005_image");
	yafaray_setParamMapString(param_map, "type", "image");
	yafaray_createTexture(scene, "Texture.005", param_map);

	yafaray_clearParamMap(param_map);
	yafaray_setParamMapString(param_map, "filename", "tex.tga");
	yafaray_createImage(scene, "Texture.004_image", &image_id, param_map);

	yafaray_clearParamMap(param_map);
	yafaray_setParamMapString(param_map, "image_name", "Texture.004_image");
	yafaray_setParamMapString(param_map, "type", "image");
	yafaray_createTexture(scene, "Texture.004", param_map);

	yafaray_clearParamMap(param_map);
	yafaray_setParamMapString(param_map, "filename", "tex.png");
	yafaray_createImage(scene, "Texture.003_image", &image_id, param_map);

	yafaray_clearParamMap(param_map);
	yafaray_setParamMapString(param_map, "image_name", "Texture.003_image");
	yafaray_setParamMapString(param_map, "type", "image");
	yafaray_createTexture(scene, "Texture.003", param_map);

	yafaray_clearParamMap(param_map);
	yafaray_setParamMapString(param_map, "filename", "tex.jpg");
	yafaray_createImage(scene, "Texture.002_image", &image_id, param_map);

	yafaray_clearParamMap(param_map);
	yafaray_setParamMapString(param_map, "image_name", "Texture.002_image");
	yafaray_setParamMapString(param_map, "type", "image");
	yafaray_createTexture(scene, "Texture.002", param_map);

	yafaray_clearParamMap(param_map);
	yafaray_setParamMapString(param_map, "color_space", "LinearRGB");
	yafaray_setParamMapString(param_map, "filename", "tex.hdr");
	yafaray_createImage(scene, "Texture.001_image", &image_id, param_map);

	yafaray_clearParamMap(param_map);
	yafaray_setParamMapString(param_map, "image_name", "Texture.001_image");
	yafaray_setParamMapString(param_map, "type", "image");
	yafaray_createTexture(scene, "Texture.001", param_map);

	yafaray_clearParamMap(param_map);
	yafaray_setParamMapString(param_map, "color_space", "LinearRGB");
	yafaray_setParamMapString(param_map, "filename", "tex.exr");
	yafaray_createImage(scene, "Texture_image", &image_id, param_map);

	yafaray_clearParamMap(param_map);
	yafaray_setParamMapString(param_map, "image_name", "Texture_image");
	yafaray_setParamMapString(param_map, "type", "image");
	yafaray_createTexture(scene, "Texture", param_map);

	/* Creating materials, each with its list of shader nodes (if any) */
	yafaray_clearParamMapList(param_map_list);
	yafaray_clearParamMap(param_map);
	yafaray_setParamMapColor(param_map, "color", 0.8, 0.8, 0.8, 1);
	yafaray_setParamMapString(param_map, "type", "shinydiffusemat");
	yafaray_createMaterial(scene, &material_id, "defaultMat", param_map, param_map_list);

	yafaray_clearParamMapList(param_map_list);
	yafaray_clearParamMap(param_map);
		yafaray_setParamMapString(param_map, "element", "shader_node");
		yafaray_setParamMapString(param_map, "input", "map0");
		yafaray_setParamMapString(param_map, "name", "diff_layer0");
		yafaray_setParamMapString(param_map, "type", "layer");
		yafaray_setParamMapColor(param_map, "upper_color", 0.8, 0.8, 0.8, 1);
	yafaray_addParamMapToList(param_map_list, param_map);
	yafaray_clearParamMap(param_map);
		yafaray_setParamMapString(param_map, "element", "shader_node");
		yafaray_setParamMapString(param_map, "mapping", "cube");
		yafaray_setParamMapString(param_map, "name", "map0");
		yafaray_setParamMapString(param_map, "texco", "orco");
		yafaray_setParamMapString(param_map, "texture", "Texture.005");
		yafaray_setParamMapString(param_map, "type", "texture_mapper");
	yafaray_addParamMapToList(param_map_list, param_map);
	yafaray_clearParamMap(param_map);
	yafaray_setParamMapColor(param_map, "color", 0.8, 0.8, 0.8, 1);
	yafaray_setParamMapFloat(param_map, "diffuse_reflect", 0.5);
	yafaray_setParamMapString(param_map, "diffuse_shader", "diff_layer0");
	yafaray_setParamMapInt(param_map, "mat_pass_index", 2);
	yafaray_setParamMapString(param_map, "type", "shinydiffusemat");
	yafaray_createMaterial(scene, &material_id, "Material.008", param_map, param_map_list);

	yafaray_clearParamMap(param_map);
	yafaray_clearParamMapList(param_map_list);
		yafaray_setParamMapString(param_map, "element", "shader_node");
		yafaray_setParamMapString(param_map, "input", "map0");
		yafaray_setParamMapString(param_map, "name", "diff_layer0");
		yafaray_setParamMapString(param_map, "type", "layer");
		yafaray_setParamMapColor(param_map, "upper_color", 0.8, 0.8, 0.8, 1);
	yafaray_addParamMapToList(param_map_list, param_map);
	yafaray_clearParamMap(param_map);
		yafaray_setParamMapString(param_map, "element", "shader_node");
		yafaray_setParamMapString(param_map, "mapping", "cube");
		yafaray_setParamMapString(param_map, "name", "map0");
		yafaray_setParamMapString(param_map, "texco", "orco");
		yafaray_setParamMapString(param_map, "texture", "Texture.004");
		yafaray_setParamMapString(param_map, "type", "texture_mapper");
	yafaray_addParamMapToList(param_map_list, param_map);
	yafaray_clearParamMap(param_map);
	yafaray_setParamMapColor(param_map, "color", 0.8, 0.8, 0.8, 1);
	yafaray_setParamMapFloat(param_map, "diffuse_reflect", 1);
	yafaray_setParamMapString(param_map, "diffuse_shader", "diff_layer0");
	yafaray_setParamMapInt(param_map, "mat_pass_index", 2);
	yafaray_setParamMapString(param_map, "type", "shinydiffusemat");
	yafaray_createMaterial(scene, &material_id, "Material.007", param_map, param_map_list);

	yafaray_clearParamMap(param_map);
	yafaray_clearParamMapList(param_map_list);
		yafaray_setParamMapString(param_map, "element", "shader_node");
		yafaray_setParamMapString(param_map, "input", "map0");
		yafaray_setParamMapString(param_map, "name", "diff_layer0");
		yafaray_setParamMapString(param_map, "type", "layer");
		yafaray_setParamMapColor(param_map, "upper_color", 0.8, 0.8, 0.8, 1);
	yafaray_addParamMapToList(param_map_list, param_map);
	yafaray_clearParamMap(param_map);
		yafaray_setParamMapString(param_map, "element", "shader_node");
		yafaray_setParamMapString(param_map, "mapping", "cube");
		yafaray_setParamMapString(param_map, "name", "map0");
		yafaray_setParamMapString(param_map, "texco", "orco");
		yafaray_setParamMapString(param_map, "texture", "Texture.003");
		yafaray_setParamMapString(param_map, "type", "texture_mapper");
	yafaray_addParamMapToList(param_map_list, param_map);
	yafaray_clearParamMap(param_map);
	yafaray_setParamMapColor(param_map, "color", 0.8, 0.8, 0.8, 1);
	yafaray_setParamMapFloat(param_map, "diffuse_reflect", 0.6);
	yafaray_setParamMapString(param_map, "diffuse_shader", "diff_layer0");
	yafaray_setParamMapInt(param_map, "mat_pass_index", 1);
	yafaray_setParamMapString(param_map, "type", "shinydiffusemat");
	yafaray_createMaterial(scene, &material_id, "Material.006", param_map, param_map_list);

	yafaray_clearParamMapList(param_map_list);
	yafaray_clearParamMap(param_map);
		yafaray_setParamMapString(param_map, "element", "shader_node");
		yafaray_setParamMapString(param_map, "input", "map0");
		yafaray_setParamMapString(param_map, "name", "diff_layer0");
		yafaray_setParamMapString(param_map, "type", "layer");
		yafaray_setParamMapColor(param_map, "upper_color", 0.8, 0.8, 0.8, 1);
	yafaray_addParamMapToList(param_map_list, param_map);
	yafaray_clearParamMap(param_map);
		yafaray_setParamMapString(param_map, "element", "shader_node");
		yafaray_setParamMapString(param_map, "mapping", "cube");
		yafaray_setParamMapString(param_map, "name", "map0");
		yafaray_setParamMapString(param_map, "texco", "orco");
		yafaray_setParamMapString(param_map, "texture", "Texture.002");
		yafaray_setParamMapString(param_map, "type", "texture_mapper");
	yafaray_addParamMapToList(param_map_list, param_map);
	yafaray_clearParamMap(param_map);
	yafaray_setParamMapColor(param_map, "color", 0.8, 0.8, 0.8, 1);
	yafaray_setParamMapFloat(param_map, "diffuse_reflect", 0.4);
	yafaray_setParamMapString(param_map, "diffuse_shader", "diff_layer0");
	yafaray_setParamMapString(param_map, "type", "shinydiffusemat");
	yafaray_createMaterial(scene, &material_id, "Material.005", param_map, param_map_list);

	yafaray_clearParamMapList(param_map_list);
	yafaray_clearParamMap(param_map);
		yafaray_setParamMapString(param_map, "blend_mode", "mix");
		yafaray_setParamMapFloat(param_map, "colfac", 0.95);
		yafaray_setParamMapString(param_map, "element", "shader_node");
		yafaray_setParamMapString(param_map, "input", "map0");
		yafaray_setParamMapString(param_map, "name", "diff_layer0");
		yafaray_setParamMapString(param_map, "type", "layer");
		yafaray_setParamMapColor(param_map, "upper_color", 0.8, 0.8, 0.8, 1);
	yafaray_addParamMapToList(param_map_list, param_map);
	yafaray_clearParamMap(param_map);
		yafaray_setParamMapString(param_map, "element", "shader_node");
		yafaray_setParamMapString(param_map, "mapping", "cube");
		yafaray_setParamMapString(param_map, "name", "map0");
		yafaray_setParamMapString(param_map, "texco", "orco");
		yafaray_setParamMapString(param_map, "texture", "Texture.001");
		yafaray_setParamMapString(param_map, "type", "texture_mapper");
	yafaray_addParamMapToList(param_map_list, param_map);
	yafaray_clearParamMap(param_map);
	yafaray_setParamMapColor(param_map, "color", 0.8, 0.8, 0.8, 1);
	yafaray_setParamMapFloat(param_map, "diffuse_reflect", 1);
	yafaray_setParamMapString(param_map, "diffuse_shader", "diff_layer0");
	yafaray_setParamMapString(param_map, "type", "shinydiffusemat");
	yafaray_createMaterial(scene, &material_id, "Material.004", param_map, param_map_list);

	yafaray_clearParamMapList(param_map_list);
	yafaray_clearParamMap(param_map);
		yafaray_setParamMapString(param_map, "blend_mode", "mix");
		yafaray_setParamMapFloat(param_map, "colfac", 0.95);
		yafaray_setParamMapString(param_map, "element", "shader_node");
		yafaray_setParamMapString(param_map, "input", "map0");
		yafaray_setParamMapString(param_map, "name", "diff_layer0");
		yafaray_setParamMapString(param_map, "type", "layer");
		yafaray_setParamMapColor(param_map, "upper_color", 0.8, 0.8, 0.8, 1);
	yafaray_addParamMapToList(param_map_list, param_map);
	yafaray_clearParamMap(param_map);
		yafaray_setParamMapString(param_map, "element", "shader_node");
		yafaray_setParamMapString(param_map, "mapping", "cube");
		yafaray_setParamMapString(param_map, "name", "map0");
		yafaray_setParamMapString(param_map, "texco", "orco");
		yafaray_setParamMapString(param_map, "texture", "Texture");
		yafaray_setParamMapString(param_map, "type", "texture_mapper");
	yafaray_addParamMapToList(param_map_list, param_map);
	yafaray_clearParamMap(param_map);
	yafaray_setParamMapColor(param_map, "color", 0.8, 0.8, 0.8, 1);
	yafaray_setParamMapFloat(param_map, "diffuse_reflect", 1);
	yafaray_setParamMapString(param_map, "diffuse_shader", "diff_layer0");
	yafaray_setParamMapString(param_map, "type", "shinydiffusemat");
	yafaray_createMaterial(scene, &material_id, "Material.003", param_map, param_map_list);

	yafaray_clearParamMapList(param_map_list);
	yafaray_clearParamMap(param_map);
	yafaray_setParamMapColor(param_map, "color", 0.8, 0.8, 0.8, 1);
	yafaray_setParamMapFloat(param_map, "diffuse_reflect", 1);
	yafaray_setParamMapString(param_map, "type", "shinydiffusemat");
	yafaray_createMaterial(scene, &material_id, "Material.002", param_map, param_map_list);

	yafaray_clearParamMap(param_map);
	yafaray_setParamMapColor(param_map, "color", 1, 1, 1, 1);
	yafaray_setParamMapVector(param_map, "from", 5.27648, -4.88993, 8.89514);
	yafaray_setParamMapFloat(param_map, "power", 72);
	yafaray_setParamMapString(param_map, "type", "pointlight");
	yafaray_createLight(scene, "Point", param_map);

	/* Creating objects/meshes */
	yafaray_clearParamMap(param_map);
	yafaray_setParamMapBool(param_map, "has_orco", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapInt(param_map, "num_faces", 6);
	yafaray_setParamMapInt(param_map, "num_vertices", 8);
	yafaray_setParamMapInt(param_map, "object_index", 5);
	yafaray_setParamMapString(param_map, "type", "mesh");
	yafaray_createObject(scene, &object_id, "Cube.005", param_map);
	yafaray_addVertexWithOrco(scene, object_id, -4.40469, 1.44162, 1.00136e-05, -1, -1, -1);
	yafaray_addVertexWithOrco(scene, object_id, -4.40469, 1.44162, 2.00001, -1, -1, 1);
	yafaray_addVertexWithOrco(scene, object_id, -4.40469, 3.44162, 1.00136e-05, -1, 1, -1);
	yafaray_addVertexWithOrco(scene, object_id, -4.40469, 3.44162, 2.00001, -1, 1, 1);
	yafaray_addVertexWithOrco(scene, object_id, -2.40468, 1.44162, 1.00136e-05, 1, -1, -1);
	yafaray_addVertexWithOrco(scene, object_id, -2.40468, 1.44162, 2.00001, 1, -1, 1);
	yafaray_addVertexWithOrco(scene, object_id, -2.40468, 3.44162, 1.00136e-05, 1, 1, -1);
	yafaray_addVertexWithOrco(scene, object_id, -2.40468, 3.44162, 2.00001, 1, 1, 1);
	yafaray_getMaterialId(scene, &material_id, "Material.008");
	yafaray_addQuad(scene, object_id, 2, 0, 1, 3, material_id);
	yafaray_addQuad(scene, object_id, 3, 7, 6, 2, material_id);
	yafaray_addQuad(scene, object_id, 7, 5, 4, 6, material_id);
	yafaray_addQuad(scene, object_id, 0, 4, 5, 1, material_id);
	yafaray_addQuad(scene, object_id, 0, 2, 6, 4, material_id);
	yafaray_addQuad(scene, object_id, 5, 7, 3, 1, material_id);
	yafaray_initObject(scene, object_id, material_id);

	yafaray_clearParamMap(param_map);
	yafaray_setParamMapBool(param_map, "has_orco", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapInt(param_map, "num_faces", 6);
	yafaray_setParamMapInt(param_map, "num_vertices", 8);
	yafaray_setParamMapInt(param_map, "object_index", 4);
	yafaray_setParamMapString(param_map, "type", "mesh");
	yafaray_createObject(scene, &object_id, "Cube.004", param_map);
	yafaray_addVertexWithOrco(scene, object_id, 3.26859, -0.393062, 1.00136e-05, -1, -1, -1);
	yafaray_addVertexWithOrco(scene, object_id, 3.26859, -0.393062, 2.00001, -1, -1, 1);
	yafaray_addVertexWithOrco(scene, object_id, 3.26859, 1.60694, 1.00136e-05, -1, 1, -1);
	yafaray_addVertexWithOrco(scene, object_id, 3.26859, 1.60694, 2.00001, -1, 1, 1);
	yafaray_addVertexWithOrco(scene, object_id, 5.26859, -0.393062, 1.00136e-05, 1, -1, -1);
	yafaray_addVertexWithOrco(scene, object_id, 5.26859, -0.393062, 2.00001, 1, -1, 1);
	yafaray_addVertexWithOrco(scene, object_id, 5.26859, 1.60694, 1.00136e-05, 1, 1, -1);
	yafaray_addVertexWithOrco(scene, object_id, 5.26859, 1.60694, 2.00001, 1, 1, 1);
	yafaray_getMaterialId(scene, &material_id, "Material.007");
	yafaray_addQuad(scene, object_id, 2, 0, 1, 3, material_id);
	yafaray_addQuad(scene, object_id, 3, 7, 6, 2, material_id);
	yafaray_addQuad(scene, object_id, 7, 5, 4, 6, material_id);
	yafaray_addQuad(scene, object_id, 0, 4, 5, 1, material_id);
	yafaray_addQuad(scene, object_id, 0, 2, 6, 4, material_id);
	yafaray_addQuad(scene, object_id, 5, 7, 3, 1, material_id);
	yafaray_initObject(scene, object_id, material_id);

	yafaray_clearParamMap(param_map);
	yafaray_setParamMapBool(param_map, "has_orco", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapInt(param_map, "num_faces", 6);
	yafaray_setParamMapInt(param_map, "num_vertices", 8);
	yafaray_setParamMapInt(param_map, "object_index", 3);
	yafaray_setParamMapString(param_map, "type", "mesh");
	yafaray_createObject(scene, &object_id, "Cube.003", param_map);
	yafaray_addVertexWithOrco(scene, object_id, -0.635578, 3.54144, 1.00136e-05, -1, -1, -1);
	yafaray_addVertexWithOrco(scene, object_id, -0.635578, 3.54144, 2.00001, -1, -1, 1);
	yafaray_addVertexWithOrco(scene, object_id, -0.635578, 5.54144, 1.00136e-05, -1, 1, -1);
	yafaray_addVertexWithOrco(scene, object_id, -0.635578, 5.54144, 2.00001, -1, 1, 1);
	yafaray_addVertexWithOrco(scene, object_id, 1.36442, 3.54144, 1.00136e-05, 1, -1, -1);
	yafaray_addVertexWithOrco(scene, object_id, 1.36442, 3.54144, 2.00001, 1, -1, 1);
	yafaray_addVertexWithOrco(scene, object_id, 1.36442, 5.54144, 1.00136e-05, 1, 1, -1);
	yafaray_addVertexWithOrco(scene, object_id, 1.36442, 5.54144, 2.00001, 1, 1, 1);
	yafaray_getMaterialId(scene, &material_id, "Material.006");
	yafaray_addQuad(scene, object_id, 2, 0, 1, 3, material_id);
	yafaray_addQuad(scene, object_id, 3, 7, 6, 2, material_id);
	yafaray_addQuad(scene, object_id, 7, 5, 4, 6, material_id);
	yafaray_addQuad(scene, object_id, 0, 4, 5, 1, material_id);
	yafaray_addQuad(scene, object_id, 0, 2, 6, 4, material_id);
	yafaray_addQuad(scene, object_id, 5, 7, 3, 1, material_id);
	yafaray_initObject(scene, object_id, material_id);

	yafaray_clearParamMap(param_map);
	yafaray_setParamMapBool(param_map, "has_orco", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapInt(param_map, "num_faces", 6);
	yafaray_setParamMapInt(param_map, "num_vertices", 8);
	yafaray_setParamMapInt(param_map, "object_index", 2);
	yafaray_setParamMapString(param_map, "type", "mesh");
	yafaray_createObject(scene, &object_id, "Cube.002", param_map);
	yafaray_addVertexWithOrco(scene, object_id, -0.635578, -0.393062, 1.00136e-05, -1, -1, -1);
	yafaray_addVertexWithOrco(scene, object_id, -0.635578, -0.393062, 2.00001, -1, -1, 1);
	yafaray_addVertexWithOrco(scene, object_id, -0.635578, 1.60694, 1.00136e-05, -1, 1, -1);
	yafaray_addVertexWithOrco(scene, object_id, -0.635578, 1.60694, 2.00001, -1, 1, 1);
	yafaray_addVertexWithOrco(scene, object_id, 1.36442, -0.393062, 1.00136e-05, 1, -1, -1);
	yafaray_addVertexWithOrco(scene, object_id, 1.36442, -0.393062, 2.00001, 1, -1, 1);
	yafaray_addVertexWithOrco(scene, object_id, 1.36442, 1.60694, 1.00136e-05, 1, 1, -1);
	yafaray_addVertexWithOrco(scene, object_id, 1.36442, 1.60694, 2.00001, 1, 1, 1);
	yafaray_getMaterialId(scene, &material_id, "Material.005");
	yafaray_addTriangle(scene, object_id, 2, 0, 1, material_id);
	yafaray_addTriangle(scene, object_id, 2, 1, 3, material_id);
	yafaray_addTriangle(scene, object_id, 3, 7, 6, material_id);
	yafaray_addTriangle(scene, object_id, 3, 6, 2, material_id);
	yafaray_addTriangle(scene, object_id, 7, 5, 4, material_id);
	yafaray_addTriangle(scene, object_id, 7, 4, 6, material_id);
	yafaray_addTriangle(scene, object_id, 0, 4, 5, material_id);
	yafaray_addTriangle(scene, object_id, 0, 5, 1, material_id);
	yafaray_addTriangle(scene, object_id, 0, 2, 6, material_id);
	yafaray_addTriangle(scene, object_id, 0, 6, 4, material_id);
	yafaray_addTriangle(scene, object_id, 5, 7, 3, material_id);
	yafaray_addTriangle(scene, object_id, 5, 3, 1, material_id);
	yafaray_initObject(scene, object_id, material_id);

	yafaray_clearParamMap(param_map);
	yafaray_setParamMapBool(param_map, "has_orco", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapInt(param_map, "num_faces", 6);
	yafaray_setParamMapInt(param_map, "num_vertices", 8);
	yafaray_setParamMapInt(param_map, "object_index", 1);
	yafaray_setParamMapString(param_map, "type", "mesh");
	yafaray_createObject(scene, &object_id, "Cube.001", param_map);
	yafaray_addVertexWithOrco(scene, object_id, -0.635578, -3.81854, 1.00136e-05, -1, -1, -1);
	yafaray_addVertexWithOrco(scene, object_id, -0.635578, -3.81854, 2.00001, -1, -1, 1);
	yafaray_addVertexWithOrco(scene, object_id, -0.635578, -1.81854, 1.00136e-05, -1, 1, -1);
	yafaray_addVertexWithOrco(scene, object_id, -0.635578, -1.81854, 2.00001, -1, 1, 1);
	yafaray_addVertexWithOrco(scene, object_id, 1.36442, -3.81854, 1.00136e-05, 1, -1, -1);
	yafaray_addVertexWithOrco(scene, object_id, 1.36442, -3.81854, 2.00001, 1, -1, 1);
	yafaray_addVertexWithOrco(scene, object_id, 1.36442, -1.81854, 1.00136e-05, 1, 1, -1);
	yafaray_addVertexWithOrco(scene, object_id, 1.36442, -1.81854, 2.00001, 1, 1, 1);
	yafaray_getMaterialId(scene, &material_id, "Material.004");
	yafaray_addTriangle(scene, object_id, 2, 0, 1, material_id);
	yafaray_addTriangle(scene, object_id, 2, 1, 3, material_id);
	yafaray_addTriangle(scene, object_id, 3, 7, 6, material_id);
	yafaray_addTriangle(scene, object_id, 3, 6, 2, material_id);
	yafaray_addTriangle(scene, object_id, 7, 5, 4, material_id);
	yafaray_addTriangle(scene, object_id, 7, 4, 6, material_id);
	yafaray_addTriangle(scene, object_id, 0, 4, 5, material_id);
	yafaray_addTriangle(scene, object_id, 0, 5, 1, material_id);
	yafaray_addTriangle(scene, object_id, 0, 2, 6, material_id);
	yafaray_addTriangle(scene, object_id, 0, 6, 4, material_id);
	yafaray_addTriangle(scene, object_id, 5, 7, 3, material_id);
	yafaray_addTriangle(scene, object_id, 5, 3, 1, material_id);
	yafaray_initObject(scene, object_id, material_id);

	yafaray_clearParamMap(param_map);
	yafaray_setParamMapBool(param_map, "has_orco", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapInt(param_map, "num_faces", 6);
	yafaray_setParamMapInt(param_map, "num_vertices", 8);
	yafaray_setParamMapInt(param_map, "object_index", 0);
	yafaray_setParamMapString(param_map, "type", "mesh");
	yafaray_createObject(scene, &object_id, "Cube", param_map);
	yafaray_addVertexWithOrco(scene, object_id, -5.01096, -1.94285, 1.00136e-05, -1, -1, -1);
	yafaray_addVertexWithOrco(scene, object_id, -5.01096, -1.94285, 2.00001, -1, -1, 1);
	yafaray_addVertexWithOrco(scene, object_id, -5.01096, 0.0571451, 1.00136e-05, -1, 1, -1);
	yafaray_addVertexWithOrco(scene, object_id, -5.01096, 0.0571451, 2.00001, -1, 1, 1);
	yafaray_addVertexWithOrco(scene, object_id, -3.01096, -1.94285, 1.00136e-05, 1, -1, -1);
	yafaray_addVertexWithOrco(scene, object_id, -3.01096, -1.94285, 2.00001, 1, -1, 1);
	yafaray_addVertexWithOrco(scene, object_id, -3.01096, 0.0571451, 1.00136e-05, 1, 1, -1);
	yafaray_addVertexWithOrco(scene, object_id, -3.01096, 0.0571451, 2.00001, 1, 1, 1);
	yafaray_getMaterialId(scene, &material_id, "Material.003");
	yafaray_addTriangle(scene, object_id, 2, 0, 1, material_id);
	yafaray_addTriangle(scene, object_id, 2, 1, 3, material_id);
	yafaray_addTriangle(scene, object_id, 3, 7, 6, material_id);
	yafaray_addTriangle(scene, object_id, 3, 6, 2, material_id);
	yafaray_addTriangle(scene, object_id, 7, 5, 4, material_id);
	yafaray_addTriangle(scene, object_id, 7, 4, 6, material_id);
	yafaray_addTriangle(scene, object_id, 0, 4, 5, material_id);
	yafaray_addTriangle(scene, object_id, 0, 5, 1, material_id);
	yafaray_addTriangle(scene, object_id, 0, 2, 6, material_id);
	yafaray_addTriangle(scene, object_id, 0, 6, 4, material_id);
	yafaray_addTriangle(scene, object_id, 5, 7, 3, material_id);
	yafaray_addTriangle(scene, object_id, 5, 3, 1, material_id);
	yafaray_initObject(scene, object_id, material_id);

	yafaray_clearParamMap(param_map);
	yafaray_setParamMapInt(param_map, "num_faces", 1);
	yafaray_setParamMapInt(param_map, "num_vertices", 4);
	yafaray_setParamMapInt(param_map, "object_index", 0);
	yafaray_setParamMapString(param_map, "type", "mesh");
	yafaray_createObject(scene, &object_id, "Plane", param_map);
	yafaray_addVertex(scene, object_id, -10, -10, 0);
	yafaray_addVertex(scene, object_id, 10, -10, 0);
	yafaray_addVertex(scene, object_id, -10, 10, 0);
	yafaray_addVertex(scene, object_id, 10, 10, 0);
	yafaray_getMaterialId(scene, &material_id, "Material.002");
	yafaray_addTriangle(scene, object_id, 0, 1, 3, material_id);
	yafaray_addTriangle(scene, object_id, 0, 3, 2, material_id);
	yafaray_initObject(scene, object_id, material_id);

	/* Setting up scene background */
	yafaray_clearParamMap(param_map);
	yafaray_setParamMapColor(param_map, "color", 0.7, 0.7, 0.7, 1);
	yafaray_setParamMapFloat(param_map, "power", 0.5);
	yafaray_setParamMapString(param_map, "type", "constant");
	yafaray_defineBackground(scene, param_map);

	/* Creating surface integrator */
	yafaray_clearParamMap(param_map);
	/* yafaray_setParamMapString(param_map, "type", "directlighting"); */
	yafaray_setParamMapString(param_map, "type", "photonmapping");
	yafaray_setParamMapInt(param_map, "raydepth", 2);
	yafaray_setParamMapInt(param_map, "shadowDepth", 2);
	yafaray_setParamMapInt(param_map, "diffuse_photons", 10000000);
	surface_integrator = yafaray_createSurfaceIntegrator(logger, "surface integrator", param_map);

	/* Creating films */
	yafaray_clearParamMap(param_map);
	yafaray_setParamMapInt(param_map, "AA_passes", 1);
	yafaray_setParamMapInt(param_map, "AA_minsamples", 3);
	yafaray_setParamMapInt(param_map, "AA_inc_samples", 3);
	yafaray_setParamMapInt(param_map, "width", 480);
	yafaray_setParamMapInt(param_map, "height", 270);
	film_1 = yafaray_createFilm(logger, surface_integrator, "film_1", param_map);
	yafaray_setParamMapInt(param_map, "width", 240);
	yafaray_setParamMapInt(param_map, "height", 135);
	film_2 = yafaray_createFilm(logger, surface_integrator, "film_2", param_map);

	/* Setting up film layers */
	yafaray_clearParamMap(param_map);
	yafaray_setParamMapString(param_map, "exported_image_name", "Combined");
	yafaray_setParamMapString(param_map, "exported_image_type", "ColorAlpha");
	yafaray_setParamMapString(param_map, "image_type", "ColorAlpha");
	yafaray_setParamMapString(param_map, "type", "combined");
	yafaray_defineLayer(film_1, param_map);
	yafaray_defineLayer(film_2, param_map);

	/* Creating film camera */
	yafaray_clearParamMap(param_map);
	yafaray_setParamMapFloat(param_map, "focal", 1.09375);
	yafaray_setParamMapVector(param_map, "from", 8.64791, -7.22615, 8.1295);
	yafaray_setParamMapInt(param_map, "resx", 480);
	yafaray_setParamMapInt(param_map, "resy", 270);
	yafaray_setParamMapVector(param_map, "to", 8.03447, -6.65603, 7.58301);
	yafaray_setParamMapString(param_map, "type", "perspective");
	yafaray_setParamMapVector(param_map, "up", 8.25644, -6.8447, 8.9669);
	yafaray_defineCamera(film_1, "Camera", param_map);
	yafaray_setParamMapInt(param_map, "resx", 240);
	yafaray_setParamMapInt(param_map, "resy", 135);
	yafaray_setParamMapVector(param_map, "from", 7.64791, -7.22615, 8.1295);
	yafaray_setParamMapVector(param_map, "to", 7.03447, -6.65603, 7.58301);
	yafaray_defineCamera(film_2, "Camera", param_map);

	/* Creating image output */
	yafaray_clearParamMap(param_map);
	yafaray_setParamMapString(param_map, "image_path", "./test10-output1.tga");
	yafaray_setParamMapString(param_map, "badge_position", "top");
	yafaray_createOutput(film_1, "output_tga", param_map);
	yafaray_setParamMapString(param_map, "image_path", "./test10-output2.tga");
	yafaray_createOutput(film_2, "output_tga", param_map);

	/* Rendering */
	render_monitor = yafaray_createRenderMonitor(NULL, NULL, YAFARAY_DISPLAY_CONSOLE_NORMAL);
	render_control = yafaray_createRenderControl();
	yafaray_setRenderControlForNormalStart(render_control);
	scene_modified_flags = yafaray_checkAndClearSceneModifiedFlags(scene);
	yafaray_preprocessScene(scene, render_control, scene_modified_flags);
	/* yafaray_preprocessSurfaceIntegrator(render_monitor, surface_integrator, render_control, scene); */
	/* yafaray_render(render_control, render_monitor, surface_integrator, film_1); */
	yafaray_setRenderControlForNormalStart(render_control);
	/* yafaray_render(render_control, render_monitor, surface_integrator, film_2); */
	yafaray_destroyRenderControl(render_control);
	yafaray_destroyRenderMonitor(render_monitor);

	container = yafaray_createContainer();
	yafaray_addSceneToContainer(container, scene);
	yafaray_addSurfaceIntegratorToContainer(container, surface_integrator);
	yafaray_addFilmToContainer(container, film_1);
	yafaray_addFilmToContainer(container, film_2);
	exported_string = yafaray_exportContainerToString(container, YAFARAY_CONTAINER_EXPORT_XML, YAFARAY_BOOL_TRUE);
	printf("**EXPORTED**\n%s\n", exported_string);

	/* Destruction/deallocation */
	yafaray_destroyCharString(exported_string);
	yafaray_destroyContainerAndContainedPointers(container);
/*	yafaray_destroyFilm(film_2);
	yafaray_destroyFilm(film_1);
	yafaray_destroySurfaceIntegrator(surface_integrator);
	yafaray_destroyScene(scene);*/
	yafaray_destroyLogger(logger);
	yafaray_destroyParamMapList(param_map_list);
	yafaray_destroyParamMap(param_map);

	return 0;
}
