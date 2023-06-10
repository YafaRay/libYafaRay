/****************************************************************************
 *      This is part of the libYafaRay package
 *
 *      test03.c : material nodes test scene
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

int main()
{
	yafaray_ParamMap *param_map = NULL;
	yafaray_ParamMapList *param_map_list = NULL;
	yafaray_Logger *logger = NULL;
	yafaray_SurfaceIntegrator *surface_integrator = NULL;
	yafaray_Scene *scene = NULL;
	yafaray_Film *film = NULL;
	yafaray_RenderControl *render_control = NULL;
	yafaray_RenderMonitor *render_monitor = NULL;
	yafaray_SceneModifiedFlags scene_modified_flags = YAFARAY_SCENE_MODIFIED_NOTHING;
	size_t material_id = 0;
	size_t object_id = 0;
	size_t image_id = 0;

	/* Creating logger */
	logger = yafaray_createLogger(NULL, NULL, YAFARAY_DISPLAY_CONSOLE_NORMAL);
	yafaray_setConsoleLogColorsEnabled(logger, YAFARAY_BOOL_TRUE);
	yafaray_setConsoleVerbosityLevel(logger, YAFARAY_LOG_LEVEL_VERBOSE);

	/* Creating scene */
	scene = yafaray_createScene(logger, "scene");

	/* Creating param map and param map list */
	param_map = yafaray_createParamMap();
	param_map_list = yafaray_createParamMapList();

	yafaray_clearParamMap(param_map);
	yafaray_setParamMapString(param_map, "color_space", "sRGB");
	yafaray_setParamMapString(param_map, "filename", "tex.hdr");
	yafaray_setParamMapFloat(param_map, "gamma", 1);
	yafaray_setParamMapString(param_map, "image_optimization", "optimized");
	yafaray_createImage(scene, "Tex_image", &image_id, param_map);

	yafaray_clearParamMap(param_map);
	yafaray_setParamMapString(param_map, "image_name", "Tex_image");
	yafaray_setParamMapString(param_map, "type", "image");
	yafaray_setParamMapBool(param_map, "adj_clamp", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapFloat(param_map, "adj_contrast", 1);
	yafaray_setParamMapFloat(param_map, "adj_hue", 0);
	yafaray_setParamMapFloat(param_map, "adj_intensity", 1);
	yafaray_setParamMapFloat(param_map, "adj_mult_factor_blue", 1);
	yafaray_setParamMapFloat(param_map, "adj_mult_factor_green", 1);
	yafaray_setParamMapFloat(param_map, "adj_mult_factor_red", 1);
	yafaray_setParamMapFloat(param_map, "adj_saturation", 1);
	yafaray_setParamMapBool(param_map, "calc_alpha", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapString(param_map, "clipping", "repeat");
	yafaray_setParamMapFloat(param_map, "cropmax_x", 1);
	yafaray_setParamMapFloat(param_map, "cropmax_y", 1);
	yafaray_setParamMapFloat(param_map, "cropmin_x", 0);
	yafaray_setParamMapFloat(param_map, "cropmin_y", 0);
	yafaray_setParamMapFloat(param_map, "ewa_max_anisotropy", 8);
	yafaray_setParamMapString(param_map, "interpolate", "bilinear");
	yafaray_setParamMapBool(param_map, "mirror_x", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapBool(param_map, "mirror_y", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapBool(param_map, "normalmap", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapBool(param_map, "rot90", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapFloat(param_map, "trilinear_level_bias", 0);
	yafaray_setParamMapBool(param_map, "use_alpha", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapInt(param_map, "xrepeat", 1);
	yafaray_setParamMapInt(param_map, "yrepeat", 1);
	yafaray_createTexture(scene, "Tex", param_map);

	yafaray_clearParamMap(param_map);
	yafaray_setParamMapBool(param_map, "adj_clamp", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapFloat(param_map, "adj_contrast", 1);
	yafaray_setParamMapFloat(param_map, "adj_hue", 0);
	yafaray_setParamMapFloat(param_map, "adj_intensity", 1);
	yafaray_setParamMapFloat(param_map, "adj_mult_factor_blue", 1);
	yafaray_setParamMapFloat(param_map, "adj_mult_factor_green", 1);
	yafaray_setParamMapFloat(param_map, "adj_mult_factor_red", 1);
	yafaray_setParamMapFloat(param_map, "adj_saturation", 1);
	yafaray_setParamMapInt(param_map, "depth", 0);
	yafaray_setParamMapBool(param_map, "hard", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapString(param_map, "noise_type", "blender");
	yafaray_setParamMapString(param_map, "shape", "sin");
	yafaray_setParamMapFloat(param_map, "size", 0.25);
	yafaray_setParamMapFloat(param_map, "turbulence", 0);
	yafaray_setParamMapString(param_map, "type", "wood");
	yafaray_setParamMapString(param_map, "wood_type", "bands");
	yafaray_createTexture(scene, "Texture", param_map);

	yafaray_clearParamMap(param_map);
	yafaray_setParamMapString(param_map, "color_space", "sRGB");
	yafaray_setParamMapString(param_map, "filename", "tex.tga");
	yafaray_setParamMapFloat(param_map, "gamma", 1);
	yafaray_setParamMapString(param_map, "image_optimization", "optimized");
	yafaray_createImage(scene, "Texture.001_image", &image_id, param_map);

	yafaray_clearParamMap(param_map);
	yafaray_setParamMapString(param_map, "image_name", "Texture.001_image");
	yafaray_setParamMapString(param_map, "type", "image");
	yafaray_setParamMapBool(param_map, "adj_clamp", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapFloat(param_map, "adj_contrast", 1);
	yafaray_setParamMapFloat(param_map, "adj_hue", 0);
	yafaray_setParamMapFloat(param_map, "adj_intensity", 1);
	yafaray_setParamMapFloat(param_map, "adj_mult_factor_blue", 1);
	yafaray_setParamMapFloat(param_map, "adj_mult_factor_green", 1);
	yafaray_setParamMapFloat(param_map, "adj_mult_factor_red", 1);
	yafaray_setParamMapFloat(param_map, "adj_saturation", 1);
	yafaray_setParamMapBool(param_map, "calc_alpha", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapString(param_map, "clipping", "repeat");
	yafaray_setParamMapFloat(param_map, "cropmax_x", 1);
	yafaray_setParamMapFloat(param_map, "cropmax_y", 1);
	yafaray_setParamMapFloat(param_map, "cropmin_x", 0);
	yafaray_setParamMapFloat(param_map, "cropmin_y", 0);
	yafaray_setParamMapFloat(param_map, "ewa_max_anisotropy", 8);
	yafaray_setParamMapString(param_map, "interpolate", "bilinear");
	yafaray_setParamMapBool(param_map, "mirror_x", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapBool(param_map, "mirror_y", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapBool(param_map, "normalmap", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapBool(param_map, "rot90", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapFloat(param_map, "trilinear_level_bias", 0);
	yafaray_setParamMapBool(param_map, "use_alpha", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapInt(param_map, "xrepeat", 1);
	yafaray_setParamMapInt(param_map, "yrepeat", 1);
	yafaray_createTexture(scene, "Texture.001", param_map);

	yafaray_clearParamMapList(param_map_list);
	yafaray_clearParamMap(param_map);
	yafaray_setParamMapColor(param_map, "color", 0.8, 0.8, 0.8, 1);
	yafaray_setParamMapString(param_map, "type", "shinydiffusemat");
	yafaray_createMaterial(scene, &material_id, "defaultMat", param_map, param_map_list);

	yafaray_clearParamMapList(param_map_list);
	yafaray_clearParamMap(param_map);
	yafaray_setParamMapFloat(param_map, "IOR", 1.52);
	yafaray_setParamMapColor(param_map, "absorption", 1, 1, 1, 1);
	yafaray_setParamMapFloat(param_map, "absorption_dist", 1);
	yafaray_setParamMapInt(param_map, "additionaldepth", 0);
	yafaray_setParamMapFloat(param_map, "dispersion_power", 0);
	yafaray_setParamMapBool(param_map, "fake_shadows", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapColor(param_map, "filter_color", 1, 1, 1, 1);
	yafaray_setParamMapBool(param_map, "flat_material", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapInt(param_map, "mat_pass_index", 0);
	yafaray_setParamMapColor(param_map, "mirror_color", 1, 1, 1, 1);
	yafaray_setParamMapBool(param_map, "receive_shadows", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapFloat(param_map, "samplingfactor", 1);
	yafaray_setParamMapFloat(param_map, "transmit_filter", 1);
	yafaray_setParamMapString(param_map, "type", "glass");
	yafaray_setParamMapString(param_map, "visibility", "normal");
	yafaray_setParamMapFloat(param_map, "wireframe_amount", 0);
	yafaray_setParamMapColor(param_map, "wireframe_color", 1, 1, 1, 1);
	yafaray_setParamMapFloat(param_map, "wireframe_exponent", 0);
	yafaray_setParamMapFloat(param_map, "wireframe_thickness", 0.01);
	yafaray_createMaterial(scene, &material_id, "Mat1", param_map, param_map_list);

	yafaray_clearParamMapList(param_map_list);
	yafaray_clearParamMap(param_map);
	yafaray_setParamMapString(param_map, "blend_mode", "mix");
	yafaray_setParamMapFloat(param_map, "colfac", 1);
	yafaray_setParamMapBool(param_map, "color_input", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapColor(param_map, "def_col", 1, 0, 1, 1);
	yafaray_setParamMapFloat(param_map, "def_val", 1);
	yafaray_setParamMapBool(param_map, "do_color", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapBool(param_map, "do_scalar", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapString(param_map, "input", "map0");
	yafaray_setParamMapString(param_map, "name", "diff_layer0");
	yafaray_setParamMapBool(param_map, "negative", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapBool(param_map, "noRGB", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapBool(param_map, "stencil", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapString(param_map, "type", "layer");
	yafaray_setParamMapColor(param_map, "upper_color", 0.8, 0.8, 0.8, 1);
	yafaray_setParamMapFloat(param_map, "upper_value", 0);
	yafaray_setParamMapBool(param_map, "use_alpha", YAFARAY_BOOL_FALSE);
	yafaray_addParamMapToList(param_map_list, param_map);
	yafaray_clearParamMap(param_map);
	yafaray_setParamMapString(param_map, "mapping", "plain");
	yafaray_setParamMapString(param_map, "name", "map0");
	yafaray_setParamMapVector(param_map, "offset", 0, 0, 0);
	yafaray_setParamMapInt(param_map, "proj_x", 1);
	yafaray_setParamMapInt(param_map, "proj_y", 2);
	yafaray_setParamMapInt(param_map, "proj_z", 3);
	yafaray_setParamMapVector(param_map, "scale", 1, 1, 1);
	yafaray_setParamMapString(param_map, "texco", "orco");
	yafaray_setParamMapString(param_map, "texture", "Tex");
	yafaray_setParamMapString(param_map, "type", "texture_mapper");
	yafaray_addParamMapToList(param_map_list, param_map);
	yafaray_clearParamMap(param_map);
	yafaray_setParamMapFloat(param_map, "IOR", 1.8);
	yafaray_setParamMapInt(param_map, "additionaldepth", 0);
	yafaray_setParamMapColor(param_map, "color", 0.8, 0.8, 0.8, 1);
	yafaray_setParamMapFloat(param_map, "diffuse_reflect", 1);
	yafaray_setParamMapString(param_map, "diffuse_shader", "diff_layer0");
	yafaray_setParamMapFloat(param_map, "emit", 0);
	yafaray_setParamMapBool(param_map, "flat_material", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapBool(param_map, "fresnel_effect", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapInt(param_map, "mat_pass_index", 0);
	yafaray_setParamMapColor(param_map, "mirror_color", 1, 1, 1, 1);
	yafaray_setParamMapBool(param_map, "receive_shadows", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapFloat(param_map, "samplingfactor", 1);
	yafaray_setParamMapFloat(param_map, "specular_reflect", 0);
	yafaray_setParamMapFloat(param_map, "translucency", 0);
	yafaray_setParamMapFloat(param_map, "transmit_filter", 1);
	yafaray_setParamMapFloat(param_map, "transparency", 0);
	yafaray_setParamMapFloat(param_map, "transparentbias_factor", 0);
	yafaray_setParamMapBool(param_map, "transparentbias_multiply_raydepth", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapString(param_map, "type", "shinydiffusemat");
	yafaray_setParamMapString(param_map, "visibility", "normal");
	yafaray_setParamMapFloat(param_map, "wireframe_amount", 0);
	yafaray_setParamMapColor(param_map, "wireframe_color", 1, 1, 1, 1);
	yafaray_setParamMapFloat(param_map, "wireframe_exponent", 0);
	yafaray_setParamMapFloat(param_map, "wireframe_thickness", 0.01);
	yafaray_createMaterial(scene, &material_id, "diffuse2", param_map, param_map_list);

	yafaray_clearParamMapList(param_map_list);
	yafaray_clearParamMap(param_map);
	yafaray_setParamMapString(param_map, "blend_mode", "mix");
	yafaray_setParamMapFloat(param_map, "colfac", 1);
	yafaray_setParamMapBool(param_map, "color_input", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapColor(param_map, "def_col", 1, 0, 1, 1);
	yafaray_setParamMapFloat(param_map, "def_val", 1);
	yafaray_setParamMapBool(param_map, "do_color", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapBool(param_map, "do_scalar", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapString(param_map, "input", "map0");
	yafaray_setParamMapString(param_map, "name", "diff_layer0");
	yafaray_setParamMapBool(param_map, "negative", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapBool(param_map, "noRGB", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapBool(param_map, "stencil", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapString(param_map, "type", "layer");
	yafaray_setParamMapColor(param_map, "upper_color", 0.8, 0.8, 0.8, 1);
	yafaray_setParamMapFloat(param_map, "upper_value", 0);
	yafaray_setParamMapBool(param_map, "use_alpha", YAFARAY_BOOL_FALSE);
	yafaray_addParamMapToList(param_map_list, param_map);
	yafaray_clearParamMap(param_map);
	yafaray_setParamMapString(param_map, "mapping", "cube");
	yafaray_setParamMapString(param_map, "name", "map0");
	yafaray_setParamMapVector(param_map, "offset", 0, 0, 0);
	yafaray_setParamMapInt(param_map, "proj_x", 1);
	yafaray_setParamMapInt(param_map, "proj_y", 2);
	yafaray_setParamMapInt(param_map, "proj_z", 3);
	yafaray_setParamMapVector(param_map, "scale", 1, 1, 1);
	yafaray_setParamMapString(param_map, "texco", "orco");
	yafaray_setParamMapString(param_map, "texture", "Tex");
	yafaray_setParamMapString(param_map, "type", "texture_mapper");
	yafaray_addParamMapToList(param_map_list, param_map);
	yafaray_clearParamMap(param_map);
	yafaray_setParamMapString(param_map, "blend_mode", "mix");
	yafaray_setParamMapFloat(param_map, "colfac", 1);
	yafaray_setParamMapBool(param_map, "color_input", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapColor(param_map, "def_col", 1, 0, 1, 1);
	yafaray_setParamMapFloat(param_map, "def_val", 1);
	yafaray_setParamMapBool(param_map, "do_color", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapBool(param_map, "do_scalar", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapString(param_map, "input", "map1");
	yafaray_setParamMapString(param_map, "name", "diff_layer1");
	yafaray_setParamMapBool(param_map, "negative", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapBool(param_map, "noRGB", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapBool(param_map, "stencil", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapString(param_map, "type", "layer");
	yafaray_setParamMapString(param_map, "upper_layer", "diff_layer0");
	yafaray_setParamMapBool(param_map, "use_alpha", YAFARAY_BOOL_FALSE);
	yafaray_addParamMapToList(param_map_list, param_map);
	yafaray_clearParamMap(param_map);
	yafaray_setParamMapString(param_map, "mapping", "cube");
	yafaray_setParamMapString(param_map, "name", "map1");
	yafaray_setParamMapVector(param_map, "offset", 0, 0, 0);
	yafaray_setParamMapInt(param_map, "proj_x", 1);
	yafaray_setParamMapInt(param_map, "proj_y", 2);
	yafaray_setParamMapInt(param_map, "proj_z", 3);
	yafaray_setParamMapVector(param_map, "scale", 1, 1, 1);
	yafaray_setParamMapString(param_map, "texco", "orco");
	yafaray_setParamMapString(param_map, "texture", "Texture");
	yafaray_setParamMapString(param_map, "type", "texture_mapper");
	yafaray_addParamMapToList(param_map_list, param_map);
	yafaray_clearParamMap(param_map);
	yafaray_setParamMapString(param_map, "blend_mode", "mix");
	yafaray_setParamMapFloat(param_map, "colfac", 1);
	yafaray_setParamMapBool(param_map, "color_input", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapColor(param_map, "def_col", 1, 0, 1, 1);
	yafaray_setParamMapFloat(param_map, "def_val", 1);
	yafaray_setParamMapBool(param_map, "do_color", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapBool(param_map, "do_scalar", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapString(param_map, "input", "map2");
	yafaray_setParamMapString(param_map, "name", "diff_layer2");
	yafaray_setParamMapBool(param_map, "negative", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapBool(param_map, "noRGB", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapBool(param_map, "stencil", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapString(param_map, "type", "layer");
	yafaray_setParamMapString(param_map, "upper_layer", "diff_layer1");
	yafaray_setParamMapBool(param_map, "use_alpha", YAFARAY_BOOL_FALSE);
	yafaray_addParamMapToList(param_map_list, param_map);
	yafaray_clearParamMap(param_map);
	yafaray_setParamMapString(param_map, "mapping", "cube");
	yafaray_setParamMapString(param_map, "name", "map2");
	yafaray_setParamMapVector(param_map, "offset", 0, 0, 0);
	yafaray_setParamMapInt(param_map, "proj_x", 1);
	yafaray_setParamMapInt(param_map, "proj_y", 2);
	yafaray_setParamMapInt(param_map, "proj_z", 3);
	yafaray_setParamMapVector(param_map, "scale", 1, 1, 1);
	yafaray_setParamMapString(param_map, "texco", "orco");
	yafaray_setParamMapString(param_map, "texture", "Texture.001");
	yafaray_setParamMapString(param_map, "type", "texture_mapper");
	yafaray_addParamMapToList(param_map_list, param_map);
	yafaray_clearParamMap(param_map);
	yafaray_setParamMapFloat(param_map, "IOR", 1.8);
	yafaray_setParamMapInt(param_map, "additionaldepth", 0);
	yafaray_setParamMapColor(param_map, "color", 0.8, 0.8, 0.8, 1);
	yafaray_setParamMapFloat(param_map, "diffuse_reflect", 1);
	yafaray_setParamMapString(param_map, "diffuse_shader", "diff_layer2");
	yafaray_setParamMapFloat(param_map, "emit", 0);
	yafaray_setParamMapBool(param_map, "flat_material", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapBool(param_map, "fresnel_effect", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapInt(param_map, "mat_pass_index", 0);
	yafaray_setParamMapColor(param_map, "mirror_color", 1, 1, 1, 1);
	yafaray_setParamMapBool(param_map, "receive_shadows", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapFloat(param_map, "samplingfactor", 1);
	yafaray_setParamMapFloat(param_map, "specular_reflect", 0);
	yafaray_setParamMapFloat(param_map, "translucency", 0);
	yafaray_setParamMapFloat(param_map, "transmit_filter", 1);
	yafaray_setParamMapFloat(param_map, "transparency", 0);
	yafaray_setParamMapFloat(param_map, "transparentbias_factor", 0);
	yafaray_setParamMapBool(param_map, "transparentbias_multiply_raydepth", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapString(param_map, "type", "shinydiffusemat");
	yafaray_setParamMapString(param_map, "visibility", "normal");
	yafaray_setParamMapFloat(param_map, "wireframe_amount", 0);
	yafaray_setParamMapColor(param_map, "wireframe_color", 1, 1, 1, 1);
	yafaray_setParamMapFloat(param_map, "wireframe_exponent", 0);
	yafaray_setParamMapFloat(param_map, "wireframe_thickness", 0.01);
	yafaray_createMaterial(scene, &material_id, "Mat2", param_map, param_map_list);

	yafaray_clearParamMapList(param_map_list);
	yafaray_clearParamMap(param_map);
	yafaray_setParamMapInt(param_map, "additionaldepth", 0);
	yafaray_setParamMapFloat(param_map, "blend_value", 0.5);
	yafaray_setParamMapBool(param_map, "flat_material", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapString(param_map, "material1", "Mat1");
	yafaray_setParamMapString(param_map, "material2", "Mat2");
	yafaray_setParamMapBool(param_map, "receive_shadows", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapFloat(param_map, "samplingfactor", 1);
	yafaray_setParamMapString(param_map, "type", "blend_mat");
	yafaray_setParamMapString(param_map, "visibility", "normal");
	yafaray_setParamMapFloat(param_map, "wireframe_amount", 0);
	yafaray_setParamMapColor(param_map, "wireframe_color", 1, 1, 1, 1);
	yafaray_setParamMapFloat(param_map, "wireframe_exponent", 0);
	yafaray_setParamMapFloat(param_map, "wireframe_thickness", 0.01);
	yafaray_createMaterial(scene, &material_id, "blend", param_map, param_map_list);

	yafaray_clearParamMapList(param_map_list);
	yafaray_clearParamMap(param_map);
	yafaray_setParamMapFloat(param_map, "IOR", 1.8);
	yafaray_setParamMapInt(param_map, "additionaldepth", 0);
	yafaray_setParamMapColor(param_map, "color", 0.8, 0.8, 0.8, 1);
	yafaray_setParamMapFloat(param_map, "diffuse_reflect", 1);
	yafaray_setParamMapFloat(param_map, "emit", 0);
	yafaray_setParamMapBool(param_map, "flat_material", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapBool(param_map, "fresnel_effect", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapInt(param_map, "mat_pass_index", 0);
	yafaray_setParamMapColor(param_map, "mirror_color", 1, 1, 1, 1);
	yafaray_setParamMapBool(param_map, "receive_shadows", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapFloat(param_map, "samplingfactor", 1);
	yafaray_setParamMapFloat(param_map, "specular_reflect", 0);
	yafaray_setParamMapFloat(param_map, "translucency", 0);
	yafaray_setParamMapFloat(param_map, "transmit_filter", 1);
	yafaray_setParamMapFloat(param_map, "transparency", 0);
	yafaray_setParamMapFloat(param_map, "transparentbias_factor", 0);
	yafaray_setParamMapBool(param_map, "transparentbias_multiply_raydepth", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapString(param_map, "type", "shinydiffusemat");
	yafaray_setParamMapString(param_map, "visibility", "normal");
	yafaray_setParamMapFloat(param_map, "wireframe_amount", 0);
	yafaray_setParamMapColor(param_map, "wireframe_color", 1, 1, 1, 1);
	yafaray_setParamMapFloat(param_map, "wireframe_exponent", 0);
	yafaray_setParamMapFloat(param_map, "wireframe_thickness", 0.01);
	yafaray_createMaterial(scene, &material_id, "diffuse", param_map, param_map_list);

	yafaray_clearParamMap(param_map);
	yafaray_setParamMapBool(param_map, "cast_shadows", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapColor(param_map, "color", 1, 1, 1, 1);
	yafaray_setParamMapVector(param_map, "from", -4.44722, 1.00545, 5.90386);
	yafaray_setParamMapBool(param_map, "light_enabled", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapBool(param_map, "photon_only", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapFloat(param_map, "power", 50);
	yafaray_setParamMapString(param_map, "type", "pointlight");
	yafaray_setParamMapBool(param_map, "with_caustic", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapBool(param_map, "with_diffuse", YAFARAY_BOOL_TRUE);
	yafaray_createLight(scene, "Lamp.001", param_map);

	yafaray_clearParamMap(param_map);
	yafaray_setParamMapBool(param_map, "cast_shadows", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapColor(param_map, "color", 1, 1, 1, 1);
	yafaray_setParamMapVector(param_map, "from", 4.07625, 1.00545, 5.90386);
	yafaray_setParamMapBool(param_map, "light_enabled", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapBool(param_map, "photon_only", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapFloat(param_map, "power", 50);
	yafaray_setParamMapString(param_map, "type", "pointlight");
	yafaray_setParamMapBool(param_map, "with_caustic", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapBool(param_map, "with_diffuse", YAFARAY_BOOL_TRUE);
	yafaray_createLight(scene, "Lamp", param_map);

	yafaray_clearParamMap(param_map);
	yafaray_setParamMapBool(param_map, "has_orco", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapBool(param_map, "has_uv", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapBool(param_map, "is_base_object", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapInt(param_map, "num_faces", 6);
	yafaray_setParamMapInt(param_map, "num_vertices", 8);
	yafaray_setParamMapInt(param_map, "object_index", 0);
	yafaray_setParamMapString(param_map, "type", "mesh");
	yafaray_setParamMapString(param_map, "visibility", "normal");
	yafaray_createObject(scene, &object_id, "Cube.002", param_map);
	yafaray_addVertexWithOrco(scene, object_id, 1, 4, 1.00136e-05, 1, 1, -1);
	yafaray_addVertexWithOrco(scene, object_id, 1, 2, 1.00136e-05, 1, -0.999999, -1);
	yafaray_addVertexWithOrco(scene, object_id, -1, 2, 1.00136e-05, -1, -0.999999, -1);
	yafaray_addVertexWithOrco(scene, object_id, -1, 4, 1.00136e-05, -0.999999, 1, -1);
	yafaray_addVertexWithOrco(scene, object_id, 1, 4, 2.00001, 1, 0.999999, 1);
	yafaray_addVertexWithOrco(scene, object_id, 0.999999, 2, 2.00001, 0.999999, -1, 1);
	yafaray_addVertexWithOrco(scene, object_id, -1, 2, 2.00001, -1, -0.999999, 1);
	yafaray_addVertexWithOrco(scene, object_id, -1, 4, 2.00001, -1, 1, 1);
	yafaray_getMaterialId(scene, &material_id, "Mat1");
	yafaray_addTriangle(scene, object_id, 0, 1, 2, material_id);
	yafaray_addTriangle(scene, object_id, 0, 2, 3, material_id);
	yafaray_addTriangle(scene, object_id, 4, 7, 6, material_id);
	yafaray_addTriangle(scene, object_id, 4, 6, 5, material_id);
	yafaray_addTriangle(scene, object_id, 0, 4, 5, material_id);
	yafaray_addTriangle(scene, object_id, 0, 5, 1, material_id);
	yafaray_addTriangle(scene, object_id, 1, 5, 6, material_id);
	yafaray_addTriangle(scene, object_id, 1, 6, 2, material_id);
	yafaray_addTriangle(scene, object_id, 2, 6, 7, material_id);
	yafaray_addTriangle(scene, object_id, 2, 7, 3, material_id);
	yafaray_addTriangle(scene, object_id, 4, 0, 3, material_id);
	yafaray_addTriangle(scene, object_id, 4, 3, 7, material_id);
	yafaray_initObject(scene, object_id, material_id);

	yafaray_clearParamMap(param_map);
	yafaray_setParamMapBool(param_map, "has_orco", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapBool(param_map, "has_uv", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapBool(param_map, "is_base_object", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapInt(param_map, "num_faces", 6);
	yafaray_setParamMapInt(param_map, "num_vertices", 8);
	yafaray_setParamMapInt(param_map, "object_index", 0);
	yafaray_setParamMapString(param_map, "type", "mesh");
	yafaray_setParamMapString(param_map, "visibility", "normal");
	yafaray_createObject(scene, &object_id, "Cube.001", param_map);
	yafaray_addVertexWithOrco(scene, object_id, 1, -2, 1.00136e-05, 1, 1, -1);
	yafaray_addVertexWithOrco(scene, object_id, 1, -4, 1.00136e-05, 1, -0.999999, -1);
	yafaray_addVertexWithOrco(scene, object_id, -1, -4, 1.00136e-05, -1, -0.999999, -1);
	yafaray_addVertexWithOrco(scene, object_id, -1, -2, 1.00136e-05, -0.999999, 1, -1);
	yafaray_addVertexWithOrco(scene, object_id, 1, -2, 2.00001, 1, 0.999999, 1);
	yafaray_addVertexWithOrco(scene, object_id, 0.999999, -4, 2.00001, 0.999999, -1, 1);
	yafaray_addVertexWithOrco(scene, object_id, -1, -4, 2.00001, -1, -0.999999, 1);
	yafaray_addVertexWithOrco(scene, object_id, -1, -2, 2.00001, -1, 1, 1);
	yafaray_getMaterialId(scene, &material_id, "blend");
	yafaray_addTriangle(scene, object_id, 0, 1, 2, material_id);
	yafaray_addTriangle(scene, object_id, 0, 2, 3, material_id);
	yafaray_addTriangle(scene, object_id, 4, 7, 6, material_id);
	yafaray_addTriangle(scene, object_id, 4, 6, 5, material_id);
	yafaray_addTriangle(scene, object_id, 0, 4, 5, material_id);
	yafaray_addTriangle(scene, object_id, 0, 5, 1, material_id);
	yafaray_addTriangle(scene, object_id, 1, 5, 6, material_id);
	yafaray_addTriangle(scene, object_id, 1, 6, 2, material_id);
	yafaray_addTriangle(scene, object_id, 2, 6, 7, material_id);
	yafaray_addTriangle(scene, object_id, 2, 7, 3, material_id);
	yafaray_addTriangle(scene, object_id, 4, 0, 3, material_id);
	yafaray_addTriangle(scene, object_id, 4, 3, 7, material_id);
	yafaray_initObject(scene, object_id, material_id);

	yafaray_clearParamMap(param_map);
	yafaray_setParamMapBool(param_map, "has_orco", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapBool(param_map, "has_uv", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapBool(param_map, "is_base_object", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapInt(param_map, "num_faces", 1);
	yafaray_setParamMapInt(param_map, "num_vertices", 4);
	yafaray_setParamMapInt(param_map, "object_index", 0);
	yafaray_setParamMapString(param_map, "type", "mesh");
	yafaray_setParamMapString(param_map, "visibility", "normal");
	yafaray_createObject(scene, &object_id, "Plane", param_map);
	yafaray_addVertex(scene, object_id, -10, -10, 0);
	yafaray_addVertex(scene, object_id, 10, -10, 0);
	yafaray_addVertex(scene, object_id, -10, 10, 0);
	yafaray_addVertex(scene, object_id, 10, 10, 0);
	yafaray_getMaterialId(scene, &material_id, "diffuse");
	yafaray_addTriangle(scene, object_id, 0, 1, 3, material_id);
	yafaray_addTriangle(scene, object_id, 0, 3, 2, material_id);
	yafaray_initObject(scene, object_id, material_id);

	yafaray_clearParamMap(param_map);
	yafaray_setParamMapBool(param_map, "has_orco", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapBool(param_map, "has_uv", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapBool(param_map, "is_base_object", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapInt(param_map, "num_faces", 6);
	yafaray_setParamMapInt(param_map, "num_vertices", 8);
	yafaray_setParamMapInt(param_map, "object_index", 0);
	yafaray_setParamMapString(param_map, "type", "mesh");
	yafaray_setParamMapString(param_map, "visibility", "normal");
	yafaray_createObject(scene, &object_id, "Cube", param_map);
	yafaray_addVertexWithOrco(scene, object_id, 1, 1, 1.00136e-05, 1, 1, -1);
	yafaray_addVertexWithOrco(scene, object_id, 1, -1, 1.00136e-05, 1, -0.999999, -1);
	yafaray_addVertexWithOrco(scene, object_id, -1, -1, 1.00136e-05, -1, -0.999999, -1);
	yafaray_addVertexWithOrco(scene, object_id, -1, 1, 1.00136e-05, -0.999999, 1, -1);
	yafaray_addVertexWithOrco(scene, object_id, 1, 0.999999, 2.00001, 1, 0.999999, 1);
	yafaray_addVertexWithOrco(scene, object_id, 0.999999, -1, 2.00001, 0.999999, -1, 1);
	yafaray_addVertexWithOrco(scene, object_id, -1, -1, 2.00001, -1, -0.999999, 1);
	yafaray_addVertexWithOrco(scene, object_id, -1, 1, 2.00001, -1, 1, 1);
	yafaray_getMaterialId(scene, &material_id, "Mat2");
	yafaray_addTriangle(scene, object_id, 0, 1, 2, material_id);
	yafaray_addTriangle(scene, object_id, 0, 2, 3, material_id);
	yafaray_addTriangle(scene, object_id, 4, 7, 6, material_id);
	yafaray_addTriangle(scene, object_id, 4, 6, 5, material_id);
	yafaray_addTriangle(scene, object_id, 0, 4, 5, material_id);
	yafaray_addTriangle(scene, object_id, 0, 5, 1, material_id);
	yafaray_addTriangle(scene, object_id, 1, 5, 6, material_id);
	yafaray_addTriangle(scene, object_id, 1, 6, 2, material_id);
	yafaray_addTriangle(scene, object_id, 2, 6, 7, material_id);
	yafaray_addTriangle(scene, object_id, 2, 7, 3, material_id);
	yafaray_addTriangle(scene, object_id, 4, 0, 3, material_id);
	yafaray_addTriangle(scene, object_id, 4, 3, 7, material_id);
	yafaray_initObject(scene, object_id, material_id);

	/* Setting up scene background */
	yafaray_clearParamMap(param_map);
	yafaray_setParamMapColor(param_map, "color", 0.155663, 0.155663, 0.155663, 1);
	yafaray_setParamMapFloat(param_map, "power", 0.5);
	yafaray_setParamMapString(param_map, "type", "constant");
	yafaray_defineBackground(scene, param_map);

	/* Creating surface integrator */
	yafaray_clearParamMap(param_map);
	yafaray_setParamMapString(param_map, "type", "directlighting");
	/* yafaray_setParamMapString(param_map, "type", "photonmapping"); */
	yafaray_setParamMapInt(param_map, "AA_passes", 5);
	yafaray_setParamMapInt(param_map, "AA_minsamples", 3);
	yafaray_setParamMapInt(param_map, "AA_inc_samples", 3);
	yafaray_setParamMapInt(param_map, "raydepth", 5);
	yafaray_setParamMapInt(param_map, "shadowDepth", 5);
	surface_integrator = yafaray_createSurfaceIntegrator(logger, "surface integrator", param_map);

	/* Creating film */
	yafaray_clearParamMap(param_map);
	yafaray_setParamMapInt(param_map, "width", 768);
	yafaray_setParamMapInt(param_map, "height", 432);
	film = yafaray_createFilm(logger, surface_integrator, "film", param_map);

	/* Creating film camera */
	yafaray_clearParamMap(param_map);
	yafaray_setParamMapFloat(param_map, "aperture", 0);
	yafaray_setParamMapFloat(param_map, "bokeh_rotation", 0);
	yafaray_setParamMapString(param_map, "bokeh_type", "disk1");
	yafaray_setParamMapFloat(param_map, "dof_distance", 0);
	yafaray_setParamMapFloat(param_map, "focal", 1.09375);
	yafaray_setParamMapVector(param_map, "from", 7.48113, -6.50764, 5.34367);
	yafaray_setParamMapInt(param_map, "resx", 768);
	yafaray_setParamMapInt(param_map, "resy", 432);
	yafaray_setParamMapVector(param_map, "to", 6.82627, -5.89697, 4.89842);
	yafaray_setParamMapString(param_map, "type", "perspective");
	yafaray_setParamMapVector(param_map, "up", 7.16376, -6.19517, 6.23901);
	yafaray_defineCamera(film, param_map);

	/* Creating image output */
	yafaray_clearParamMap(param_map);
	yafaray_setParamMapString(param_map, "image_path", "./test03-output1.tga");
	yafaray_setParamMapString(param_map, "badge_position", "top");
	yafaray_createOutput(film, "output1_tga", param_map);

	/* Rendering */
	render_monitor = yafaray_createRenderMonitor(NULL, NULL, YAFARAY_DISPLAY_CONSOLE_NORMAL);
	render_control = yafaray_createRenderControl();
	yafaray_setRenderControlForNormalStart(render_control);
	scene_modified_flags = yafaray_checkAndClearSceneModifiedFlags(scene);
	yafaray_preprocessScene(scene, render_control, scene_modified_flags);
	yafaray_preprocessSurfaceIntegrator(render_monitor, surface_integrator, render_control, scene);
	yafaray_render(render_control, render_monitor, surface_integrator, film);
	yafaray_destroyRenderControl(render_control);
	yafaray_destroyRenderMonitor(render_monitor);

	/* Destruction/deallocation */
	yafaray_destroyFilm(film);
	yafaray_destroySurfaceIntegrator(surface_integrator);
	yafaray_destroyScene(scene);
	yafaray_destroyLogger(logger);
	yafaray_destroyParamMapList(param_map_list);
	yafaray_destroyParamMap(param_map);

	return 0;
}
